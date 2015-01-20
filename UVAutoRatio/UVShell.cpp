// Copyright � 2007 RenderHeads Ltd.  All Rights Reserved.
// Author: Andrew David Griffiths

#define MNoVersionString
#include "MayaPCH.h"
#include "Utility.h"
#include "UVMesh.h"
#include "UVShell.h"

UVShell::UVShell(UVMesh* mesh, MObject uvComponents, const MString* desiredUVSet)
{
	m_surfaceArea = 0.0;
	m_uvArea = 0.0;
	m_finalScale = 1.0;
	m_centerU = 0.0;
	m_centerV = 0.0;
	m_uvSetName = desiredUVSet;

	m_uvComponentObject = uvComponents;
	m_isComponents = !(m_uvComponentObject == MObject::kNullObj);
	m_uvComponents = NULL;

	m_mesh = mesh;

	m_faceComponentObject = MObject::kNullObj;

	CollectData();
}

UVShell::~UVShell()
{
	if (m_uvComponents)
	{
		delete m_uvComponents;
		m_uvComponents = NULL;
	}
}

bool
UVShell::Is(MDagPath meshDagPath, MObject uvComponents) const
{
	return (m_mesh->GetPath() == meshDagPath) && (m_mesh->GetPath().fullPathName() == meshDagPath.fullPathName()) && (m_uvComponentObject == uvComponents);
}

void
UVShell::CollectData()
{
	MStatus status;

	// Get surface and UV area
	if (m_isComponents)
	{
		m_uvComponents = new MFnSingleIndexedComponent(m_uvComponentObject);
		GetUVIndices();

		// Get face components from UV components
		UVToFaceComponents(m_mesh->GetPath(), m_uvComponentObject, m_faceComponentObject);

		m_uvArea = GetAreaFacesUV(m_mesh->GetPath(), m_faceComponentObject, m_uvSetName);
		m_surfaceArea = GetAreaFacesSurface(m_mesh->GetPath(), m_faceComponentObject, true);
	}
	else
	{
		m_uvArea = GetAreaMeshUV(m_mesh->GetPath(), m_uvSetName);
		m_surfaceArea = GetAreaMeshSurface(m_mesh->GetPath(), true);
	}

	// Get center
	FindUVCenter();
}

void
UVShell::GetUVIndices()
{
	MStatus status;

	// TODO: perhaps indices can be a static array instead of an stl vector for improved performance
	int numIndices = m_uvComponents->elementCount();
	m_uvIndices.reserve(numIndices);
	for (int i = 0; i < numIndices; i++)
	{
		int index = m_uvComponents->element(i);
		m_uvIndices.push_back(index);
	}

/*
	// Get the UV Shells
	MIntArray uvShellIDs;
	unsigned int numShells;
	status = m_mesh.getUvShellsIds(uvShellIDs, numShells, m_uvSetName);

	for (int i = 0; i < numShells; i++)
	{
		// collect UV indices
		for (int j = 0; j < uvShellIDs.length(); j++)
		{
			// if the shell ID matches the current index, add the corresponding indices
			if (i == uvShellIDs[j])
			{
				m_uvIndices.push_back(j);
			}
		}
	}*/
}

void
UVShell::FindUVCenter()
{
	/*const MFloatArray& uArray = m_mesh->UArray();
	const MFloatArray& vArray = m_mesh->VArray();
	assert(uArray.length() == vArray.length());

	if (m_isComponents)
	{
		// if components are used, use the lookup table to get the relevant UV's
		m_centerU = GetCenterValue(uArray, m_uvIndices);
		m_centerV = GetCenterValue(vArray, m_uvIndices);
	}
	else
	{
		// slightly faster alternative if the whole mesh is used
		m_centerU = GetCenterValue(uArray);
		m_centerV = GetCenterValue(vArray);
	}*/
}

// Scales the mesh UV's by a factor
// Note: if the mesh has history that modifies the UV's, this operation will be overridden
// once the DAG updates
// It's slightly faster to not scale about the center so we can use this to quickly do temporary scales
// to calculate area
void
UVShell::ScaleUVs(double scale, bool aboutCenter)
{
	MStatus status;

	/*status = m_mesh->GetMesh()->setUVs(m_mesh->UArray(), m_mesh->VArray(), m_uvSetName);

	status = MGlobal::clearSelectionList();
	status = MGlobal::setSelectionMode(MGlobal::kSelectObjectMode);
	status = MGlobal::select(m_mesh->GetPath(), MObject::kNullObj);
	SelectUVSet(NULL, *m_uvSetName);*/

	// copy original UV's
	MFloatArray& uArrayOrig = m_mesh->UArray();
	MFloatArray& vArrayOrig = m_mesh->VArray();
	assert(uArrayOrig.length() == vArrayOrig.length());

	int numUVs = m_mesh->GetMesh()->numUVs(*m_uvSetName);
	assert(uArrayOrig.length() == numUVs);

	MFloatArray uArray, vArray;
	uArray.copy(uArrayOrig);
	vArray.copy(vArrayOrig);

	// scale UV's
	if (!aboutCenter)
	{
		if (m_isComponents)
		{
			uint numSamples = m_uvIndices.size();
			for (uint i = 0; i < numSamples; i++)
			{
				unsigned int index = m_uvIndices[i];
				assert(index < uArray.length());
				uArray[index] *= (float)scale;
				vArray[index] *= (float)scale;
			}
		}
		else
		{
			uint numSamples = uArray.length();
			for (uint i = 0; i < numSamples; i++)
			{
				uArray[i] *= (float)scale;
				vArray[i] *= (float)scale;
			}
		}
	}
	else
	{
		if (m_isComponents)
		{
			uint numSamples = m_uvIndices.size();
			for (uint i = 0; i < numSamples; i++)
			{
				unsigned int index = m_uvIndices[i];
				assert(index < uArray.length());
				// cast to double for more accurate operations
				double u = uArray[index];
				double v = vArray[index];
				u -= m_centerU;
				v -= m_centerV;
				u *= scale;
				v *= scale;
				u += m_centerU;
				v += m_centerV;
				uArray[index] = (float)u;
				vArray[index] = (float)v;
			}
		}
		else
		{
			uint numSamples = uArray.length();
			for (uint i = 0; i < numSamples; i++)
			{
				// cast to double for more accurate operations
				double u = uArray[i];
				double v = vArray[i];
				u -= m_centerU;
				v -= m_centerV;
				u *= scale;
				v *= scale;
				u += m_centerU;
				v += m_centerV;
				uArray[i] = (float)u;
				vArray[i] = (float)v;
			}
		}
	}

	// set mesh UV's
	status = m_mesh->GetMesh()->setUVs(uArray, vArray, m_uvSetName);
}

