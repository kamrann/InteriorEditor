// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "InteriorEditorPrivatePCH.h"
#include "InteriorEditorMode.h"
#include "SInteriorEditor.h"
#include "InteriorEditorCommands.h"
//#include "AssetThumbnail.h"
#include "InteriorEditorModeSettings.h"
#include "InteriorGraphActor.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"

#define LOCTEXT_NAMESPACE "InteriorEditor"


void FInteriorToolKit::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
}

void FInteriorToolKit::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
}

void FInteriorToolKit::Init(const TSharedPtr< class IToolkitHost >& InitToolkitHost)
{
	Widget = SNew(SInteriorEditor, SharedThis(this));

	FModeToolkit::Init(InitToolkitHost);
}

FName FInteriorToolKit::GetToolkitFName() const
{
	return FName("InteriorEditor");
}

FText FInteriorToolKit::GetBaseToolkitName() const
{
	return LOCTEXT("ToolkitName", "Interior Editor");
}

class FInteriorEditorMode* FInteriorToolKit::GetEditorMode() const
{
	// TODO: Can we safely just delegate to Widget->GetEditorMode(), or might this method be called before
	// the widget is created inside Init()??
	return (FInteriorEditorMode*)GLevelEditorModeTools().GetActiveMode(FInteriorEditorMode::ModeId);
}

TSharedPtr<SWidget> FInteriorToolKit::GetInlineContent() const
{
	return Widget;
}

void FInteriorToolKit::NotifyToolChanged()
{
	Widget->NotifyToolChanged();
}

void FInteriorToolKit::NotifyBrushChanged()
{
	Widget->NotifyBrushChanged();
}

//////////////////////////////////////////////////////////////////////////

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SInteriorEditor::Construct(const FArguments& InArgs, TSharedRef<FInteriorToolKit> InParentToolkit)
{
	CommandList = InParentToolkit->GetToolkitCommands();

	// Modes:
/*	CommandList->MapAction(
		FInteriorEditorCommands::Get().GenMesh,
		FUIAction(FExecuteAction::CreateSP(this, &SInteriorEditor::OnGenerateMesh))
		);
		*/
/*	CommandList->MapAction(FInteriorEditorCommands::Get().SculptMode, FUIAction(FExecuteAction::CreateSP(this, &SInteriorEditor::OnChangeMode, FName("ToolMode_Sculpt")), FCanExecuteAction::CreateSP(this, &SInteriorEditor::IsModeEnabled, FName(TEXT("ToolMode_Sculpt"))), FIsActionChecked::CreateSP(this, &SInteriorEditor::IsModeActive, FName(TEXT("ToolMode_Sculpt")))));
	CommandList->MapAction(FInteriorEditorCommands::Get().PaintMode,  FUIAction(FExecuteAction::CreateSP(this, &SInteriorEditor::OnChangeMode, FName("ToolMode_Paint" )), FCanExecuteAction::CreateSP(this, &SInteriorEditor::IsModeEnabled, FName(TEXT("ToolMode_Paint" ))), FIsActionChecked::CreateSP(this, &SInteriorEditor::IsModeActive, FName(TEXT("ToolMode_Paint" )))));
	*/
	FToolBarBuilder ModeSwitchButtons(CommandList, FMultiBoxCustomization::None);
	{
		ModeSwitchButtons.AddToolBarButton(FInteriorEditorCommands::Get().DefaultMode, NAME_None, LOCTEXT("Mode.Default", "Default Mode"), LOCTEXT("Mode.Default.Tooltip", "Default Interior Editing Mode"));
		//LOCTEXT("Mode.Default.Tooltip", "Generate a static mesh from the graph")
//		ModeSwitchButtons.AddToolBarButton(FInteriorEditorCommands::Get().SculptMode, NAME_None, LOCTEXT("Mode.Sculpt", "Sculpt"), LOCTEXT("Mode.Sculpt.Tooltip", "Contains tools that modify the shape of a Interior"));
//		ModeSwitchButtons.AddToolBarButton(FInteriorEditorCommands::Get().PaintMode,  NAME_None, LOCTEXT("Mode.Paint",  "Paint"),  LOCTEXT("Mode.Paint.Tooltip",  "Contains tools that paint materials on to a Interior"));
	}

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked< FPropertyEditorModule >("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs(false, false, false, FDetailsViewArgs::HideNameArea);

	DetailsPanel = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsPanel->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &SInteriorEditor::GetIsPropertyVisible));

	auto Mode = GetEditorMode();
	if(Mode)
	{
		DetailsPanel->SetObject(Mode->Settings);
	}

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 5)
		[
			SAssignNew(Error, SErrorText)
		]
		+ SVerticalBox::Slot()
		.Padding(0)
		[
			SNew(SVerticalBox)
			//.IsEnabled(this, &SInteriorEditor::GetInteriorEditorIsEnabled)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4, 0, 4, 5)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
					.HAlign(HAlign_Center)
					[
						ModeSwitchButtons.MakeWidget()
					]
				]
			]
			+ SVerticalBox::Slot()
			.Padding(0)
			[
				DetailsPanel.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.Padding(0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(FString("Generate Static Mesh"))
					.OnClicked_Raw(this, &SInteriorEditor::OnGenerateMesh)
				]
			]
			+ SVerticalBox::Slot()
			.Padding(0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(FString("Test"))
					.OnClicked_Raw(this, &SInteriorEditor::OnTestClicked)
				]
			]
		]
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

