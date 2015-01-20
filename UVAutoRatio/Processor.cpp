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
#include "UVAutoRatioPro.h"
#include "ShellProcessor.h"

Mesh::Mesh()
{
	model = NULL;
	error = OK;
	m_jobs.reserve(32);
}

Mesh::~Mesh()
{
	if (model != NULL)
	{
		delete model;
		model = NULL;
	}

	// Delete all jobs
	{
		std::vector<UVJob*>::reverse_iterator riter;
		for ( riter = m_jobs.rbegin(); riter != m_jobs.rend(); ++riter )
		{
			delete (*riter);
		}
		m_jobs.clear();
	}
}

void
Mesh::ResetUVs()
{
	MStatus status;
	status = model->setUVs(uArray, vArray, &useUVSetName);
	//CHECK_MSTATUS(status);
}

void
Mesh::Gather(Processor& processor, bool isUVSetOverride, bool isFallback, const MString& UVSetName)
{
	MStatus status;

	// Mesh data
	model = new MFnMesh(dagPath, &status);
	if (status != MS::kSuccess)
	{
		error = INVALID_MESH;
		return;
	}
	name = model->name();

	// Find the UV set to use
	UVSetResult uvSetResult = FindUVSet(*model, isUVSetOverride, isFallback, UVSetName);
	if (uvSetResult == NONE)
	{
		error = UVSET_NOTFOUND;
		return;
	}
	else
	{
		currentUVSetName = GetCurrentUVSetName(*model);

		switch (uvSetResult)
		{
		case CURRENT:
			useUVSetName = currentUVSetName;//NULL;
			break;
		case OVERRIDE:
			useUVSetName = UVSetName;
			model->setCurrentUVSetName(useUVSetName);
			break;
		case NONE:
			break;
		}
	}

	UVAutoRatioPro::SetNumSubTasks((int)m_jobs.size(), "Jobs");
	for (uint i = 0; i < m_jobs.size(); i++)
	{
		if (UVAutoRatioPro::IsProgressCancelled())
			break;

		UVAutoRatioPro::StepJobProgress();

		UVJob& job = *m_jobs[i];
		processor.Gather(job);
	}
}

void
Mesh::FindScale(Processor& processor, double goalRatio, double threshold)
{
	// if an alternative uvset was used, restore the previous one
	if (useUVSetName != currentUVSetName)
	{
		model->setCurrentUVSetName(useUVSetName);
	}

	UVAutoRatioPro::SetNumSubTasks((int)m_jobs.size(), "Jobs");
	for (uint i = 0; i < m_jobs.size(); i++)
	{
		if (UVAutoRatioPro::IsProgressCancelled())
			break;

		UVAutoRatioPro::StepJobProgress();

		UVJob& job = *m_jobs[i];
		if (!job.error)
		{
			double ratio = job.surfaceArea / job.textureArea;

			// if we're close enough then don't operate
			if (fabs(ratio - goalRatio) >= threshold)
			{
				processor.FindScale(job);
			}
		}
	}
}

void
Mesh::ApplyScale(Processor& processor)
{
	UVAutoRatioPro::SetNumSubTasks((int)m_jobs.size(), "Jobs");
	for (uint i = 0; i < m_jobs.size(); i++)
	{
		if (UVAutoRatioPro::IsProgressCancelled())
			break;

		UVAutoRatioPro::StepJobProgress();

		UVJob& job = *m_jobs[i];
		if (!job.error)
		{
			if (job.finalScaleX != 1.0 || job.finalScaleY != 1.0 || job.offsetU != 0.0 || job.offsetV != 0.0)
			{
				processor.ApplyScale(job);
			}
		}
	}

	// if an alternative uvset was used, restore the previous one
	if (useUVSetName != currentUVSetName)
	{
		model->setCurrentUVSetName(currentUVSetName);
	}
}

UVJob::UVJob()
{
	error = OK;
	completed = false;
	surfaceArea = 0.0;
	textureArea = 0.0;
	finalScaleX = finalScaleY = 1.0;
	finalTextureArea = 0.0;
	iterationsPerformed = 0;

	centerU = centerV = 0.0;
	uvWidth = uvHeight = 0.0;
	offsetU = offsetV = 0.0;

	mesh = NULL;
}


UVJob::~UVJob()
{
}

MeshJob::MeshJob() : UVJob()
{
}

MeshJob::~MeshJob()
{
}

ShellJob::ShellJob() : UVJob()
{
	meshShellNumber = 0;
	uvIndices = NULL;
	uvComponents = NULL;
	numIndices = 0;
}

ShellJob::~ShellJob()
{
	if (uvIndices != NULL)
	{
		delete [] uvIndices;
		uvIndices = NULL;
	}

	if (uvComponents != NULL)
	{
		delete uvComponents;
		uvComponents = NULL;
	}
}


MString
MeshJob::GetName() const
{
	return mesh->name;
}

MString
ShellJob::GetName() const
{
	MString name = " shell:";
	name += meshShellNumber;
	return name;
}

