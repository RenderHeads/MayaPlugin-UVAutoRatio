//
// UVAutoRatio Maya Plugin Source Code
// Copyright (C) 2007-2014 RenderHeads Ltd.
//
// This source is available for distribution and/or modification
// only under the terms of the MIT license.  All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the MIT license
// for more details.
//

#include "MayaPCH.h"
#include "MayaUtility.h"
#include "Utility.h"
#include "UVMesh.h"
#include "UVShell.h"
#include "UVSpringLayout.h"
#include "UVAutoRatioPro.h"

using namespace std;

//static const char* error_nothingSelected = "At least 1 mesh or mesh component must be selected";
static const char* error_minimumSelection = "At least 1 mesh must be selected";
static const char* error_unknownOpMode = "Unknown operation mode";
static const char* error_componentConversionError = "Unexpected result during component conversion";

int	UVAutoRatioPro::m_subTasks = 0;
int	UVAutoRatioPro::m_subTasksLeft = 0;
double UVAutoRatioPro::m_subTaskJump = 0;
MString UVAutoRatioPro::m_subTaskName;
int	UVAutoRatioPro::m_totalJobs = 0;
double UVAutoRatioPro::m_progress = 0.0;
double UVAutoRatioPro::m_progressStep = 0.0;
MString UVAutoRatioPro::m_progressMessage;

MStatus
UVAutoRatioPro::doIt(const MArgList& args)
{
	MStatus status;

	SaveSelection();

	status = Initialise(args);

	if (status == MS::kSuccess)
	{
		status = ProcessMeshes();
	}

	RestoreSelection();

	MProgressWindow::endProgress();

	return status;
}

MStatus
UVAutoRatioPro::Initialise(const MArgList& args)
{
	clearResult();

	// Parse arguments
	const char* errorMessage = ParseArguments(args);

	// Display help
	if (m_params.m_isHelp)
	{
		DisplayHelp();
		return MS::kSuccess;
	}

	if (errorMessage)
	{
		displayError(errorMessage);
		return MS::kFailure;
	}

	// Check we have enough objects selected
	if (m_savedSelection.length() < 1)
	{
		displayError(error_minimumSelection);
		return MS::kFailure;
	}

	return MS::kSuccess;
}

void
UVAutoRatioPro::TestColourFaces()
{
	for (uint i = 0; i < m_meshes.size(); i++)
	{
		if (MGlobal::mayaState() == MGlobal::kInteractive)
		{
			if (IsProgressCancelled())
				break;
		}

		Mesh& mesh = *m_meshes[i];
		if (!mesh.error)
		{
			for (uint j = 0; j < mesh.m_jobs.size(); j++)
			{
				ShellJob& job = (ShellJob&)(*mesh.m_jobs[j]);
				if (!job.error)
				{
					ColourFaces(mesh.dagPath, job.faceComponentObject, &mesh.useUVSetName);
				}
			}
		}
	}
}

