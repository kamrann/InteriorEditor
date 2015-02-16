
#pragma once

#include "Editor/EditorStyle/Public/EditorStyleSet.h"


/**
 * Interior editor actions
 */
class FInteriorEditorCommands: public TCommands< FInteriorEditorCommands >
{
public:
	FInteriorEditorCommands(): TCommands< FInteriorEditorCommands >
	(
		"InteriorEditor", // Context name for fast lookup
		NSLOCTEXT("Contexts", "InteriorEditor", "Interior Editor"), // Localized context name for displaying
		NAME_None, //"LevelEditor" // Parent
		FEditorStyle::GetStyleSetName() // Icon Style Set
	)
	{
	}
	
	/**
	 * Initialize commands
	 */
	virtual void RegisterCommands() override;

public:
	// Mode Switch
	TSharedPtr< FUICommandInfo > TestCommand;

	// Tools
//	TSharedPtr< FUICommandInfo > NewLandscape;

	// Map
	TMap< FName, TSharedPtr< FUICommandInfo > > NameToCommandMap;
};

///**
// * Implementation of various level editor action callback functions
// */
//class FLevelEditorActionCallbacks
//{
//public:
//};
