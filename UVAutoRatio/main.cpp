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
#include "UVAutoRatioPlugin.h"

MayaPlugin* UvArPlugin = NULL;

MStatus
initializePlugin(MObject obj)
{
	MayaPluginParams params;
	params.authorString = "RenderHeads Ltd";
	params.productName = "UVAutoRatio 2.0 Pro";
	params.productShortName = "UVAutoRatioPro";
	params.versionString = "2.6.0 Pro";
	UvArPlugin = new UVAutoRatioPlugin(params);

	UvArPlugin->Load(obj);

	return MStatus::kSuccess;
}

MStatus
uninitializePlugin(MObject obj)
{
	if (UvArPlugin)
	{
		UvArPlugin->Unload(obj);
		delete UvArPlugin;
		UvArPlugin = NULL;
	}

	return MStatus::kSuccess;
}