void
UVAutoRatioPro::ColourFaces(const MDagPath& dagPath, MObject& component, const MString* uvSetName)
{
	MStatus status;

	// Build palette
	MColorArray colorPalette;
	{
		MColor startCol(0.0f, 0.0f, 1.0f, 1.0f);
		MColor endCol(0.0f, 1.0f, 0.0f, 1.0f);
		for (int i = 0; i < 32; i++)
		{
			float t = (float)i / 32.0f;
			MColor col = Lerp(startCol, endCol, t);
			colorPalette.append(col);
		}
	}
	{
		MColor startCol(0.0f, 1.0f, 0.0f, 1.0f);
		MColor endCol(1.0f, 0.0f, 0.0f, 1.0f);
		for (int i = 0; i < 32; i++)
		{
			float t = (float)i / 32.0f;
			MColor col = Lerp(startCol, endCol, t);
			colorPalette.append(col);
		}
	}	

	int faceCount = 0;
	double totalRatio = 0.0;
	MColorArray finalColors;
	MIntArray faceIndices;
	MDoubleArray faceRatios;

	MItMeshPolygon iter(dagPath, component, &status);
	finalColors.setLength(iter.count());
	faceIndices.setLength(iter.count());
	faceRatios.setLength(iter.count());
	for ( ; !iter.isDone(); iter.next() ) 
	{
		double area3d = 0.0;

		MPointArray points;
		MIntArray vertexList;
		status = iter.getTriangles(points, vertexList, MSpace::kWorld);
		int numTriangles = points.length() / 3;
		for (int i = 0; i < numTriangles; i++)
		{
			MPoint a, b, c;
			int indexA = (i*3+0);
			int indexB = (i*3+1);
			int indexC = (i*3+2);
			a = points[indexA];
			b = points[indexB];
			c = points[indexC];

			double la = a.distanceTo(b);
			double lb = a.distanceTo(c);
			double lc = b.distanceTo(c);
			la = MDistance::internalToUI(la);
			lb = MDistance::internalToUI(lb);
			lc = MDistance::internalToUI(lc);
			double areaSquared = GetTriangleAreaSquared(la, lb, lc);
			if (areaSquared > 0)
				area3d += sqrt(areaSquared);
		}

		double area2d = 0.0;

		MIntArray polygonToFaceVertexIndexMap;
		iter.getVertices(polygonToFaceVertexIndexMap);

		int numTrianglesInPoly=0;
		iter.numTriangles(numTrianglesInPoly);
		for (int j=0; j<numTrianglesInPoly; j++)
		{
			MPointArray			unTweakedTrianglePoints;
			MIntArray			meshVertexIndices;	// index's into the meshes vertex list!
			iter.getTriangle(j,unTweakedTrianglePoints,meshVertexIndices,MSpace::kObject); 

			// convert mesh-relative vertex index into polygon-relative
			int polygonVertexIndex[3];
			for (int k=0; k<3; k++)
			{
				for (uint kk=0; kk<polygonToFaceVertexIndexMap.length(); kk++)
				{
					if (polygonToFaceVertexIndexMap[kk]==meshVertexIndices[k])
					{
						polygonVertexIndex[k]=kk;
						break;
					}
				}
			}

			float2 uv1, uv2, uv3;
			iter.getUV(polygonVertexIndex[0], uv1, uvSetName);
			iter.getUV(polygonVertexIndex[1], uv2, uvSetName);
			iter.getUV(polygonVertexIndex[2], uv3, uvSetName);

			area2d += GetTriangleArea2D(uv1, uv2, uv3);
		}

		double ratio = 0.0;
		if (area2d > 0.0)
		{
			ratio = (area3d / area2d);
			totalRatio += ratio;
		}
		
		faceRatios[faceCount] = ratio;
		faceIndices[faceCount] = iter.index();
		faceCount++;
	}

	double averageRatio = totalRatio / (double)faceCount;
	double distance = averageRatio * 0.8;
	double minRatio = averageRatio - distance;
	double maxRatio = averageRatio + distance;

	for (int i = 0; i < faceCount; i++)
	{
		double ratio = faceRatios[i];
		ratio = max(min(ratio, maxRatio), minRatio);

		if (ratio > minRatio)
		{
			double t = (ratio - minRatio) / (maxRatio - minRatio);
			int paletteIndex = (int)(t * 63.0);

			finalColors[i] = colorPalette[paletteIndex];;
		}
		else
		{
			finalColors[i] = MColor(0.0f, 0.0f, 0.0f, 1.0f);
		}		
	}

	MFnMesh mesh(dagPath);
	mesh.setFaceColors(finalColors, faceIndices);
}

