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
#include "ShellProcessor.h"

void
MeshProcessor::Gather(UVJob& job)
{
	MStatus status;

	job.surfaceArea = GetAreaMeshSurface(job.mesh->dagPath, true);
	if (job.surfaceArea == 0.0)
	{
		job.error = ZERO_SURFACE_AREA;
		return;
	}

	job.textureArea = GetAreaMeshUV(job.mesh->dagPath, &job.mesh->useUVSetName);
	job.finalTextureArea = job.textureArea;
	if (job.textureArea == 0.0)
	{
		job.error = ZERO_TEXTURE_AREA;
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

	// Find the UV Center
	double minU, maxU, minV, maxV;
	GetMinMaxValues(job.mesh->uArray, minU, maxU);
	GetMinMaxValues(job.mesh->vArray, minV, maxV);
	job.centerU = (minU + maxU) * 0.5;
	job.centerV = (minV + maxV) * 0.5;
	job.uvWidth = maxU - minU;
	job.uvHeight = maxV - minV;
}

JobError
MeshProcessor::FindScale(MeshJob& job, double threshold, double& finalScale, double& finalTextureArea, int& iterationsPerformed)
{
	MStatus status;

	JobError result = OK;

	double minScale, maxScale;
	double scale = 1.0;

	finalTextureArea = job.surfaceArea / m_params.m_goalRatio;

	if (job.textureArea > finalTextureArea)
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
		double newUVArea = GetAreaMeshUV(job.mesh->dagPath, &job.mesh->useUVSetName);

		// If it is within threshold, leave it and return
		{
			double ratio = job.surfaceArea / newUVArea;
			if (fabs(ratio - m_params.m_goalRatio) < threshold)
			//if (fabs(newUVArea - finalTextureArea) < threshold)
			{
				finalScale = scale;
				break;
			}
		}

		// Converge the min and max scale range
		if (newUVArea > finalTextureArea)
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
MeshProcessor::FindScaleFactor(double shapeArea, double targetArea, double width, double height) const
{
	double rectArea = width * height;
	double areaRatio = rectArea / shapeArea;      // how many times bigger the square is compared to the shape, and it'll ALWAYS be bigger

	double targetSquareArea = targetArea * areaRatio;
	double rectRatio = width / height;
	double targetHeight = sqrt(targetSquareArea / rectRatio);
	//double targetWidth = targetSquareArea / targetHeight;

	return targetHeight / height;
}

void
MeshProcessor::FindScale(UVJob& uvjob)
{
	MeshJob& job = (MeshJob&)uvjob;

	bool fast = true;

	if (m_params.m_scalingAxis != Both)
		fast = false;

	// try to quickly find the scale
	if (fast)
	{
		double targetTextureArea = job.surfaceArea / m_params.m_goalRatio;
		double scale = FindScaleFactor(job.textureArea, targetTextureArea, job.uvWidth, job.uvHeight);
		job.finalScaleX = scale;
		job.finalScaleY = scale;
		job.finalTextureArea = targetTextureArea;
		job.iterationsPerformed = 0;

		/*
		// Scale the UV's
		ScaleMeshUVs(job, scale);

		// Read the new UV Area
		double newUVArea = GetAreaMeshUV(job.mesh->dagPath, &job.mesh->useUVSetName);
		double ratio = job.surfaceArea / newUVArea;

		static char text[1024];
		sprintf_s(text, sizeof(text), "RESULTS: Scale:%.25f  Area:%.25f  Ratio:%.25f   %.25f", scale, newUVArea, ratio, fabs(ratio - m_params.m_goalRatio));
		MGlobal::displayInfo(text);

		// Scale the UV's back to original
		job.mesh->ResetUVs();*/
	}
	// if it's not accurate enough, iterate to find it
	else
	{
		// NHKL - check with Badge this is intentionally not used, and not a bug
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
			// At this point the scale wasn't found within the threshold accuracy
			// We can choose to fail or just pick the closest ratio
			bool takeBest = false;
			if (takeBest)
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
		}

		job.iterationsPerformed = totalIterations;

		// Scale the UV's back to original
		job.mesh->ResetUVs();
	}
}

void
MeshProcessor::ScaleMeshUVs(UVJob& job, double scale)
{
	MStatus status;

	// copy original UV's
	MFloatArray uArray, vArray;
	uArray.copy(job.mesh->uArray);
	vArray.copy(job.mesh->vArray);

	assert(uArray.length() == vArray.length());

	uint numSamples = uArray.length();
	uint i = 0;

	switch (m_params.m_scalingAxis)
	{
	case Both:
		for (i=0; i<numSamples; i++)
		{
			uArray[i] *= (float)scale;
			vArray[i] *= (float)scale;
		}
		break;
	case Horizontal:
		for (i=0; i<numSamples; i++)
		{
			uArray[i] *= (float)scale;
		}
		break;
	case Vertical:
		for (i=0; i<numSamples; i++)
		{
			vArray[i] *= (float)scale;
		}
		break;
	}

	// set mesh UV's
	status = job.mesh->model->setUVs(uArray, vArray, &job.mesh->useUVSetName);
}

void
MeshProcessor::ApplyScale(UVJob& job)
{
	MStatus status;

	// Select the UVs
	status = MGlobal::clearSelectionList();
	status = MGlobal::setSelectionMode(MGlobal::kSelectObjectMode);
	status = MGlobal::select(job.mesh->dagPath, MObject::kNullObj);
	status = MGlobal::executeCommand("ConvertSelectionToUVs;");


	// Execute scale command
	static char text[1024];
	/*sprintf_s(text, sizeof(text), "%lf  %.25f %.32f", job.centerU, job.centerU, job.centerU);
	MGlobal::displayInfo(text);*/
	//const char* command = "polyEditUV -relative true -pivotU %.20f -pivotV %.20f -scale true -scaleU %.20f -scaleV %.20f";
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

	job.completed = true;
}

