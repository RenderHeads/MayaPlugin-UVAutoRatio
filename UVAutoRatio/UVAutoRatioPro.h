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

#ifndef UVAUTORATIOPRO_H
#define UVAUTORATIOPRO_H

#include <iostream>
#include <vector>
#include "Timer.h"
#include "ShellProcessor.h"

class MeshJob;
class ShellJob;
class Mesh;

struct ValidMesh
{
	MDagPath dagPath;
	std::vector<MObject> components;
	std::vector<int> validShells;

	ValidMesh()
	{
		components.reserve(16);
		validShells.reserve(16);
	}

	inline bool HasShell(int index)
	{
		size_t num = validShells.size();
		for (size_t i = 0; i < num; i++)
		{
			if (validShells[i] == index)
				return true;
		}
		return false;
	}
};

class UVMesh;

class UVAutoRatioPro :
	public MPxCommand
{
public:
	UVAutoRatioPro();
	virtual     ~UVAutoRatioPro();

	// MPxCommand Implemented
	MStatus     doIt (const MArgList& args);
	MStatus     redoIt ();
	MStatus     undoIt ();
	bool        isUndoable() const;
	bool		hasSyntax() const;
	static      void* creator();
	static MSyntax newSyntax();

	static bool		IsProgressCancelled();

	static void		SetNumSubTasks(int tasks, const MString& name);
	static void		StepJobProgress();

	void			TestColourFaces();
	void			ColourFaces(const MDagPath& dagPath, MObject& component, const MString* uvSetName);

private:
	// Startup
	MStatus		Initialise(const MArgList& args);
	const char*	ParseArguments(const MArgList& args);

	MStatus		ProcessMeshes();
	MStatus		BuildDataLists();
	void		GatherData();
	void		FindScales();
	void		LayoutShells();
	void		Normalise();
	void		ApplyScales();
	void		ProcessAsObjectLevel();
	void		ProcessAsUVShellLevel();

	// Helper
	void		DisplayHelp() const;
	void		OutputText(const char* text) const;
	void		OutputText(const MString& text) const;
	void		DisplayStats(bool showErrorsOnly);
	void		DisplayTimingStats();
	void		SaveSelection();
	void		RestoreSelection();
	const char* ErrorToString(JobError error) const;

	// Progress
	static void		UpdateProgress();
	static void		SetProgress(int value);
	

	std::vector<Mesh*>		m_meshes;
	Processor*				m_activeProcessor;

	static int				m_totalJobs;
	static int				m_subTasks, m_subTasksLeft;
	static double			m_subTaskJump;
	static MString			m_subTaskName;
	static double			m_progress, m_progressStep;
	static MString			m_progressMessage;

	// Input parameters
	UVAutoRatioProParams			m_params;

	// Save & Restore selection
	MGlobal::MSelectionMode		m_savedSelectionMode;
	MSelectionMask*				m_savedObjectSelectionMask;
	MSelectionMask*				m_savedComponentSelectionMask;
	MSelectionList				m_savedSelection;

	// Profiling
	Timer		m_timer, m_masterTimer;
	float		m_loadTime, m_gatherTime, m_processTime, m_layoutTime, m_applyTime, m_totalTime;

	// Text scratchpad
	char		m_text[2048];

	// Undo & redo stack
	std::vector<MDGModifier*> m_undos;	// The stack of undo to perform.

	// Help static string data
	static const char* UVAutoRatio_Help[];
	static unsigned int helpLineCount;
};

#endif