MStatus
UVAutoRatioPro::ProcessMeshes()
{
	MStatus status;

	if (m_params.m_isVerbose)
	{
		OutputText("");
		OutputText("UVAutoRatio 2.0 Pro: Building Task List...");
	}

	if (MGlobal::mayaState() == MGlobal::kInteractive)
	{
		MProgressWindow::reserve();
		MProgressWindow::setTitle("UVAutoRatio");
		MProgressWindow::setProgressMin(0);
		MProgressWindow::setProgressMax(6000);
		MProgressWindow::setInterruptable(true);
	}

	m_masterTimer.reset();

	MProgressWindow::startProgress();

	m_progressMessage = "Inspecting Selection...";
	SetProgress(0);
	status = BuildDataLists();

	if (status == MS::kSuccess)
	{
		// Count how many jobs there are to compute step size for progress meter
		{
			m_totalJobs = 0;
			for (uint i = 0; i < m_meshes.size(); i++)
			{
				Mesh& mesh = *m_meshes[i];
				for (uint j = 0; j < mesh.m_jobs.size(); j++)
				{
					m_totalJobs++;
				}
			}
		}

		// Gather Data
		if (!IsProgressCancelled())
		{
			if (m_params.m_isVerbose)
			{
				OutputText("UVAutoRatio 2.0 Pro: Gathering Data...");
			}
			m_progressMessage = "Gathering Data...";
			SetProgress(1000);

			GatherData();
		}

		// Find Scale
		if (!IsProgressCancelled())
		{
			if (m_params.m_isVerbose)
			{
				OutputText("UVAutoRatio 2.0 Pro: Calculating Scale...");
			}
			m_progressMessage = "Calculating Scale...";
			SetProgress(2000);

			if (!m_params.m_skipScaling)
			{
				FindScales();
			}
		}

		// Find non-overlapping positions
		if (!IsProgressCancelled())
		{
			if (m_params.m_layoutShells)// && m_params.m_operationMode == UVShellLevel)
			{
				if (m_params.m_isVerbose)
				{
					OutputText("UVAutoRatio 2.0 Pro: Solving Overlaps...");
				}
				m_progressMessage = "Solving Overlaps...";
				SetProgress(3000);

				LayoutShells();
			}
		}
		
		// Normalize
		if (!IsProgressCancelled())
		{
			if (m_params.m_normalise)
			{
				if (m_params.m_isVerbose)
				{
					OutputText("UVAutoRatio 2.0 Pro: Normalising...");
				}
				m_progressMessage = "Normalising...";
				SetProgress(4000);

				Normalise();
			}
		}

		// Apply
		if (!IsProgressCancelled())
		{
			if (m_params.m_isVerbose)
			{
				OutputText("UVAutoRatio 2.0 Pro: Applying...");
			}
			m_progressMessage = "Applying...";
			SetProgress(5000);

			ApplyScales();

			if (m_params.m_isColour)
			{
				TestColourFaces();
			}

			m_progressMessage = "Finito!";
			SetProgress(6000);
		}

	}
	m_totalTime = m_masterTimer.getTime();

	// Display Stats
	if (status == MS::kSuccess)
	{
		if (m_params.m_isVerbose)
		{
			// If the operation was canceled mark jobs not processed so they appear in the warning list
			if (IsProgressCancelled())
			{
				for (uint i = 0; i < m_meshes.size(); i++)
				{
					Mesh& mesh = *m_meshes[i];
					for (uint j = 0; j < mesh.m_jobs.size(); j++)
					{
						UVJob& job = *mesh.	m_jobs[j];
						if (!job.completed)
							job.error = SKIPPED;
					}
				}
			}
		}
		DisplayStats(!m_params.m_isVerbose);
		if (m_params.m_isShowTiming)
			DisplayTimingStats();
	}

	return status;
}

bool
UVAutoRatioPro::IsProgressCancelled()
{
	if (MGlobal::mayaState() != MGlobal::kInteractive)
	{
		return false;
	}

	return MProgressWindow::isCancelled();
}

void
UVAutoRatioPro::SetProgress(int value)
{
	if (MGlobal::mayaState() != MGlobal::kInteractive)
		return;

	MProgressWindow::setProgress(value);
	m_subTasks = 0;
	m_subTasksLeft = 0;
	m_progress = 0.0;
	UpdateProgress();
}

void
UVAutoRatioPro::UpdateProgress()
{
	if (MGlobal::mayaState() != MGlobal::kInteractive)
		return;

	// Update percentage display
	int pRange = MProgressWindow::progressMax() - MProgressWindow::progressMin();
	int percent = ((MProgressWindow::progress() - MProgressWindow::progressMin()) * 100) / pRange;
	MString message = "";
	message += percent;
	message += "%   ";
	message += m_progressMessage;
	if (m_subTasksLeft > 0)
	{
		message += m_subTasksLeft;
		message += "/";
		message += m_subTasks;
		message += " ";
		message += m_subTaskName;
	}
	MProgressWindow::setProgressStatus(message);
}

void
UVAutoRatioPro::SetNumSubTasks(int tasks, const MString& name)
{
	m_subTasks = tasks;
	m_subTasksLeft = 0;
	m_subTaskJump = (1000.0 / (double)tasks);
	m_subTaskName = name;
}

void
UVAutoRatioPro::StepJobProgress()
{
	m_subTasksLeft++;
	m_progress += m_subTaskJump;
	if (m_progress >= 1.0)
	{
		// Advance progress bar
		int step = (int)floor(m_progress);
		MProgressWindow::advanceProgress(step);
		m_progress -= (double)step;

		// Update percentage display
		UpdateProgress();
	}
}

MStatus
UVAutoRatioPro::BuildDataLists()
{
	// Build list of meshes/shells to process
	m_timer.reset();
	// Break down the selection into UVShell objects
	switch (m_params.m_operationMode)
	{
	case ObjectLevel:
		m_activeProcessor = new MeshProcessor();
		ProcessAsObjectLevel();
		break;

	case UVShellLevel:
		m_activeProcessor = new ShellProcessor();
		ProcessAsUVShellLevel();
		break;
	default:
		displayError(error_unknownOpMode);
		return MS::kFailure;
		break;
	}
	m_loadTime = m_timer.getTime();

	m_activeProcessor->m_undoHistory = &m_undos;
	m_activeProcessor->m_params = m_params;
	return MS::kSuccess;
}

