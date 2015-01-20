// Copyright © 2007 RenderHeads Ltd.  All Rights Reserved.
// Author: Andrew David Griffiths

#ifndef UVMESH_H
#define UVMESH_H

class UVMesh
{
public:
	UVMesh(MDagPath meshDagPath, const MString* uvSetName);
	~UVMesh();

	MFnMesh*		GetMesh();
	const char*		GetName();
	MDagPath&		GetPath();
	MFloatArray&	UArray();
	MFloatArray&	VArray();

private:
	MFnMesh*	m_mesh;
	MString		m_name;
	MDagPath	m_meshDagPath;

	MFloatArray m_uArray, m_vArray;
};

inline MFnMesh*
UVMesh::GetMesh()
{
	return m_mesh;
}
inline const char*
UVMesh::GetName()
{
	return m_name.asChar();
}
inline MDagPath&
UVMesh::GetPath()
{
	return m_meshDagPath;
}
inline MFloatArray&
UVMesh::UArray()
{
	return m_uArray;
}
inline MFloatArray&
UVMesh::VArray()
{
	return m_vArray;
}

#endif

