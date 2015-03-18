// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "InteriorEditorPrivatePCH.h"
#include "InteriorEditorCommands.h"


void FInteriorEditorCommands::RegisterCommands()
{
	// TODO: Too many strings! Find out what they're each used for

	UI_COMMAND(DefaultMode, "DefaultMode", "", EUserInterfaceActionType::RadioButton, FInputGesture());
	NameToCommandMap.Add("DefaultMode", DefaultMode);

	UI_COMMAND(GenMesh, "GenMesh", "", EUserInterfaceActionType::Button, FInputGesture());
	NameToCommandMap.Add("GenMesh", GenMesh);

//	UI_COMMAND(NewLandscape, "Tool - New Landscape", "", EUserInterfaceActionType::RadioButton, FInputGesture());
//	NameToCommandMap.Add("Tool_NewLandscape", NewLandscape);

/*	this stuff was for the viewport menu extension
	UI_COMMAND(ViewModeNormal, "Normal", "", EUserInterfaceActionType::RadioButton, FInputGesture());
	UI_COMMAND(ViewModeLOD, "LOD", "", EUserInterfaceActionType::RadioButton, FInputGesture());
	UI_COMMAND(ViewModeLayerDensity, "Layer Density", "", EUserInterfaceActionType::RadioButton, FInputGesture());
	UI_COMMAND(ViewModeLayerDebug, "Layer Debug", "", EUserInterfaceActionType::RadioButton, FInputGesture());
	UI_COMMAND(ViewModeWireframeOnTop, "Wireframe on Top", "", EUserInterfaceActionType::RadioButton, FInputGesture());
*/
}