void
UVAutoRatioPro::GatherData()
{
	m_timer.reset();
	for (uint i = 0; i < m_meshes.size(); i++)
	{
		if (IsProgressCancelled())
			break;

		Mesh& mesh = *m_meshes[i];
		mesh.Gather(*m_activeProcessor, m_params.m_isUVSetOverride, m_params.m_isFallback, m_params.m_UVSetName);
	}
	m_gatherTime = m_timer.getTime();
}

void
UVAutoRatioPro::FindScales()
{
	m_timer.reset();
	for (uint i = 0; i < m_meshes.size(); i++)
	{
		if (IsProgressCancelled())
			break;

		Mesh& mesh = *m_meshes[i];
		if (!mesh.error)
		{
			mesh.FindScale(*m_activeProcessor, m_params.m_goalRatio, m_params.m_threshold);
		}
	}
	m_processTime = m_timer.getTime();
}

void
UVAutoRatioPro::LayoutShells()
{
	m_timer.reset();

	UVSpringLayout* layout = new UVSpringLayout();

	//float progressBase = (float)MProgressWindow::progress();
	//float progressTotal = 1000.0f;
	//float progressMesh = progressTotal / (float)m_meshes.size();
	//float progressStep = iterations / 100.0f;

	float iterationSteps = m_params.m_layoutIterations / 100.0f;
	int numSubTasks = (int)(iterationSteps * m_meshes.size());
	UVAutoRatioPro::SetNumSubTasks(numSubTasks, "Jobs");

	enum OverlapFixMode
	{
		Global,
		PerMesh,
	};

	OverlapFixMode mode = Global;

	if (true)//mode == Global)
	{
		// Add boxes
		uint i = 0;
		for (i = 0; i < m_meshes.size(); i++)
		{
			if (IsProgressCancelled())
				break;

			Mesh& mesh = *m_meshes[i];
			if (!mesh.error)
			{
				// Add all the shells of a mesh to the layout class
				for (uint j = 0; j < mesh.m_jobs.size(); j++)
				{
					UVJob& job = (UVJob&)(*mesh.m_jobs[j]);
					if (!job.error)
					{
						double2 center;
						center[0] = job.centerU;
						center[1] = job.centerV;
						layout->AddBox(job.uvWidth * job.finalScaleX + m_params.m_layoutMinDistance, 
									   job.uvHeight * job.finalScaleY + m_params.m_layoutMinDistance, center);
					}
				}
			}
		}

		// Iterate!
		int itCount = 0;
		for (i =0; i < m_params.m_layoutIterations; i++)
		{
			if (!layout->Step(m_params.m_layoutStep))
				break;

			itCount++;
			if (itCount >= 100)
			{
				itCount = 0;
				UVAutoRatioPro::StepJobProgress();
				if (IsProgressCancelled())
					break;
			}
		}

		// Retrieve results
		int index = 0;
		for (i = 0; i < m_meshes.size(); i++)
		{
			if (IsProgressCancelled())
				break;

			Mesh& mesh = *m_meshes[i];
			if (!mesh.error)
			{
				for (uint j = 0; j < mesh.m_jobs.size(); j++)
				{
					UVJob& job = (UVJob&)(*mesh.m_jobs[j]);
					if (!job.error)
					{
						double2 position;
						layout->GetPosition(index, position);
						job.offsetU = position[0] - job.centerU;
						job.offsetV = position[1] - job.centerV;
						index++;
					}
				}
			}
		}
	}
	else
	{
		uint i = 0;
		for (i = 0; i < m_meshes.size(); i++)
		{
			if (IsProgressCancelled())
				break;

			Mesh& mesh = *m_meshes[i];
			if (!mesh.error)
			{
				uint j = 0;

				// Add all the shells of a mesh to the layout class
				for (j = 0; j < mesh.m_jobs.size(); j++)
				{
					UVJob& job = (UVJob&)(*mesh.m_jobs[j]);
					if (!job.error)
					{
						double2 center;
						center[0] = job.centerU;
						center[1] = job.centerV;
						layout->AddBox(job.uvWidth * job.finalScaleX + m_params.m_layoutMinDistance, job.uvHeight * job.finalScaleY + m_params.m_layoutMinDistance, center);
					}
				}

				// Iterate!
				int itCount = 0;
				for (j = 0; j < m_params.m_layoutIterations; j++)
				{
					if (!layout->Step(m_params.m_layoutStep))
						break;

					itCount++;
					if (itCount >= 100)
					{
						itCount = 0;
						UVAutoRatioPro::StepJobProgress();
						if (IsProgressCancelled())
							break;
					}
				}

				// Retrieve results
				int index = 0;
				for (j = 0; j < mesh.m_jobs.size(); j++)
				{
					UVJob& job = (UVJob&)(*mesh.m_jobs[j]);
					if (!job.error)
					{
						double2 position;
						layout->GetPosition(index, position);
						job.offsetU = position[0] - job.centerU;
						job.offsetV = position[1] - job.centerV;
						index++;
					}
				}

				layout->Clear();
			}
		}
	}


	delete layout;
	layout = NULL;

	m_layoutTime = m_timer.getTime();
}


