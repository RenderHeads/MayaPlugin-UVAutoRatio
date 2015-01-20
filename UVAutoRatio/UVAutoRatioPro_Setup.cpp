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
#include "UVShell.h"
#include "UVAutoRatioPro.h"

const char* UVAutoRatioPro::UVAutoRatio_Help[] = {
	" UVAutoRatio PRO Help\n",
	"\n",
	" Works with selections of polygon objects\n",
	"\n",
	"\t-help       (-hlp) This gets printed\n",
	"\t-ratio      (-r)   [double] Desired 2D : 3D ratio\n",
	"\t-uvSetName  (-us)  [string] The name of the UV set to use (optional)\n",
	"\t-fallback   (-fb)  If the named UV set is not found in a mesh, use the default UV set instead of skipping it (optional)\n",
	"\t-iterations (-it)  [integer] Maximum number of iterations (optional), default 200\n",
	"\t-threshold  (-th)  [double] Accuracy threshold for ratio finder (optional), default 0.001\n",
	"\t-operation  (-op)  [integer] 0 = whole mesh, 1 = uv shell\n",
	"\t-verbose    (-vb)  Display output (optional), default false\n",
	"\t-layout     (-lay) Layout UV shells to prevent overlapping (optional), default true\n",
	"\t-skipscale  (-ss)  Skip the scaling operation (useful if you only want to fix layout)\n",
	"\t-onlyScaleH (-osh) Restrict scaling of UVs to horizontal axis (optional), default false\n",
	"\t-onlyScaleV (-osv) Restrict scaling of UVs to vertical axis (optional), default false\n",
	"\n"
};

unsigned int UVAutoRatioPro::helpLineCount=sizeof(UVAutoRatio_Help)/sizeof(UVAutoRatio_Help[0]);

MSyntax
UVAutoRatioPro::newSyntax()
{
	MStatus status;
	MSyntax syntax;

	// flags
	syntax.addFlag("-hlp", "-help");
	syntax.addFlag("-vb", "-verbose");
	syntax.addFlag("-tim", "-showTimings");
	syntax.addFlag("-fb", "-fallback");
	syntax.addFlag("-r", "-ratio",MSyntax::kDouble);
	syntax.addFlag("-th", "-threshold",MSyntax::kDouble);
	syntax.addFlag("-it", "-iterations", MSyntax::kLong);
	syntax.addFlag("-op", "-operation", MSyntax::kLong);
	syntax.addFlag("-us", "-uvSetName",MSyntax::kString);
	syntax.addFlag("-lay", "-layout");
	syntax.addFlag("-nor", "-normalise");
	syntax.addFlag("-kar", "-keepAspectRatio");
	syntax.addFlag("-lai", "-layoutIterations", MSyntax::kLong);
	syntax.addFlag("-las", "-layoutStep", MSyntax::kDouble);
	syntax.addFlag("-lad", "-layoutMinDistance", MSyntax::kDouble);
	syntax.addFlag("-ss", "-skipscale");
	syntax.addFlag("-osh", "-onlyScaleH");
	syntax.addFlag("-osv", "-onlyScaleV");
	syntax.addFlag("-col", "-colour");
	
	syntax.useSelectionAsDefault(false);
	syntax.enableQuery(false);
	syntax.enableEdit(false);
	return syntax;
}

const char*
UVAutoRatioPro::ParseArguments(const MArgList& args)
{
	//static const char* error_noArgs = "No parameters specified. -help for available flags.";
	static const char* error_noRatio = "No ratio parameter specified.";
	static const char* error_bothScalings = "Cannot have both scaling directions limited";

	MArgDatabase argData(syntax(), args);

	m_params.m_isHelp = argData.isFlagSet("-help");

	m_params.m_isVerbose = argData.isFlagSet("-verbose");
	m_params.m_isShowTiming = argData.isFlagSet("-showTimings");
	m_params.m_isFallback = argData.isFlagSet("-fallback");
	m_params.m_layoutShells = argData.isFlagSet("-layout");
	m_params.m_normalise = argData.isFlagSet("-normalise");
	m_params.m_normaliseKeepAspectRatio = argData.isFlagSet("-keepAspectRatio");
	m_params.m_skipScaling = argData.isFlagSet("-skipscale");
	m_params.m_isColour = argData.isFlagSet("-colour");

	if (m_params.m_layoutShells)
	{
		getArgValue(argData, "-lai", "-layoutIterations", m_params.m_layoutIterations);
		getArgValue(argData, "-las", "-layoutStep", m_params.m_layoutStep);
		getArgValue(argData, "-lad", "-layoutMinDistance", m_params.m_layoutMinDistance);
		m_params.m_layoutIterations = ClampUInt(1, 10000, m_params.m_layoutIterations);
		m_params.m_layoutStep = ClampDouble(0.00001, 0.1, m_params.m_layoutStep);
		m_params.m_layoutMinDistance = ClampDouble(0.0, 1000.0, m_params.m_layoutMinDistance);
	}

	if (!m_params.m_skipScaling)
	{
		if (!getArgValue(argData, "-r", "-ratio", m_params.m_goalRatio))
		{
			return error_noRatio;
		}

		if (argData.isFlagSet("-onlyScaleH") && argData.isFlagSet("-onlyScaleV"))
		{
			return error_bothScalings;
		}

		if (argData.isFlagSet("-onlyScaleH"))
			m_params.m_scalingAxis = Horizontal;
		else
			if (argData.isFlagSet("-onlyScaleV"))
				m_params.m_scalingAxis = Vertical;

		getArgValue(argData, "-it", "-iterations", m_params.m_maxIterations);
		getArgValue(argData, "-th", "-threshold", m_params.m_threshold);
		
		m_params.m_maxIterations = ClampInt(1, 10000, m_params.m_maxIterations);
		m_params.m_layoutIterations = ClampUInt(1, 10000, m_params.m_layoutIterations);
		m_params.m_threshold = ClampDouble(0.0001, 1.0, m_params.m_threshold);
	}

	int opMode = 0;
	getArgValue(argData, "-op", "-operation", opMode);
	m_params.m_operationMode = (OperationMode)opMode;
	
	if (getArgValue(argData, "-us", "-uvSetName", m_params.m_UVSetName))
	{
		m_params.m_isUVSetOverride = true;
	}

	return NULL;
}

