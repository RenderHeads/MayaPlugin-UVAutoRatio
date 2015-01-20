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

#ifndef GETUVSHELLSELECTIONSTRINGS_H
#define GETUVSHELLSELECTIONSTRINGS_H

// Return discreet strings for all the selection strings that make up UV shells.
// So if there are 2 shells, and each shell needs multiple things to define their selection,
// It will return a string array of 2 strings, where each string is a space separated
// list of the selection elements
class GetUVShellSelectionStrings :
	public MPxCommand
{
public:
	GetUVShellSelectionStrings();
	virtual     ~GetUVShellSelectionStrings();

	MStatus     doIt ( const MArgList& args );
	bool        isUndoable() const;
	bool		hasSyntax() const;
	static      void* creator();

private:
};

#endif