void
UVAutoRatioPro::Normalise()
{
	m_timer.reset();

	double lowX, lowY;
	double highX, highY;
	lowX = lowY = DBL_MAX;
	highX = highY = -DBL_MAX;
	uint i = 0;

	// Find extents
	for (i = 0; i < m_meshes.size(); i++)
	{
		if (IsProgressCancelled())
			break;

		Mesh& mesh = *m_meshes[i];
		if (!mesh.error)
		{
			for (uint j = 0; j < mesh.m_jobs.size(); j++)
			{
				UVJob& job = (UVJob&)(*mesh.m_jobs[j]);
				if (!job.error)
				{
					lowX = Min(lowX, job.centerU - job.uvWidth * 0.5);
					lowY = Min(lowY, job.centerV - job.uvHeight * 0.5);
					highX = Max(highX, job.centerU + job.uvWidth * 0.5);
					highY = Max(highY, job.centerV + job.uvHeight * 0.5);
				}
			}
		}
	}

	double extentsX = highX - lowX;
	double extentsY = highY - lowY;
	double centerX = lowX + extentsX * 0.5;
	double centerY = lowY + extentsY * 0.5;

	// Set transform
	for (i = 0; i < m_meshes.size(); i++)
	{
		if (IsProgressCancelled())
			break;

		Mesh& mesh = *m_meshes[i];
		if (!mesh.error)
		{
			for (uint j = 0; j < mesh.m_jobs.size(); j++)
			{
				UVJob& job = (UVJob&)(*mesh.m_jobs[j]);
				if (!job.error)
				{
					if (m_params.m_normaliseKeepAspectRatio)
					{
						double majorExtent = Max(extentsX, extentsY);
						job.finalScaleX = 1.0 / majorExtent;
						job.finalScaleY = job.finalScaleX;
					}
					else
					{
						job.finalScaleX = 1.0 / extentsX;
						job.finalScaleY = 1.0 / extentsY;
					}
					job.centerU = lowX;
					job.centerV = lowY;
					job.offsetU = -lowX;
					job.offsetV = -lowY;
				}
			}
		}
	}


	if (m_params.m_isVerbose)
	{
		const char* extentsMessage = "Extents: %f, %f -> %f, %f  Center:(%f, %f)";
#ifdef WIN32
		sprintf_s(m_text, sizeof(m_text), extentsMessage, lowX, lowY, highX, highY, centerX, centerY);
#else
		sprintf(m_text,                   extentsMessage, lowX, lowY, highX, highY, centerX, centerY);
#endif
		OutputText(m_text);
	}

	float m_normaliseTime = m_timer.getTime();
}

void
UVAutoRatioPro::ApplyScales()
{
	m_timer.reset();
	for (uint i = 0; i < m_meshes.size(); i++)
	{
		if (IsProgressCancelled())
			break;

		Mesh& mesh = *m_meshes[i];
		if (!mesh.error)
		{
			mesh.ApplyScale(*m_activeProcessor);
		}
	}
	m_applyTime = m_timer.getTime();
}

const char*
UVAutoRatioPro::ErrorToString(JobError error) const
{
	const char* status = "Ok";
	switch (error)
	{
	case NO_UVSET_FOUND:
		status = "Mesh has no UV-Set, mesh skipped";
		break;
	case UVSET_NOTFOUND:
		status = "Specified UV-Set not found, mesh skipped";
		break;
	case RATIO_NOT_FOUND:
		status = "Ratio not found, mesh skipped";
		break;
	case INVALID_MESH:
		status = "Invalid mesh";
		break;
	case U_V_LISTS_DIFFERENT_LENGTHS:
		status = "Unequal length of U and V lists";
		break;
	case ITERATION_FAILED:
		status = "Iteration failed";
		break;
	case ZERO_TEXTURE_AREA:
		status = "Zero texture area";
		break;
	case ZERO_SURFACE_AREA:
		status = "Zero surface area";
		break;
	case SKIPPED:
		status = "Skipped, User Aborted";
		break;
	default:
	case OK:
		break;
	}

	return status;
}

