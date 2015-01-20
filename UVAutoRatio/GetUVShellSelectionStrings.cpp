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
#include "MayaUtility.h"
#include "Utility.h"
#include "GetUVShellSelectionStrings.h"

using namespace std;

GetUVShellSelectionStrings::GetUVShellSelectionStrings()
{
}

GetUVShellSelectionStrings::~GetUVShellSelectionStrings()
{
}

void*
GetUVShellSelectionStrings::creator()
{
	return new GetUVShellSelectionStrings;
}

bool
GetUVShellSelectionStrings::isUndoable() const
{
	return false;
}

bool
GetUVShellSelectionStrings::hasSyntax() const
{
	return false;
}

MStatus
GetUVShellSelectionStrings::doIt( const MArgList& args )
{
	MStatus status;
	MStringArray result;

	MSelectionList activeList;
	MGlobal::getActiveSelectionList (activeList);

	// Process the selection
	int objectIndex = 0;
	MItSelectionList iter(activeList);
	for ( ; !iter.isDone(); iter.next() ) 
	{
		MDagPath	dagPath;
		MObject		component;
		iter.getDagPath( dagPath, component );
		if (dagPath.node().hasFn(MFn::kTransform) || dagPath.node().hasFn(MFn::kPolyMesh) || dagPath.node().hasFn(MFn::kMesh))
		{
			// Get Mesh
			dagPath.extendToShape();
			MFnMesh mesh(dagPath, &status);

			MString uvSetName;
			//MStringArray setNames;
			//mesh.getUVSetNames(setNames)
			//for (int j = 0; j < setNames.length(); j++)
			//{
			//	MString uvSetName = setNames[j];
				
#ifdef MAYA85
				uvSetName = mesh.currentUVSetName();	
#else
				mesh.getCurrentUVSetName(uvSetName);
#endif		

			//}

			MIntArray uvShellIDs;
			unsigned int numShells;
			status = mesh.getUvShellsIds(uvShellIDs, numShells, &uvSetName);

			for (unsigned int shellIndex = 0; shellIndex < numShells; shellIndex++)
			{
				/*// Count number of UV in this shell
				int numUVs = 0;
				for (unsigned int j = 0; j < uvShellIDs.length(); j++)
				{
					if (i == uvShellIDs[j])
					{
						numUVs++;
					}
				}*/

				// Build component list for UVs in shell
				int numComponents = 0;
				MFnSingleIndexedComponent uvComponent;
				MObject uvComponentObject = uvComponent.create(MFn::kMeshMapComponent, &status);
				for (unsigned int j = 0; j < uvShellIDs.length(); j++)
				{
					if (shellIndex == uvShellIDs[j])
					{
						uvComponent.addElement(j);
						numComponents++;
					}
				}

				// Create selection for UVs
				MSelectionList uvSelection;
				uvSelection.add(dagPath, uvComponentObject);

				/*// Select UVs
				status = MGlobal::clearSelectionList();
				status = MGlobal::setSelectionMode(MGlobal::MSelectionMode::kSelectComponentMode);
				MSelectionMask uvSelectionMask(MSelectionMask::kSelectMeshUVs);
				status = MGlobal::setComponentSelectionMask(uvSelectionMask);
				status = MGlobal::setActiveSelectionList(uvSelection);*/


				MStringArray selectionStrings;
				uvSelection.getSelectionStrings(selectionStrings);
				MString selectionString = Convert(selectionStrings);

				// Friendly name
				MString friendlyString;

				friendlyString += objectIndex;
				friendlyString += ") " + mesh.name() + ".shell";
				friendlyString += (int)shellIndex;
				friendlyString += "  ";
				friendlyString += numComponents;
				friendlyString += "uvs  (" + uvSetName + ")                 ";
				friendlyString += "*" + selectionString;

				// 1) polySurfaceShape14 s34 34 (map1)
				this->appendToResult(friendlyString);
				//this->appendToResult(selectionString);

				objectIndex++;
			}
		}
	}


	return MS::kSuccess;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

