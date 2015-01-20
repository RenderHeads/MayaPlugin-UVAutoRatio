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
#include "GetUVShellSelectionStrings.h"
#include "GetSurfaceUVArea.h"
#include "UVAutoRatioPro.h"
#include "UVAutoRatioPlugin.h"

UVAutoRatioPlugin::UVAutoRatioPlugin(const MayaPluginParams& params) : MayaPlugin(params)
{
	m_UVAutoRatioProCreated = false;
	m_GetSurfaceUVAreaCreated = false;
	m_GetUVShellSelectionStringsCreated = false;
}

void
UVAutoRatioPlugin::LoadUserPlugins(MFnPlugin& plugin)
{
	MStatus status;

	// Register the command
	if (!m_UVAutoRatioProCreated)
	{
		status = plugin.registerCommand("UVAutoRatioPro", UVAutoRatioPro::creator,UVAutoRatioPro::newSyntax);
		if (MStatus::kSuccess != status)
		{
			status.perror("registerCommand UVAutoRatioPro failed");
		}
		m_UVAutoRatioProCreated = true;
	}

	// Register the command
	if (!m_GetSurfaceUVAreaCreated)
	{
		status = plugin.registerCommand("GetSurfaceUVArea", GetSurfaceUVArea::creator, GetSurfaceUVArea::newSyntax);
		if (MStatus::kSuccess != status)
		{
			status.perror("registerCommand GetSurfaceUVArea failed");
		}
		m_GetSurfaceUVAreaCreated = true;
	}

	// Register the command
	if (!m_GetUVShellSelectionStringsCreated)
	{
		status = plugin.registerCommand("GetUVShellSelectionStrings", GetUVShellSelectionStrings::creator);
		if (MStatus::kSuccess != status)
		{
			status.perror("registerCommand GetUVShellSelectionStrings failed");
		}
		m_GetUVShellSelectionStringsCreated = true;
	}

	//
	//status = plugin.registerNode( Cas_checkerBoxNode::typeName, Cas_checkerBoxNode::typeId, Cas_checkerBoxNode::creator	, Cas_checkerBoxNode::initialize ,	MPxNode::kLocatorNode );
	//MCheckStatus(status,"registerCommand Cas_checkerBoxNode failed");
}

void
UVAutoRatioPlugin::UnloadUserPlugins(MFnPlugin& plugin)
{
	MStatus status;

	// Unregister command
	if (m_GetUVShellSelectionStringsCreated)
	{
		status = plugin.deregisterCommand("GetUVShellSelectionStrings");
		if (MStatus::kSuccess != status)
		{
			status.perror("deregisterCommand GetUVShellSelectionStrings failed");
		}
		m_GetUVShellSelectionStringsCreated = false;
	}

	// Unregister command
	if (m_GetSurfaceUVAreaCreated)
	{
		status = plugin.deregisterCommand("GetSurfaceUVArea");
		if (MStatus::kSuccess != status)
		{
			status.perror("deregisterCommand GetSurfaceUVArea failed");
		}
		m_GetSurfaceUVAreaCreated = false;
	}

	// Unregister command
	if (m_UVAutoRatioProCreated)
	{
		status = plugin.deregisterCommand("UVAutoRatioPro");
		if (MStatus::kSuccess != status)
		{
			status.perror("deregisterCommand UVAutoRatioPro failed");
		}
		m_UVAutoRatioProCreated = false;
	}
}
