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
#include "Utility.h"
#include "Timer.h"
#include "UVAutoRatioPro.h"
#include "ShellProcessor.h"

// Scales the mesh UV's by a factor
// Note: if the mesh has history that modifies the UV's, this operation will be overridden
// once the DAG updates
// It's slightly faster to not scale about the center so we can use this to quickly do temporary scales
// to calculate area
void
ShellProcessor::ScaleMeshUVs(ShellJob& job, double scale)
{
	MStatus status;

	// copy original UV's
	MFloatArray uArray, vArray;
	uArray.copy(job.mesh->uArray);
	vArray.copy(job.mesh->vArray);
	assert(uArray.length() == vArray.length());
	int i = 0;

	// scale UV's
	//if (!aboutCenter)
	{		
		switch (m_params.m_scalingAxis)
		{
		case Both:
			for (i = 0; i < job.numIndices; i++)
			{
				unsigned int index = job.uvIndices[i];
				assert(index < uArray.length());

				uArray[index] *= (float)scale;
				vArray[index] *= (float)scale;
			}
			break;
		case Horizontal:
			for (i = 0; i < job.numIndices; i++)
			{
				unsigned int index = job.uvIndices[i];
				assert(index < uArray.length());

				uArray[index] *= (float)scale;
			}
			break;
		case Vertical:
			for (i = 0; i < job.numIndices; i++)
			{
				unsigned int index = job.uvIndices[i];
				assert(index < uArray.length());

				vArray[index] *= (float)scale;
			}
			break;
		}
	}
	/*
	else
	{
		for (i = 0; i < job.numIndices; i++)
		{
			unsigned int index = job.uvIndices[i];
			assert(index < uArray.length());

			// cast to double for more accurate operations
			double u = uArray[index];
			double v = vArray[index];
			u -= job.centerU;
			v -= job.centerV;
			u *= scale;
			v *= scale;
			u += job.centerU;
			v += job.centerV;
			uArray[index] = (float)u;
			vArray[index] = (float)v;
		}
	}*/

	// set mesh UV's
	status = job.mesh->model->setUVs(uArray, vArray, &job.mesh->useUVSetName);
}


JobError
ShellProcessor::FindScale(ShellJob& job, double threshold, double& finalScale, double& finalTextureArea, int& iterationsPerformed)
{
	MStatus status;

	JobError result = OK;

	double minScale, maxScale;
	double scale = 1.0;

	double startRatio = job.surfaceArea / job.textureArea;

	if (startRatio < m_params.m_goalRatio)
	{
		// we need to scale uv's DOWN
		minScale = 0.00001;
		maxScale = 1.0;
	}
	else
	{
		// we need to scale uv's UP
		minScale = 1.0;
		maxScale = 100000.0;
	}

	// iterate until UV area is within the threshold of the goal
	int numIterations = 0;
	while (numIterations < m_params.m_maxIterations)
	{
		// Pick a midpoint between the scale ranges
		scale = minScale + (maxScale - minScale) * 0.5;

		// Scale the UV's
		ScaleMeshUVs(job, scale);

		// Read the new UV Area
		double newUVArea = GetAreaFacesUV(job.mesh->dagPath, job.faceComponentObject, &job.mesh->useUVSetName);

		double ratio = job.surfaceArea / newUVArea;

		// If it is within threshold, leave it and return
		if (fabs(ratio - m_params.m_goalRatio) < threshold)
		{
			finalScale = scale;
			finalTextureArea = newUVArea;
			break;
		}

		// Converge the min and max scale range
		if (ratio < m_params.m_goalRatio)
		{
			maxScale = scale;
		}
		else
		{
			minScale = scale;
		}

		// Check for impossible situation, perhaps change this to < threshold check
		if (minScale == maxScale)
		{
			// Exit here, otherwise this function will never exit
			result = ITERATION_FAILED;
			break;
		}

		numIterations++;
	}

	if (numIterations >= m_params.m_maxIterations)
	{
		result = RATIO_NOT_FOUND;
	}

	iterationsPerformed = numIterations;

	return result;
}

