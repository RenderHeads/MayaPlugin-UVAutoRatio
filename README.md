# UV Auto Ratio
![UI](https://github.com/RenderHeads/MayaPlugin-UVAutoRatio/blob/master/docs/rtt_examplecar_small.jpg "UVAudioRatio")
###### Plugin for Autodesk Maya for unifying texture area usage.

## Introduction

UVAutoRatio was a Maya plugin developed by RenderHeads Ltd between 2007-2013 and open-sourced in 2015.  The software is no longer supported or developed by RenderHeads Ltd and this serves as a historic archive.

Artists in the film and games industry spend hours manually scaling the texture coordinates of models to get them in the right proportion. This plugin completely automates this process but still leaves the artist in control.

To get the most out of textures they should be mapped in proportion to the size of the geometry. Not doing so means that different parts of the scene will have different texture distribution, resulting in some textures appearing blurrier than others while some will be overly sharp, making the scene appear inconsistent and strange. Those objects with too much texture mapped to them waste memory and cause aliasing (flickering) due to the overly high frequency of texels.

Artists in the game and film industry can spend hours scaling UVs up and down manually trying to get them to the ideal proportion. This isn’t a particularly fun job, in fact it’s not something we should be wasting our time with. Wouldn’t it be great if this tiresome task was automated?

Enter UVAutoRatio Pro: the complete tool for automating all texel ratio tasks.

## Features

+ Automatically scale UVs based on surface area
+ Fast and accurate processing algorithm
+ Ability to specify UV-set
+ Operates on UV shells
+ Automatically moves overlapping UV shells
+ Normalise across multiple objects
+ UV shell finder tool
+ Ratio inspector tool
+ Maintains settings between sessions

![UI](https://github.com/RenderHeads/MayaPlugin-UVAutoRatio/blob/master/docs/UITabsExpanded_small.png "UI")

## Links

+ [PDF docs](https://github.com/RenderHeads/MayaPlugin-UVAutoRatio/releases/download/2015-2.5.x/UVAutoRatio2.0ProDocumentation.pdf)
+ [Tutorial video](https://vimeo.com/856301)
+ [Cached website 2011](https://web.archive.org/web/20111205111334/http://www.renderheads.com/portfolio/UVAutoRatio/)

## Open Source

After selling this software for many years, we decided to stop developing it, so we released it as an open source project.

+ Written in C++
+ Maya scripts written in MEL
+ Builds for Windows, macOS and Linux

If you are prompted for a license key, you can use the following:

`contact@renderheads.com-(0 user license)-RenderHeads-RenderHeads-UVAutoRatio 2.0 Pro-5A5W-2K55-6J2Z-343C-5A5W-2K55-4R2Z-343C-2K4N-0E1M-3G3C-1N4H`

## Maya SDK Setup

The core Maya SDK must be available to the plugin in order to compile.  The 3 folders needed are:
 + include
 + lib
 + lib64

In order to build for multiple versions of Maya we use the following directory structure:

	MayaSDK/
		Common/
		Windows/
		MacOSX/
		Linux/

The Common folder contains all of the header files (contents of include folder), and the platform folders contain all the of the platform specific libraries (lib and lib64 folder).

Each sub-folder starts with the Maya version number.

For example:

	MayaSDK/
		Common/
			6.0/
				maya/
			8.5/
				maya/
			2011/
				maya/
				tbb/
		Windows/
			6.0/
				lib/
			2008/
				lib/
				lib64/
			2011/
				lib/
				lib64/
		MacOSX/
		Linux/

In Windows set the environment variable MAYALIBBASE to the path of your MayaSDK folder.  After adding the environment variable you may need to reopen Visual Studio.

## Compiling for Windows

+ The project solution is created with Visual Studio 2008.
+ Make sure you have set up your Maya SDK files as specified above
+ Each version of Maya has its own solution configuration. 32-bit and 64-bit cpu targets can be selected from the solution platform list.
+ Hit 'Build' and it should build a plugin for the current configuration.

#### Add support for additional Maya versions
To add a new configuration for another version of Maya:
+ Right-click on the solution and open Configuration Manager
+ In "Action solution configuration" select "New"
+ Select the latest release version of Maya from the list of "Copy settings from"
+ Make the name the same format but replace the version number
+ Make sure that the 32 and 64-bit settings are correct

+ Once the new configuration is created you'll need to configure the project for that configuration:
	+ Right-click on the UVAutoRatio project and go to Properties
	+ Select C/C++ > General
		+ Edit "Additional Include Directories" to point the location of that versions include files.  For example in Maya 2011 it would read "$(MAYALIBBASE)\Common\2011;"
	+ Select C/C++ > Preprocessor
		+ Make sure the list contains an entry for your version of Maya.  For 2011 use "MAYA2011".  This preprocessor is used later when including the Maya SDK headers to exclude or include certain features based on the version we're building for.
	+ Select Linker > General 
		+ Set the Output File property to match your Maya version number.  For 2011 it should read "$(OutDir)\UVAutoRatioPro2011.mll"
		+ Set Additional Library Directories to point to the relevent Maya SDK folder.  
			For Maya 2011 32-bit it would read "$(OutDir)";"$(MAYALIBBASE)\Windows\2011\lib";
			For Maya 2011 64-bit it would read "$(OutDir)";"$(MAYALIBBASE)\Windows\2011\lib64";

## Compiling for MacOSX

+ Use XCode to load the project "uvAutoRatioPro.xcodeproj"

## Compiling for Linux

+ See build.sh and buildall.sh
+ Use 'make'
+ The build script needs the following variales defines:
	+ export ARCH=64		(for 64-bit building)
	+ export MAYAVER=2011		(this is the maya version number, eg 8.5)
	+ export MAYAVERs=2011		(this is the string version of the Maya version, so Maya 8.5 would be 85)

## Packaging for Windows

+ We build to a setup executable
+ Install NSIS v2.46 with the FindProcDLL plugin (http://nsis.sourceforge.net/FindProcDLL_plug-in)
+ Open InstallScript.NSI and increment the version numbers at the top to match the new build version number
+ Right-click the InstallScript.NSI and compile with NSI
+ The resulting installer file should appear in the Releases folder

NOTE: If the installer fails to run, it may be because using a mapped drive

## Packaging for OSX and Linux

+ There is no setup, simply ZIP up all the files in the module folder to an appropriatly named ZIP file.