void
UVAutoRatioPro::DisplayStats(bool showErrorsOnly)
{
	for (uint j = 0; j < m_meshes.size(); j++)
	{
		Mesh& mesh = *m_meshes[j];

		// check if any errors in the jobs of this mesh
		bool jobErrors = false;
		for (uint i = 0; i < mesh.m_jobs.size(); i++)
		{
			UVJob& job = *mesh.m_jobs[i];

			if (job.error)
			{
				jobErrors = true;
				break;
			}
		}

		if ((showErrorsOnly && jobErrors) || !showErrorsOnly)
		{
			const char* meshstatus = ErrorToString(mesh.error);

			const char* meshmessage = "  Mesh%2i - %s : %s on uvset '%s'";
	#ifdef WIN32
			sprintf_s(m_text, sizeof(m_text), meshmessage, j, meshstatus, mesh.name.asChar(), mesh.useUVSetName.asChar());
	#else
			sprintf(m_text,                   meshmessage, j, meshstatus, mesh.name.asChar(), mesh.useUVSetName.asChar());
	#endif
			OutputText(m_text);

			for (uint i = 0; i < mesh.m_jobs.size(); i++)
			{
				UVJob& job = *mesh.m_jobs[i];

				if ((showErrorsOnly && job.error) || !showErrorsOnly)
				{
					double initialRatio = job.surfaceArea / job.textureArea;
					double finalRatio = job.surfaceArea / job.finalTextureArea;

					const char* status = ErrorToString(job.error);

					if (showErrorsOnly)
					{
						if (job.error)
						{
							const char* message = "    Job%2i - %s : %s";
#ifdef WIN32
							sprintf_s(m_text, sizeof(m_text), message, i, status, job.GetName().asChar());
#else
							sprintf(m_text,                   message, i, status, job.GetName().asChar());
#endif
							MGlobal::displayError(m_text);
						}
					}
					else
					{
						const char* message = "    Job%2i - %s : %s  PolyArea: %8.8f  UVArea: (%.12f -> %.12f) Scale: %.12f  Ratio: (%.12f -> %.12f)  (after %i iterations)  bounds:(%.12f %.12f) center:(%8.8f %8.8f)  offset:(%8.8f, %8.8f)";
#ifdef WIN32
						sprintf_s(m_text, sizeof(m_text), message, i, status, job.GetName().asChar(), job.surfaceArea, job.textureArea, job.finalTextureArea, job.finalScaleX, initialRatio, finalRatio, job.iterationsPerformed, job.uvWidth, job.uvHeight, job.centerU, job.centerV, job.offsetU, job.offsetV);
#else
						sprintf(m_text,                   message, i, status, job.GetName().asChar(), job.surfaceArea, job.textureArea, job.finalTextureArea, job.finalScaleX, initialRatio, finalRatio, job.iterationsPerformed, job.uvWidth, job.uvHeight, job.centerU, job.centerV, job.offsetU, job.offsetV);
#endif

						OutputText(m_text);
					}
				}
			}
		}
	}
}

void
UVAutoRatioPro::DisplayTimingStats()
{
	const char* message = "UVAR: %i meshes, %i jobs:  LoadTime: %.2fms    GatherTime: %.2fms    ProcessTime: %.2fms    LayoutTime: %.2fms    ApplyTime: %.2fms    TotalTime: %.2fms";

#ifdef WIN32
	sprintf_s(m_text, sizeof(m_text), message, (int)m_meshes.size(), m_totalJobs, m_loadTime,m_gatherTime, m_processTime, m_layoutTime, m_applyTime, m_totalTime);
#else
	sprintf(m_text,                   message, (int)m_meshes.size(), m_totalJobs, m_loadTime,m_gatherTime, m_processTime, m_layoutTime, m_applyTime, m_totalTime);
#endif
	OutputText(m_text);
}

