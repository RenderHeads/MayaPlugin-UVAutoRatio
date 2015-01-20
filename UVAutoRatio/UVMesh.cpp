// Copyright � 2007 RenderHeads Ltd.  All Rights Reserved.
// Author: Andrew David Griffiths

#define MNoVersionString
#include "MayaPCH.h"
#include "Utility.h"
#include "UVMesh.h"

UVMesh::UVMesh(MDagPath meshDagPath, const MString* uvSetName)
{
	m_meshDagPath = meshDagPath;
	m_meshDagPath.extendToShape();
	m_mesh = new MFnMesh(m_meshDagPath);
	m_name = m_mesh->name();

	MStatus status;

	// Get all the UV's
	status = m_mesh->getUVs(m_uArray, m_vArray, uvSetName);
}

UVMesh::~UVMesh()
{
	if (m_mesh)
	{
		delete m_mesh;
		m_mesh = NULL;
	}
}

