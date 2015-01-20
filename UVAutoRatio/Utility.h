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

#ifndef UTILITY_H
#define UTILITY_H

#include <iostream>
#include <vector>
#include <string>

enum UVSetResult
{
	NONE,
	CURRENT,
	OVERRIDE,
};

double		GetAreaMeshUV(const MDagPath& meshDagPath, const MString* uvSetName);
double		GetAreaMeshSurface(const MDagPath& meshDagPath, bool isWorldSpace);
double		GetAreaFacesUV(const MDagPath& dagPath, MObject& component, const MString* uvSetName);
double		GetAreaFacesSurface(const MDagPath& dagPath, MObject& component, bool isWorldSpace);

void		GetMinMaxValues(const MFloatArray& values, double& mmin, double& mmax);
void		GetMinMaxValues(const MFloatArray& values, const int* indices, int numIndices, double& mmin, double& mmax);

bool		ContainsUVSet(const MFnMesh& mesh, const MString& uvSetName);
bool		FindMeshUVSetName(const MFnMesh& mesh, bool overrideCurrentUVSet, bool fallback, const MString** desiredUVSetName);

bool		UVToFaceComponents(const MDagPath& meshDagPath, const MObject& uvComponents, MObject& faceComponents);
void		SelectUVSet(std::vector<MDGModifier*>* history, const MString& uvSetName);
MString		GetCurrentUVSetName(const MFnMesh& mesh);
UVSetResult	FindUVSet(const MFnMesh& mesh, bool overrideUVSet, bool fallbackToCurrentAllowed, const MString& overrideSetName);

// Heron 3d Triangle Area algorithm
// a, b, c are the lengths of the sides of the triangle
// There are faster algorithms, but this will do for now
inline double
GetTriangleAreaSquared(const double& a, const double& b, const double& c)
{
	double s = (a + b + c) * 0.5;
	double areaSquared = s * (s - a) * (s - b) * (s - c);
	return areaSquared;
}

// Very fast 2d triangle area function
inline double
GetTriangleArea2D(const float2& p0, const float2& p1, const float2& p2)
{
	double s = (p1[0] - p0[0]) * (p2[1] - p0[1]);
	double t = (p2[0] - p0[0]) * (p1[1] - p0[1]);
	return fabs((s - t) * 0.5);
}

#endif