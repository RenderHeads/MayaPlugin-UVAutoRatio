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

#ifndef GETSURFACEUVAREA_H
#define GETSURFACEUVAREA_H

// Given a selection or a selection string argument, this command 
// returns 2 doubles, the surface area and then UV area.
//
// All types of selections are supported, and are converted to faces internally.
class GetSurfaceUVArea :
	public MPxCommand
{
public:
	GetSurfaceUVArea();
	virtual     ~GetSurfaceUVArea();

	MStatus     doIt ( const MArgList& args );
	bool        isUndoable() const;
	bool		hasSyntax() const;
	static void* creator();
	static MSyntax newSyntax();

private:
	MStatus		Initialise(const MArgList& args);
	const char*	ParseArguments(const MArgList& args);
	void		DisplayHelp() const;

	bool		m_isHelp;
	bool		m_isFallback;
	bool		m_isUVSetOverride;
	bool		m_reportZeroAreas;
	MString		m_UVSetName;

	// Help static string data
	static const char* GetSurfaceUVArea_Help[];
	static unsigned int helpLineCount;
};

#endif

