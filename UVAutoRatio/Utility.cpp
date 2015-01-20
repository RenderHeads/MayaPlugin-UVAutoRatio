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

double
GetAreaMeshSurface(const MDagPath& meshDagPath, bool isWorldSpace)
{
	return GetAreaFacesSurface(meshDagPath, MObject::kNullObj, isWorldSpace);
}

double
GetAreaFacesSurface(const MDagPath& dagPath, MObject& component, bool isWorldSpace)
{
	MStatus status;

	double result = 0.0;
	MItMeshPolygon iter(dagPath, component, &status);

	if (isWorldSpace)
	{
		for ( ; !iter.isDone(); iter.next() ) 
		{
			double area = 0.0;

			area = 0.0;
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
				if (areaSquared > 0.0)
					area += sqrt(areaSquared);

				/*float2 uv1, uv2, uv3;
				status = iter.getUV (a, uv1, NULL);
				status = iter.getUV (b, uv2, NULL);
				status = iter.getUV (c, uv3, NULL);*/
			}

			result += area;
		}
	}
	else
	{
		for ( ; !iter.isDone(); iter.next() ) 
		{
			// NOTE: This function "getArea" can only return the area in in object space
			//       it is a limitation of Maya's API
			double area = 0.0;
			status = iter.getArea(area, MSpace::kObject);
			result += area;
		}
	}

	return result;
}

double
GetAreaMeshUV(const MDagPath& meshDagPath, const MString* uvSetName)
{
	return GetAreaFacesUV(meshDagPath, MObject::kNullObj, uvSetName);
}

double
GetAreaFacesUV(const MDagPath& meshDagPath, MObject& component, const MString* uvSetName)
{
	MStatus status;

	double result = 0.0;

	/*
	// There is a bug in Maya's API so we can't use this way
	// If a mesh has a tweak, but no history, this method doesn't return the real area
	MItMeshPolygon iter(meshDagPath, component, &status);
	for ( ; !iter.isDone(); iter.next() ) 
	{
		double faceArea = 0.0;
		iter.getUVArea(faceArea, uvSetName);
		result += faceArea;
	}*/
	
	{
		MItMeshPolygon iter(meshDagPath, component, &status);
		for ( ; !iter.isDone(); iter.next() ) 
		{
			double faceArea = 0.0;
			
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

				faceArea += GetTriangleArea2D(uv1, uv2, uv3);
			}

			result += faceArea;
		}
	}


/*
	int faceID = 0;
	int vertCount = 0;
 	MItMeshPolygon iter(meshDagPath, component, &status);
	for ( ; !iter.isDone(); iter.next() ) 
	{
		double area = 0.0;

		// Go through each triangle in the polygon

		MIntArray polygonToFaceVertexIndexMap;
		iter.getVertices(polygonToFaceVertexIndexMap);

		int numTrianglesInPoly=0;
		iter.numTriangles(numTrianglesInPoly);
		for (int j=0; j<numTrianglesInPoly; j++)
		{
			MPointArray			unTweakedTrianglePoints;
			MIntArray			meshVertexIndices;	// index's into the meshes vertex list!
			iter.getTriangle(j,unTweakedTrianglePoints,meshVertexIndices,MSpace::kObject); 

			// the indices we have are relative to the whole mesh
			// convert to polygon relative indices
			
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

			// Get the values of those UVs at the triangle indices
			MItMeshVertex itMeshVert(meshDagPath);

			float2 uv1, uv2, uv3;

			// Set index
			int unused, iMeshVertexMaya;
				
			iMeshVertexMaya=iter.vertexIndex(polygonVertexIndex[0]);
			itMeshVert.setIndex(iMeshVertexMaya, unused);

			MStatus stat=itMeshVert.getUV(faceID,uv1,NULL);
			if (stat.error())
			{
				uv1[0]=uv1[1]=0.0f;
			}

			iMeshVertexMaya=iter.vertexIndex(polygonVertexIndex[1]);
			itMeshVert.setIndex(iMeshVertexMaya, unused);

			stat=itMeshVert.getUV(faceID,uv2,NULL);
			if (stat.error())
			{
				uv2[0]=uv2[1]=0.0f;
			}


			iMeshVertexMaya=iter.vertexIndex(polygonVertexIndex[2]);
			itMeshVert.setIndex(iMeshVertexMaya, unused);

			stat=itMeshVert.getUV(faceID,uv3,NULL);
			if (stat.error())
			{
				uv3[0]=uv3[1]=0.0f;
			}

			MPoint a(uv1[0], uv1[1], 1.0, 1.0);
			MPoint b(uv2[0], uv2[1], 1.0, 1.0);
			MPoint c(uv3[0], uv3[1], 1.0, 1.0);

			char txt[512];
			sprintf_s(txt, 512, "%f, %f   %f, %f   %f, %f", uv1[0], uv1[1], uv2[0], uv2[1], uv3[0], uv3[1]);
			MGlobal::displayInfo(txt);

			double la = a.distanceTo(b);
			double lb = a.distanceTo(c);
			double lc = b.distanceTo(c);
			area += GetTriangleArea(la, lb, lc);
		}

		result += area;
		faceID++;
	}*/
	return result;
}

void
GetMinMaxValues(const MFloatArray& values, double& mmin, double& mmax)
{
	float low, hi;
	low = FLT_MAX;
	hi = -FLT_MAX;

	uint numSamples = values.length();
	for (uint i = 0; i < numSamples; i++)
	{
#ifdef WIN32
		low = min(low, values[i]);
		hi = max(hi, values[i]);
#else
		low = std::min(low, values[i]);
		hi = std::max(hi, values[i]);
#endif
	}

	mmin = low;
	mmax = hi;
}

