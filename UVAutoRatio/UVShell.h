// Copyright © 2007 RenderHeads Ltd.  All Rights Reserved.
// Author: Andrew David Griffiths

#ifndef UVSHELL_H
#define UVSHELL_H

#include <iostream>
#include <vector>

class UVMesh;

class UVShell
{
public:
	UVShell(UVMesh* mesh, MObject uvComponents, const MString* desiredUVSet = NULL);
	~UVShell();

	double	GetRatio() const;
	const char* GetMeshName() const;
	double	GetFinalScale() const;

	bool	Is(MDagPath meshDagPath, MObject uvComponents) const;
	void	ScaleUV(double scale, std::vector<MDGModifier*>* history);
	bool	CalculateScaleForRatio(double goalRatio, int& numIterations, double& finalScale, double threshold = 0.001, int maxIterations = 200);
	void	ScaleUVs(double scale, bool aboutCenter);

private:

	// job
	const MString*	m_uvSetName;		// if this is null, the current UV set will be used
	UVMesh*			m_mesh;
	double			m_centerU, m_centerV;
	MString		m_name;
	double		m_surfaceArea;
	double		m_uvArea;
	double		m_finalScale;

	// to be killed
	bool		m_isComponents;

	// uvshells
	std::vector<int> m_uvIndices;
	MFnSingleIndexedComponent* m_uvComponents;
	MObject m_uvComponentObject;
	MObject m_faceComponentObject;

	void	CollectData();
	bool	GetFaceComponents();
	void	FindUVCenter();
	double	GetRatioAtUVScale(double scale);
	void	GetUVIndices();
	void	SelectUVs(std::vector<MDGModifier*>* history, const MString& uvSetName);
};

inline double
UVShell::GetRatio() const
{
	return m_surfaceArea / m_uvArea;
}

inline double
UVShell::GetFinalScale() const
{
	return m_finalScale;
}

inline const char*
UVShell::GetMeshName() const
{
	return m_name.asChar();
}

#endif