void
UVAutoRatioPro::DisplayHelp() const
{
	for (unsigned int i=0; i<helpLineCount; i++)
	{
		appendToResult(UVAutoRatio_Help[i]);
	}
}

UVAutoRatioPro::UVAutoRatioPro()
{
	m_savedObjectSelectionMask = NULL;
	m_savedComponentSelectionMask = NULL;

	m_params.m_goalRatio = 0.0;
	m_params.m_maxIterations = 200;
	m_params.m_threshold = 0.001;
	m_params.m_isHelp = false;
	m_params.m_isVerbose = false;
	m_params.m_isShowTiming = false;
	m_params.m_isFallback = false;
	m_params.m_skipScaling = false;
	m_params.m_isUVSetOverride = false;
	m_params.m_operationMode = ObjectLevel;
	m_params.m_layoutShells = false;
	m_params.m_layoutIterations = 10000;
	m_params.m_scalingAxis = Both;
	m_params.m_isColour = false;
	m_params.m_layoutStep = 0.001;
	m_params.m_layoutMinDistance = 0.0;
	m_params.m_normalise = false;
	m_params.m_normaliseKeepAspectRatio = true;

	m_activeProcessor = NULL;

	m_loadTime = 0;
	m_gatherTime = 0;
	m_processTime = 0;
	m_layoutTime = 0;
	m_applyTime = 0;
	m_totalTime = 0;

	m_meshes.reserve(128);
}

UVAutoRatioPro::~UVAutoRatioPro()
{
	if (m_activeProcessor != NULL)
	{
		delete m_activeProcessor;
		m_activeProcessor = NULL;
	}


	if (m_savedObjectSelectionMask)
	{
		delete m_savedObjectSelectionMask;
		m_savedObjectSelectionMask = NULL;
	}
	if (m_savedComponentSelectionMask)
	{
		delete m_savedComponentSelectionMask;
		m_savedComponentSelectionMask = NULL;
	}

	// Delete all undo info
	{
		std::vector<MDGModifier*>::reverse_iterator riter;
		for ( riter = m_undos.rbegin(); riter != m_undos.rend(); ++riter )
		{
			delete (*riter);
		}
		m_undos.clear();
	}

	// Delete all meshes
	{
		std::vector<Mesh*>::reverse_iterator riter;
		for ( riter = m_meshes.rbegin(); riter != m_meshes.rend(); ++riter )
		{
			delete (*riter);
		}
		m_meshes.clear();
	}
}

void*
UVAutoRatioPro::creator()
{
	return new UVAutoRatioPro;
}

bool
UVAutoRatioPro::isUndoable() const
{
	return true;
}

bool
UVAutoRatioPro::hasSyntax() const
{
	return true;
}


void
UVAutoRatioPro::SaveSelection()
{
	// Get the current selection
	m_savedSelectionMode = MGlobal::selectionMode();
	m_savedObjectSelectionMask = new MSelectionMask(MGlobal::objectSelectionMask());
	m_savedComponentSelectionMask = new MSelectionMask(MGlobal::componentSelectionMask());
	MGlobal::getActiveSelectionList(m_savedSelection);
}

void
UVAutoRatioPro::RestoreSelection()
{
	MStatus status;

	status = MGlobal::clearSelectionList();
	status = MGlobal::setSelectionMode(m_savedSelectionMode);
	status = MGlobal::setObjectSelectionMask(*m_savedObjectSelectionMask);
	status = MGlobal::setComponentSelectionMask(*m_savedComponentSelectionMask);
	status = MGlobal::setActiveSelectionList(m_savedSelection);
}

void
UVAutoRatioPro::OutputText(const char* text) const
{
#if MAYA60 || MAYA65
	MGlobal::displayInfo(text);
#else
	displayInfo(text);
#endif
}

void
UVAutoRatioPro::OutputText(const MString& text) const
{
#if MAYA60 || MAYA65
	MGlobal::displayInfo(text);
#else
	displayInfo(text);
#endif
}

MStatus
UVAutoRatioPro::redoIt()
{
	MStatus status = MS::kSuccess;

	std::vector<MDGModifier*>::reverse_iterator riter;
	for ( riter = m_undos.rbegin(); riter != m_undos.rend(); ++riter )
	{
		status=(*riter)->doIt();
	}

	return status;
}

MStatus
UVAutoRatioPro::undoIt()
{
	MStatus status = MS::kSuccess;

	std::vector<MDGModifier*>::reverse_iterator riter;
	for ( riter = m_undos.rbegin(); riter != m_undos.rend(); ++riter )
	{
		status=(*riter)->undoIt();
	}

	return MS::kSuccess;
}