// Build UVShell objects from the selection
// We need to find the unique uv shells for each selection element
//
// 1)find which uv shells we want to work with:
// a) find each unique mesh in the selection
// b) get the meshes uv shell faces
// c) go through each element in the selection and see which uv shell it's in, for that mesh
//    flag that uv shell as active
//    might have to convert selection to faces, or uv's
// 2) get the components of that uv shell
//
// 1) For each mesh of X components
//  a) convert itself to either FACES or UV's - not sure which, need to check old source code
//  b) collect the shell's in this mesh that contain one of these faces/uv's element
//  c) convert shells into components and add it
//
// For each mesh in the selection:
//    1) from the selected components
//    2) see which uv shells the components are included in..
//    3) take the whole of these uv shells.
//
// For all selected meshes, create uv shell data
// Now go through selection components and see which uv shells to keep, once we decide to keep a shell, don't need to compare against it anymore.
// Vector of ints.. which shells and which mesh
void
UVAutoRatioPro::ProcessAsUVShellLevel()
{
	MStatus status;

	std::vector<ValidMesh*>	potentialMeshes;
	potentialMeshes.reserve(512);

	// Build up a list of all the unique meshes in the selection
	MItSelectionList iter(m_savedSelection);
	for ( ; !iter.isDone(); iter.next() ) 
	{
		if (MGlobal::mayaState() == MGlobal::kInteractive)
		{
			if (IsProgressCancelled())
				break;
		}

		MDagPath	dagPath;
		MObject		component;
		iter.getDagPath( dagPath, component );
		if (dagPath.node().hasFn(MFn::kTransform) || dagPath.node().hasFn(MFn::kPolyMesh) || dagPath.node().hasFn(MFn::kMesh))
		{
			// Try extend to a mesh
			dagPath.extendToShape();
			MFnMesh mesh(dagPath, &status);
			if (status == MStatus::kSuccess)
			{
				ValidMesh* vm = NULL;
				for (unsigned int i = 0; i < potentialMeshes.size(); i++)
				{
					MDagPath path = potentialMeshes[i]->dagPath;
					if ((path.fullPathName() == dagPath.fullPathName()))
					{
						vm = potentialMeshes[i];
						break;
					}
				}
				// Add to the list if not already contained
				if (NULL == vm)
				{
					ValidMesh* vm = new ValidMesh;
					vm->dagPath = dagPath;
					vm->components.push_back(component);
					potentialMeshes.push_back(vm);
				}
				else
				{
					vm->components.push_back(component);
				}
			}
		}
	}

/*	MString* desiredUVSetName = NULL;
	if (m_params.m_isUVSetOverride)
	{
		desiredUVSetName = &(m_params.m_UVSetName);
	}*/

	// For each mesh, collect UV shells that are valid
	for (unsigned int i = 0; i < potentialMeshes.size(); i++)
	{
		if (MGlobal::mayaState() == MGlobal::kInteractive)
		{
			if (IsProgressCancelled())
				break;
		}

		MDagPath dagPath = potentialMeshes[i]->dagPath;
		dagPath.extendToShape();
		MFnMesh mesh(dagPath);

		// Find the uvset we can use for this mesh
		UVSetResult uvSetResult = FindUVSet(mesh, m_params.m_isUVSetOverride, m_params.m_isFallback, m_params.m_UVSetName);
		if (uvSetResult == NONE)
		{
			// this mesh has no desired uv set, and it can be skipped
			continue;
		}

		MString currentUVSetName = GetCurrentUVSetName(mesh);

		if (uvSetResult == OVERRIDE)
		{
			mesh.setCurrentUVSetName(m_params.m_UVSetName);
		}

		// Get the UV shell information
		MIntArray uvShellIDs;
		unsigned int numShells;

		// it would appear that maya has a bug with this function,
		// in that it ignores the uvset you pass in, and instead
		// uses the current uvset of the mesh.
		// So for now we'll just manually set and unset the uvset
		// to current before calling this function
		status = mesh.getUvShellsIds(uvShellIDs, numShells);

		// Go through components in the selection finding which UV shell they are in
		for (unsigned int j = 0; j < potentialMeshes[i]->components.size(); j++)
		{
			if (MGlobal::mayaState() == MGlobal::kInteractive)
			{
				if (IsProgressCancelled())
					break;
			}

			MObject component = potentialMeshes[i]->components[j];

			// Special case for null component, this happens when the entire mesh is selected
			if (MObject::kNullObj == component)
			{
				// Add all the shells, clear any previously added ones
				potentialMeshes[i]->validShells.clear();
				for (unsigned int k = 0; k < numShells; k++)
				{
					potentialMeshes[i]->validShells.push_back(k);
				}
				// Since we have added all the uv shells, stop processing this mesh
				break;
			}
			else
			{
				MObject uvComponent = MObject::kNullObj;
				// Special case for if the selection is already made of UV's
				// because we don't have to convert the selection
				if (MFn::kMeshMapComponent == component.apiType())
				{
					uvComponent = component;
				}
				else
				{
					// select the components
					status = MGlobal::clearSelectionList();
					status = MGlobal::select(dagPath, component);

					MStringArray selections;
					MGlobal::executeCommand("polyListComponentConversion -fromVertexFace -fromEdge -fromFace -fromVertex -toUV -internal;", selections);

					// Now try to get the component from the current selection
					MSelectionList activeList;
					for (unsigned int i = 0; i < selections.length(); i++)
					{
						activeList.add(selections[i]);
					}

					// I think this assert is wrong
					// because a single face component can become multiple uv components

					// We should only have 1 valid in the list, or 0 if it failed
					assert(activeList.length() < 2);
					if (activeList.length() == 1)
					{
						MDagPath tempDagPath;
						activeList.getDagPath(0, tempDagPath, uvComponent);
					}
					else
					{
						displayError(error_componentConversionError);
					}
				}

				// If the conversion to UV components went ok, process them
				if (uvComponent != MObject::kNullObj)
				{
					MFnSingleIndexedComponent uvComponents(uvComponent);
					int numUVs = uvComponents.elementCount();
					for (int k = 0; k < numUVs; k++)
					{
						int uvIndex = uvComponents.element(k);
						assert((unsigned int)uvIndex < uvShellIDs.length());
						int shellIndex = uvShellIDs[uvIndex];
						if (!potentialMeshes[i]->HasShell(shellIndex) && shellIndex >= 0)
						{
							potentialMeshes[i]->validShells.push_back(shellIndex);
						}
					}

				}
			}
		}

		//UVMesh* uvMesh = NULL;
		if (potentialMeshes[i]->validShells.size() > 0)
		{
			//uvMesh = new UVMesh(dagPath, desiredUVSetName);
			//m_uvMeshes.push_back(uvMesh);

			Mesh* mesh = new Mesh();
			mesh->dagPath = dagPath;
			m_meshes.push_back(mesh);

			// Add all the valid shells
			for (unsigned int j = 0; j < potentialMeshes[i]->validShells.size(); j++)
			{
				if (MGlobal::mayaState() == MGlobal::kInteractive)
				{
					if (IsProgressCancelled())
						break;
				}

				MDagPath meshDagPath = potentialMeshes[i]->dagPath;
				int shellIndex = potentialMeshes[i]->validShells[j];

				MFnSingleIndexedComponent uvComponents;
				MObject uvComponentObject;
				uvComponentObject = uvComponents.create(MFn::kMeshMapComponent, &status);

				// collect UV indices
				for (unsigned int j = 0; j < uvShellIDs.length(); j++)
				{
					// if the shell ID matches the current index, add the corresponding indices
					if (shellIndex == uvShellIDs[j])
					{
						uvComponents.addElement( j );
					}
				}

				if (uvComponents.elementCount() > 0)
				{
					//UVShell* shell = new UVShell(uvMesh, uvComponentObject, desiredUVSetName);
					//m_uvShells.push_back(shell);

					ShellJob* job = new ShellJob();
					job->mesh = mesh;
					job->meshShellNumber = shellIndex;
					job->uvComponentObject = uvComponentObject;
					mesh->m_jobs.push_back(job);
				}
			}
		}

		// Restore the uvset
		if (uvSetResult == OVERRIDE)
		{
			mesh.setCurrentUVSetName(currentUVSetName);
		}
	}

	// Delete all validmeshes
	{
		std::vector<ValidMesh*>::reverse_iterator riter;
		for ( riter = potentialMeshes.rbegin(); riter != potentialMeshes.rend(); ++riter )
		{
			delete (*riter);
		}
		potentialMeshes.clear();
	}
}


