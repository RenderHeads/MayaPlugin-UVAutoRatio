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

#ifndef UVAUTORATIOPLUGIN_H
#define UVAUTORATIOPLUGIN_H

#include "MayaPlugin.h"

class MFnPlugin;

class UVAutoRatioPlugin : public MayaPlugin
{
public:
	UVAutoRatioPlugin(const MayaPluginParams& params);

protected:
	void	LoadUserPlugins(MFnPlugin& plugin);
	void	UnloadUserPlugins(MFnPlugin& plugin);

private:
	bool			m_GetUVShellSelectionStringsCreated;
	bool			m_GetSurfaceUVAreaCreated;
	bool			m_UVAutoRatioProCreated;
};

#endif