double
ShellProcessor::FindScaleFactor(double shapeArea, double targetArea, double width, double height) const
{
	double rectArea = width * height;
	double areaRatio = rectArea / shapeArea;      // how many times bigger the square is compared to the shape, and it'll ALWAYS be bigger

	double targetSquareArea = targetArea * areaRatio;
	double rectRatio = width / height;
	double targetHeight = sqrt(targetSquareArea / rectRatio);
	//double targetWidth = targetSquareArea / targetHeight;

	return targetHeight / height;
}

double
roundPrecision(double v,const double precision)
{
	return ((double)(int((v*precision)+0.5))) / precision;
}

void
ShellProcessor::FindScale(UVJob& uvjob)
{
	ShellJob& job = (ShellJob&)uvjob;

	bool fast = true;

	if (m_params.m_scalingAxis != Both)
		fast = false;

	// try to quickly find the scale
	if (fast)
	{
		double targetTextureArea = job.surfaceArea / m_params.m_goalRatio;
		double scale = FindScaleFactor(job.textureArea, targetTextureArea, job.uvWidth, job.uvHeight);
		//scale=roundPrecision(scale, 10000.0);
		job.finalScaleX = scale;
		job.finalScaleY = scale;
		job.finalTextureArea = targetTextureArea;
		job.iterationsPerformed = 0;

		/*
		// Scale the UV's
		ScaleMeshUVs(job, scale);
		//ApplyScale(job);

		// Read the new UV Area
		double newUVArea = GetAreaFacesUV(job.mesh->dagPath, job.faceComponentObject, &job.mesh->useUVSetName);
		double ratio = job.surfaceArea / newUVArea;

		static char text[1024];
		sprintf_s(text, sizeof(text), "RESULTS: Scale:%.25f  Area:%.12f,%.12f  Ratio:%.12f   %.12f", scale, newUVArea, targetTextureArea, ratio, fabs(ratio - m_params.m_goalRatio));
		MGlobal::displayInfo(text);

		// Scale the UV's back to original
		job.mesh->ResetUVs();*/
	}
	// if it's not accurate enough, iterate to find it
	else
	{
		//double threshold = m_params.m_threshold;
		int iterationsPerformed, totalIterations = 0;
		double finalScale, finalTextureArea;

		JobError error = ITERATION_FAILED;
		double power = -4.0;

		while (error != OK && power < 4.0)
		{
			double threshold = pow(10, power);
			error = FindScale(job, threshold, finalScale, finalTextureArea, iterationsPerformed);

			power += 1.0;
			totalIterations += iterationsPerformed;
		}

		if (error == OK)
		{
			job.finalScaleX = finalScale;
			job.finalScaleY = finalScale;
			job.finalTextureArea = finalTextureArea;
		}
		else
		{
			job.error = error;
			job.finalScaleX = 1.0;
			job.finalScaleY = 1.0;
			job.finalTextureArea = 1.0;
		}

		job.iterationsPerformed = totalIterations;

		// Scale the UV's back to original
		job.mesh->ResetUVs();
	}
}