class FInteriorEditorMode* SInteriorEditor::GetEditorMode() const
{
	return (FInteriorEditorMode*)GLevelEditorModeTools().GetActiveMode(FInteriorEditorMode::ModeId);
}

FText SInteriorEditor::GetErrorText() const
{
	/* TODO:
	const FEdModeInterior* InteriorEdMode = GetEditorMode();
	EInteriorEditingState EditState = InteriorEdMode->GetEditingState();
	switch (EditState)
	{
		case EInteriorEditingState::SIEWorld:
		{

			if (InteriorEdMode->NewInteriorPreviewMode != ENewInteriorPreviewMode::None)
			{
				return LOCTEXT("IsSimulatingError_create", "Can't create Interior while simulating!");
			}
			else
			{
				return LOCTEXT("IsSimulatingError_edit", "Can't edit Interior while simulating!");
			}
			break;
		}
		case EInteriorEditingState::PIEWorld:
		{
			if (InteriorEdMode->NewInteriorPreviewMode != ENewInteriorPreviewMode::None)
			{
				return LOCTEXT("IsPIEError_create", "Can't create Interior in PIE!");
			}
			else
			{
				return LOCTEXT("IsPIEError_edit", "Can't edit Interior in PIE!");
			}
			break;
		}
		case EInteriorEditingState::BadFeatureLevel:
		{
			if (InteriorEdMode->NewInteriorPreviewMode != ENewInteriorPreviewMode::None)
			{
				return LOCTEXT("IsFLError_create", "Can't create Interior with a feature level less than SM4!");
			}
			else
			{
				return LOCTEXT("IsFLError_edit", "Can't edit Interior with a feature level less than SM4!");
			}
			break;
		}
		case EInteriorEditingState::NoInterior:
		{
			return LOCTEXT("NoInteriorError", "No Interior!");
		}
		case EInteriorEditingState::Enabled:
		{
			return FText::GetEmpty();
		}
		default:
			checkNoEntry();
	}
	*/
	return FText::GetEmpty();
}
/*
bool SInteriorEditor::GetInteriorEditorIsEnabled() const
{
	auto Mode = GetEditorMode();
	if (Mode)
	{
		Error->SetError(GetErrorText());
		return Mode->GetEditingState() == EInteriorEditingState::Enabled;
	}
	return false;
}
*/
bool SInteriorEditor::GetIsPropertyVisible(const FPropertyAndParent& PropertyAndParent) const
{
	const UProperty& Property = PropertyAndParent.Property;

	auto Mode = GetEditorMode();
	if (Mode != nullptr )//&& Mode->CurrentTool != nullptr)
	{
/*		if (Property.HasMetaData("ShowForTools"))
		{
			const FName CurrentToolName = Mode->CurrentTool->GetToolName();

			TArray<FString> ShowForTools;
			Property.GetMetaData("ShowForTools").ParseIntoArray(&ShowForTools, TEXT(","), true);
			if (!ShowForTools.Contains(CurrentToolName.ToString()))
			{
				return false;
			}
		}
*/		return true;
	}

	return false;
}

/*void*/ FReply SInteriorEditor::OnGenerateMesh()
{
	auto Mode = GetEditorMode();
	if(Mode)
	{
		Mode->GenerateStaticMesh();
	}

	return FReply::Handled();
}

FReply SInteriorEditor::OnTestClicked()
{
	auto Mode = GetEditorMode();
	if(Mode)
	{
		if(Mode->Graph)
		{
			GEditor->SelectActor(Mode->Graph, true, true, true);
		}
	}

	return FReply::Handled();
}

/*
void SInteriorEditor::OnChangeMode(FName ModeName)
{
	auto Mode = GetEditorMode();
	if(Mode)
	{
		Mode->SetCurrentToolMode(ModeName);
	}
}

bool SInteriorEditor::IsModeEnabled(FName ModeName) const
{
	auto Mode = GetEditorMode();
	if(Mode)
	{
		// Manage is the only mode enabled if we have no Interior
		if (ModeName == "ToolMode_Manage" || Mode->GetInteriorList().Num() > 0)
		{
			return true;
		}
	}

	return false;
}

bool SInteriorEditor::IsModeActive(FName ModeName) const
{
	auto Mode = GetEditorMode();
	if(Mode && Mode->CurrentTool)
	{
		return Mode->CurrentToolMode->ToolModeName == ModeName;
	}

	return false;
}
*/
void SInteriorEditor::NotifyToolChanged()
{
	auto Mode = GetEditorMode();
	if(Mode)
	{
		// Refresh details panel
		DetailsPanel->SetObject(Mode->Settings, true);
	}
}

void SInteriorEditor::NotifyBrushChanged()
{
	auto Mode = GetEditorMode();
	if(Mode)
	{
		// Refresh details panel
		DetailsPanel->SetObject(Mode->Settings, true);
	}
}


#undef LOCTEXT_NAMESPACE


