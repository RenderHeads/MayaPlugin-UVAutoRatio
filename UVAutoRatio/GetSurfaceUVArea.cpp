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
#include "GetSurfaceUVArea.h"

using namespace std;

static const char* error_minimumSelection = "Something must be selected";

const char* GetSurfaceUVArea::GetSurfaceUVArea_Help[] = {
	" GetSurfaceUVArea Help\n",
	"\n",
	"\t-help       (-hlp) This gets printed\n",
	"\t-uvSetName  (-us)  [string] The name of the UV set to use (optional)\n",
	"\t-fallback   (-fb)  If the named UV set is not found in a mesh, use the default UV set instead of skipping it (optional)\n",
	"\n"
};

unsigned int GetSurfaceUVArea::helpLineCount = sizeof(GetSurfaceUVArea_Help)/sizeof(GetSurfaceUVArea_Help[0]);


GetSurfaceUVArea::GetSurfaceUVArea()
{
	m_isHelp = false;
	m_isFallback = true;
	m_isUVSetOverride = false;
	m_reportZeroAreas = false;
}

GetSurfaceUVArea::~GetSurfaceUVArea()
{
}

void*
GetSurfaceUVArea::creator()
{
	return new GetSurfaceUVArea;
}

bool
GetSurfaceUVArea::isUndoable() const
{
	return false;
}

bool
GetSurfaceUVArea::hasSyntax() const
{
	return true;
}

MSyntax
GetSurfaceUVArea::newSyntax()
{
	MStatus status;
	MSyntax syntax;

	// flags
	syntax.addFlag("-hlp", "-help");
	syntax.addFlag("-fb", "-fallback");
	syntax.addFlag("-za", "-zeroArea");
	syntax.addFlag("-us", "-uvSetName",MSyntax::kString);

	syntax.useSelectionAsDefault(false);
	syntax.enableQuery(false);
	syntax.enableEdit(false);
	return syntax;
}

const char*
GetSurfaceUVArea::ParseArguments(const MArgList& args)
{
	MArgDatabase argData(syntax(), args);

	m_isHelp = argData.isFlagSet("-help");
	m_isFallback = argData.isFlagSet("-fallback");
	m_reportZeroAreas = argData.isFlagSet("-zeroArea");

	if (getArgValue(argData, "-us", "-uvSetName", m_UVSetName))
	{
		m_isUVSetOverride = true;
	}

	return NULL;
}

MStatus
GetSurfaceUVArea::Initialise(const MArgList& args)
{
	clearResult();

	// Parse arguments
	const char* errorMessage = ParseArguments(args);

	// Display help
	if (m_isHelp)
	{
		DisplayHelp();
		return MS::kSuccess;
	}

	if (errorMessage)
	{
		displayError(errorMessage);
		return MS::kFailure;
	}

	return MS::kSuccess;
}

void
GetSurfaceUVArea::DisplayHelp() const
{
	for (unsigned int i=0; i<helpLineCount; i++)
	{
		appendToResult(GetSurfaceUVArea_Help[i]);
	}
}


MStatus
GetSurfaceUVArea::doIt( const MArgList& args )
{
	MStatus status;

	status = Initialise(args);
	if (status != MS::kSuccess)
	{
		return status;
	}

	MSelectionList selection;
	/*
	MString selectionString = args.asString(0, &status);

	// Either get the selection from the active selection,
	// or from the first parameter which is a list of selection components
	

	// Create a selection from the selectionString
	if (selectionString.length() > 0)
	{
		// Split selection names out from string into string array
		MStringArray selectionNames;
		selectionString.split(' ', selectionNames);

		// Build up selection from selection names
		MSelectionList selection;
		selection.clear();
		for (unsigned int i = 0; i < selectionNames.length(); i++)
		{
			selection.add(selectionNames[i]);
		}
		MGlobal::setActiveSelectionList(selection);
	}*/

	// Convert selection to internal faces
	//status = MGlobal::executeCommand("select `polyListComponentConversion -fromUV -fromFace -fromVertex -toFace -internal`");

	// Get selection
	//status = MGlobal::getActiveSelectionList(selection);

	MStringArray selections;
	MGlobal::executeCommand("polyListComponentConversion -fromUV -fromEdge -fromFace -fromVertex -toFace -internal;", selections);

	selection.clear();
	for (unsigned int i = 0; i < selections.length(); i++)
	{
		selection.add(selections[i]);
	}

	double uvArea = 0.0;
	double surfaceArea = 0.0;

	// Process the selection
	MItSelectionList iter(selection);
	for ( ; !iter.isDone(); iter.next() )
	{
		MDagPath	dagPath;
		MObject		component;

		iter.getDagPath( dagPath, component );
		if (dagPath.node().hasFn(MFn::kPolyMesh) || dagPath.node().hasFn(MFn::kMesh))
		{
			dagPath.extendToShape();

			const MString* desiredUVSetName = NULL;
			if (m_isUVSetOverride)
			{
				desiredUVSetName = &m_UVSetName;
			}
			MFnMesh mesh(dagPath);
			if (FindMeshUVSetName(mesh, m_isUVSetOverride, m_isFallback, &desiredUVSetName))
			{
				// Accumulate areas
				uvArea += GetAreaFacesUV(dagPath, component, desiredUVSetName);
				surfaceArea += GetAreaFacesSurface(dagPath, component, true);
			}
		}
	}

	double ratio = 1.0;
	if (surfaceArea != 0.0 && uvArea != 0.0)
	{
		ratio = surfaceArea / uvArea;
	}

	// Due to Maya's lack of precision when returning doubles, we need to convert to strings...
	// or NOT, seems this also sucks
	appendToResult(surfaceArea);
	appendToResult(uvArea);
	appendToResult(ratio);

/*
	surfaceArea=0.00000001;
	uvArea=0.0000000000000002;

	static char text[128];
	sprintf_s(text, sizeof(text), "%.25f", surfaceArea);
	appendToResult(text);
	sprintf_s(text, sizeof(text), "%.25f", uvArea);
	appendToResult(text);
*/
	//appendToResult("0.00000001");
	//appendToResult("0.0000000000000002");

	/*MDoubleArray results;
	results.append(0.00000001);
	results.append(0.0000000000000002);
	setResult(results);*/

	return MS::kSuccess;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