void
ShellProcessor::Gather(UVJob& uvjob)
{
	MStatus status;

	ShellJob& job = (ShellJob&)uvjob;

	job.uvComponents = new MFnSingleIndexedComponent(job.uvComponentObject);

	// Get UV indices
	{
		job.numIndices = job.uvComponents->elementCount();
		job.uvIndices = new int[job.numIndices];
		for (int i = 0; i < job.numIndices; i++)
		{
			int index = job.uvComponents->element(i);
			job.uvIndices[i] = index;
		}
	}

	// Get face components from UV components
	UVToFaceComponents(job.mesh->dagPath, job.uvComponentObject, job.faceComponentObject);

	// Get area
	job.textureArea = GetAreaFacesUV(job.mesh->dagPath, job.faceComponentObject, &job.mesh->useUVSetName);
	job.finalTextureArea = job.textureArea;
	if (job.textureArea == 0.0)
	{
		job.error = ZERO_TEXTURE_AREA;
		return;
	}

	job.surfaceArea = GetAreaFacesSurface(job.mesh->dagPath, job.faceComponentObject, true);
	if (job.surfaceArea == 0.0)
	{
		job.error = ZERO_SURFACE_AREA;
		return;
	}

	// Get the UV's
	status = job.mesh->model->getUVs(job.mesh->uArray, job.mesh->vArray, &job.mesh->useUVSetName);
	assert(job.mesh->uArray.length() == job.mesh->vArray.length());
	if (job.mesh->uArray.length() != job.mesh->vArray.length())
	{
		job.error = U_V_LISTS_DIFFERENT_LENGTHS;
		return;
	}

	// Find the UV center, width & height
	double minU, maxU, minV, maxV;
	GetMinMaxValues(job.mesh->uArray, job.uvIndices, job.numIndices, minU, maxU);
	GetMinMaxValues(job.mesh->vArray, job.uvIndices, job.numIndices, minV, maxV);
	job.centerU = (minU + maxU) * 0.5;
	job.centerV = (minV + maxV) * 0.5;
	job.uvWidth = maxU - minU;
	job.uvHeight = maxV - minV;
}

void
ShellProcessor::ApplyScale(UVJob& uvjob)
{
	MStatus status;

	ShellJob& job = (ShellJob&)uvjob;

	// Select mesh object
	// WARNING: This may FAIL in the old version, must make sure the selection type is correct for both!!
	// TODO: couldn't i just select the uvComponents????
	status = MGlobal::clearSelectionList();
	status = MGlobal::setSelectionMode(MGlobal::kSelectComponentMode);
	status = MGlobal::select(job.mesh->dagPath, job.faceComponentObject);
	status = MGlobal::executeCommand("ConvertSelectionToUVs;");

	if (UVAutoRatioPro::IsProgressCancelled())
		return;

	// Execute UV scale MEL command
	//const char* command = "polyEditUV -relative true -pivotU %.20f -pivotV %.20f -scale true -scaleU %.20f -scaleV %.20f -uValue %.20f -vValue %.20f";
	const char* command = "polyMoveUV -pivot %.20f %.20f -scale %.20f %.20f -translate %.20f %.20f";

	double scaleU = job.finalScaleX;
	double scaleV = job.finalScaleY;
	switch (m_params.m_scalingAxis)
	{
	case Both:
		break;
	case Horizontal:
		scaleV = 1.0;
		break;
	case Vertical:
		scaleU = 1.0;
		break;
	}

	static char text[2048];
#ifdef WIN32
	sprintf_s(text, sizeof(text), command, job.centerU, job.centerV, scaleU, scaleV, job.offsetU, job.offsetV);
#else
	sprintf(text, command, job.centerU, job.centerV, scaleU, scaleV, job.offsetU, job.offsetV);
#endif

	if (m_undoHistory != NULL)
	{
		MDGModifier* modifier = new MDGModifier;
		m_undoHistory->push_back(modifier);
		status = modifier->commandToExecute(text);
		status = modifier->doIt();
	}
	else
	{
		MGlobal::executeCommand(text);
	}

/*
	if (UVAutoRatioPro::IsProgressCancelled())
		return;

	if (job.offsetU != 0.0 || job.offsetV != 0.0)
	{
		const char* command = "polyMoveUV -tu %.20f -tv %.20f";

#ifdef WIN32
		sprintf_s(text, sizeof(text), command, job.offsetU, job.offsetV);
#else
		sprintf(text, command, job.offsetU, job.offsetV);
#endif

		if (m_undoHistory != NULL)
		{
			MDGModifier* modifier = new MDGModifier;
			m_undoHistory->push_back(modifier);
			status = modifier->commandToExecute(text);
			status = modifier->doIt();
		}
		else
		{
			MGlobal::executeCommand(text);
		}
	}
*/
	job.completed = true;
}