void
UVAutoRatioPro::ProcessAsObjectLevel()
{
	MStatus status;

	// add unique meshes to a list
	MItSelectionList iter(m_savedSelection);
	for ( ; !iter.isDone(); iter.next() ) 
	{
		if (MGlobal::mayaState() == MGlobal::kInteractive)
		{
			if (IsProgressCancelled())
				break;
		}

		MDagPath	dagPath;
		MObject		component;
		iter.getDagPath( dagPath, component );

		// check node type
		if (dagPath.node().hasFn(MFn::kTransform) || dagPath.node().hasFn(MFn::kPolyMesh) || dagPath.node().hasFn(MFn::kMesh))
		{
			dagPath.extendToShape();

			// check for uniqueness
			bool unique = true;
			for (uint i = 0; i < m_meshes.size(); i++)
			{
				if (m_meshes[i]->dagPath == dagPath || m_meshes[i]->dagPath.fullPathName() == dagPath.fullPathName())
				{
					unique = false;
					break;
				}
			}

			// add to list
			if (unique)
			{
				Mesh* mesh = new Mesh;
				mesh->dagPath = dagPath;
				m_meshes.push_back(mesh);

				MeshJob* job = new MeshJob;
				job->mesh = mesh;
				mesh->m_jobs.push_back(job);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