void
UVShell::SelectUVs(std::vector<MDGModifier*>* history, const MString& uvSetName)
{
	MStatus status;

	// Select mesh UVs
	// WARNING: This may FAIL in the old version, must make sure the selection type is correct for both!!
	status = MGlobal::clearSelectionList();
	status = MGlobal::setSelectionMode(MGlobal::kSelectObjectMode);
	if (m_isComponents)
	{
		status = MGlobal::select(m_mesh->GetPath(), m_uvComponentObject);
	}
	else
	{
		status = MGlobal::select(m_mesh->GetPath(), MObject::kNullObj);
	}


	static char text[2048];
	const char* command = "polyUVSet -currentUVSet -uvSet \"%s\"";
#ifdef WIN32
	sprintf_s(text, sizeof(text), command, uvSetName.asChar());
#else
	sprintf(text, command, uvSetName.asChar());
#endif

	if (history != NULL)
	{
		MDGModifier * modifier = new MDGModifier;
		history->push_back(modifier);
		status = modifier->commandToExecute(text);
		status = modifier->doIt();
	}
	else
	{
		MGlobal::executeCommand(text);
	}


	if (!m_isComponents)
	{
		// Convert selection to UVs
		if (history != NULL)
		{
			MDGModifier * modifier = new MDGModifier;
			history->push_back(modifier);
			status = modifier->commandToExecute("ConvertSelectionToUVs;");
			status = modifier->doIt();
		}
		else
		{
			MGlobal::executeCommand("ConvertSelectionToUVs;");
		}
	}
}

void
UVShell::ScaleUV(double scale, std::vector<MDGModifier*>* history)
{
	MStatus status;

	// Get the name of the UV set to operate on
	MString uvSetName;
	if (m_uvSetName != NULL)
	{
		uvSetName = *m_uvSetName;
	}
	else
	{
		m_mesh->GetMesh()->getCurrentUVSetName(uvSetName);
	}

	SelectUVs(history, uvSetName);

	// Execute UV scale MEL command
	static char text[2048];
	const char* command = "polyEditUV -relative true -pivotU %.20f -pivotV %.20f -scale true -scaleU %.20f -scaleV %.20f";
#ifdef WIN32
	sprintf_s(text, sizeof(text), command, m_centerU, m_centerV, scale, scale);
#else
	sprintf(text, command, m_centerU, m_centerV, scale, scale);
#endif
	if (history != NULL)
	{
		MDGModifier * modifier = new MDGModifier;
		history->push_back(modifier);
		status = modifier->commandToExecute(text);
		status = modifier->doIt();
	}
	else
	{
		MGlobal::executeCommand(text);
	}
}

bool
UVShell::CalculateScaleForRatio(double goalRatio, int& numIterations, double& finalScale, double threshold, int maxIterations)
{
	bool result = true;

	double minScale = 0.0001;
	double maxScale = 10000.0;
	double scale = 1.0;

	//SelectUVSet(NULL, *m_uvSetName);

	// iterate until UV area is within threshold of the goal
	numIterations = 0;
	while (numIterations < maxIterations)
	{
		// Pick a midpoint between the scale ranges
		scale = minScale + (maxScale - minScale) * 0.5;

		// Calculate new ratio
		double ratio = GetRatioAtUVScale(scale);

		// If it is within threshold, leave it and return
		if (fabs(ratio - goalRatio) < threshold)
		{
			break;
		}
		else
		{
			// Converge the min and max scale range
			if (ratio < goalRatio)
			{
				maxScale = scale;
			}
			else
			{
				minScale = scale;
			}

			// Check for impossible situation
			if (minScale == maxScale)
			{
				MGlobal::displayError("FATAL ERROR, ratio not found");
				// Exit here, otherwise this function will never exit
				result = false;
				break;
			}
		}
		numIterations++;
	}

	finalScale = scale;
	m_finalScale = scale;

	// Restore UV scale
	ScaleUVs(1.0, false);

	return result;
}

// Scale the UV's and compute the new ratio of surface to uv area
double
UVShell::GetRatioAtUVScale(double scale)
{
	// Scale the UV's
	ScaleUVs(scale, false);

	// Compute the new UV Area
	double newUVArea = 0.0;
	if (m_isComponents)
	{
		newUVArea = GetAreaFacesUV(m_mesh->GetPath(), m_faceComponentObject, m_uvSetName);
	}
	else
	{
		newUVArea = GetAreaMeshUV(m_mesh->GetPath(), m_uvSetName);
	}

	// Calculate ratio
	double ratio = m_surfaceArea / newUVArea;

	return ratio;
}

