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

#ifndef SHELLPROCESSOR_H
#define SHELLPROCESSOR_H

#include <iostream>
#include <vector>

enum OperationMode
{
	ObjectLevel,
	UVShellLevel,
};

enum ScaleDirection
{
	Both,
	Horizontal,
	Vertical
};

struct UVAutoRatioProParams
{
	bool			m_isHelp;
	double			m_goalRatio;
	double			m_threshold;
	bool			m_isVerbose;
	bool			m_isShowTiming;
	MString			m_UVSetName;
	bool			m_isUVSetOverride;
	bool			m_isFallback;
	bool			m_skipScaling;
	bool			m_isColour;
	int				m_maxIterations;
	OperationMode	m_operationMode;
	bool			m_layoutShells;
	bool			m_normalise;
	bool			m_normaliseKeepAspectRatio;
	ScaleDirection	m_scalingAxis;
	uint			m_layoutIterations;
	double			m_layoutMinDistance;
	double			m_layoutStep;

	UVAutoRatioProParams& 		operator = (const UVAutoRatioProParams& src)
	{
		m_isHelp = src.m_isHelp;
		m_goalRatio = src.m_goalRatio;
		m_threshold = src.m_threshold;
		m_isVerbose = src.m_isVerbose;
		m_UVSetName = src.m_UVSetName;
		m_isUVSetOverride = src.m_isUVSetOverride;
		m_isFallback = src.m_isFallback;
		m_skipScaling = src.m_skipScaling;
		m_isColour = src.m_isColour;
		m_maxIterations = src.m_maxIterations;
		m_operationMode = src.m_operationMode;
		m_layoutShells = src.m_layoutShells;
		m_layoutIterations = src.m_layoutIterations;
		m_scalingAxis = src.m_scalingAxis;
		m_layoutStep = src.m_layoutStep;

		m_isShowTiming = src.m_isShowTiming;
		m_normalise = src.m_normalise;
		m_normaliseKeepAspectRatio = src.m_normaliseKeepAspectRatio;
		m_layoutMinDistance = src.m_layoutMinDistance;

		return *this;
	}
};

enum JobError
{
	OK = 0,
	NO_UVSET_FOUND = 1,
	UVSET_NOTFOUND = 2,
	RATIO_NOT_FOUND = 3,
	INVALID_MESH = 4,
	U_V_LISTS_DIFFERENT_LENGTHS = 5,
	ITERATION_FAILED = 6,
	ZERO_TEXTURE_AREA = 7,
	ZERO_SURFACE_AREA = 8,
	SKIPPED = 9,
};

class UVJob;
class Processor;

class Mesh
{
public:
	// stores mesh stuff
	// for each mesh that is going to be processed
	Mesh();
	~Mesh();

	MFnMesh*	model;
	MDagPath	dagPath;
	MString		name;

	MFloatArray uArray, vArray;

	MString		currentUVSetName, useUVSetName;

	JobError	error;

	std::vector<UVJob*>		m_jobs;

	void	Gather(Processor& processor, bool isUVSetOverride, bool isFallback, const MString& UVSetName);
	void	FindScale(Processor& processor, double goalRatio, double threshold);
	void	ApplyScale(Processor& processor);
	void	ResetUVs();
};

class UVJob
{
public:
	UVJob();
	virtual ~UVJob();

	virtual MString		GetName() const=0;

	JobError	error;

	double		surfaceArea, textureArea;
	double		finalScaleX, finalScaleY;
	double		finalTextureArea;
	int			iterationsPerformed;

	double		centerU, centerV;
	double		uvWidth, uvHeight;
	double		offsetU, offsetV;

	Mesh*		mesh;

	bool		completed;
};

class MeshJob : public UVJob
{
public:
	MeshJob();
	virtual ~MeshJob();

	MString		GetName() const;
};

class ShellJob : public UVJob
{
public:
	ShellJob();
	virtual ~ShellJob();

	int							meshShellNumber;
	int*						uvIndices;
	int							numIndices;
	MFnSingleIndexedComponent*	uvComponents;

	MObject			uvComponentObject;
	MObject			faceComponentObject;

	MString		GetName() const;
};

class Processor
{
public:
	UVAutoRatioProParams m_params;
	std::vector<MDGModifier*>* m_undoHistory;

	virtual void		Gather(UVJob& job)=0;
	virtual void		FindScale(UVJob& job)=0;
	virtual void		ApplyScale(UVJob& job)=0;
};

class MeshProcessor : public Processor
{
public:
	void		Gather(UVJob& job);
	void		FindScale(UVJob& job);
	void		ApplyScale(UVJob& job);

private:
	double		FindScaleFactor(double shapeArea, double targetArea, double width, double height) const;
	void		ScaleMeshUVs(UVJob& job, double scale);
	JobError	FindScale(MeshJob& job, double threshold, double& finalScale, double& finalTextureArea, int& iterationsPerformed);
};

class ShellProcessor : public Processor
{
public:
	void		Gather(UVJob& job);
	void		FindScale(UVJob& job);
	void		ApplyScale(UVJob& job);

private:
	double		FindScaleFactor(double shapeArea, double targetArea, double width, double height) const;
	void		ScaleMeshUVs(ShellJob& job, double scale);
	JobError	FindScale(ShellJob& job, double threshold, double& finalScale, double& finalTextureArea, int& iterationsPerformed);
};

#endif