void
GetMinMaxValues(const MFloatArray& values, const int* indices, int numIndices, double& mmin, double& mmax)
{
	float low, hi;
	low = FLT_MAX;
	hi = -FLT_MAX;

	uint numSamples = (uint)numIndices;
	for (uint i = 0; i < numSamples; i++)
	{
		int index = indices[i];
		assert(index >= 0 && index < (int)values.length());

#ifdef WIN32
		low = min(low, values[index]);
		hi = max(hi, values[index]);
#else
		low = std::min(low, values[index]);
		hi = std::max(hi, values[index]);
#endif
	}

	mmin = low;
	mmax = hi;
}

bool
ContainsUVSet(const MFnMesh& mesh, const MString& uvSetName)
{
	MStringArray uvSetNames;
	mesh.getUVSetNames(uvSetNames);
	for (uint i = 0; i < uvSetNames.length(); i++)
	{
		if (uvSetNames[i] == uvSetName)
		{
			return true;
		}
	}

	return false;
}

bool
FindMeshUVSetName(const MFnMesh& mesh, bool overrideCurrentUVSet, bool fallback, const MString** desiredUVSetName)
{
	static const char* error_noUVSets = "Mesh '%s' contains no UVSet.";
	static const char* error_UVSetNotFound = "Specified UVSet '%s' not found in mesh '%s', skipping operation";

	static char text[2048];

	// check there at least one UV set
	if (mesh.numUVSets() <= 0)
	{
#ifdef WIN32
		sprintf_s(text, sizeof(text), error_noUVSets, mesh.name().asChar());
#else
		sprintf(text, error_noUVSets, mesh.name().asChar());
#endif
		MGlobal::displayWarning(text);
		return false;
	}

	// If a specific UV set is specified, verify that it exists
	if (overrideCurrentUVSet)
	{
		// try find the specified UV set
		if (!ContainsUVSet(mesh, **desiredUVSetName))
		{
			// we can't find the specified UV set,
			// so if we can fallback, use the current UV set
			if (fallback)
			{
				*desiredUVSetName = NULL;
			}
			else
			{
#ifdef WIN32
				sprintf_s(text, sizeof(text), error_UVSetNotFound, (*desiredUVSetName)->asChar(), mesh.name().asChar());
#else
				sprintf(text, error_UVSetNotFound, (*desiredUVSetName)->asChar(), mesh.name().asChar());
#endif
				MGlobal::displayWarning(text);
				return false;
			}
		}
	}
	else
	{
		// use the current UV set
		*desiredUVSetName = NULL;
	}

	return true;
}

// take the UV components and convert to face components
bool
UVToFaceComponents(const MDagPath& meshDagPath, const MObject& uvComponents, MObject& faceComponents)
{
	MStatus status;

	// Create selection for UVs components
	MSelectionList uvSelection;
	uvSelection.add(meshDagPath, uvComponents);

	// Set UV selection in Maya
	status = MGlobal::clearSelectionList();
	status = MGlobal::setSelectionMode(MGlobal::kSelectComponentMode);
	MSelectionMask uvSelectionMask(MSelectionMask::kSelectMeshUVs);
	status = MGlobal::setComponentSelectionMask(uvSelectionMask);
	status = MGlobal::setActiveSelectionList(uvSelection);

	// Convert selection to faces
	MGlobal::executeCommand("ConvertSelectionToFaces");

	// Get face selection
	{
		MSelectionList faceSelection;
		status = MGlobal::getActiveSelectionList(faceSelection);

		// Verify there should only be 1 object selected
		if (faceSelection.length() != 1)
		{
			return false;
		}

		MDagPath meshDagPath;
		status = faceSelection.getDagPath(0, meshDagPath, faceComponents);
	}

	return true;
}

void
SelectUVSet(std::vector<MDGModifier*>* history, const MString& uvSetName)
{
	MStatus status;

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
}

MString
GetCurrentUVSetName(const MFnMesh& mesh)
{
	MString status;

	MString currentUVSetName;
#if MAYA65 || MAYA60 || MAYA70 || MAYA80
	status = mesh.getCurrentUVSetName(currentUVSetName);
#else
	currentUVSetName = mesh.currentUVSetName();
#endif

	return currentUVSetName;
}

UVSetResult
FindUVSet(const MFnMesh& mesh, bool overrideUVSet, bool fallbackToCurrentAllowed, const MString& overrideSetName)
{
	UVSetResult result = NONE;

	// check there is at least one uvset
	if (mesh.numUVSets() <= 0)
	{
		result = NONE;
		return result;
	}

	// If we have to override which UV set to use
	if (overrideUVSet)
	{
		MStatus status;

		MString currentUVSetName = GetCurrentUVSetName(mesh);

		if (overrideSetName == currentUVSetName)
		{
			result = CURRENT;
		}
		else
		{
			bool hasSet = ContainsUVSet(mesh, overrideSetName);

			if (hasSet)
			{
				result = OVERRIDE;
			}
			else
			{
				// if we can fall back to the default UV set
				if (fallbackToCurrentAllowed)
				{
					result = CURRENT;
				}
				else
				{
					result = NONE;
				}
			}
		}
	}
	else
	{
		result = CURRENT;
	}

	return result;
}

