// Fill out your copyright notice in the Description page of Project Settings.

#include "InteriorEditorPrivatePCH.h"
#include "InteriorEditor.h"
#include "InteriorEditorMode.h"
#include "InteriorEditorCommands.h"
#include "GraphDetailsCustomization.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"


void FInteriorEditorModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("%s"), TEXT("InteriorEditor module startup"));

	FInteriorEditorCommands::Register();

	FEditorModeRegistry::Get().RegisterMode< FInteriorEditorMode >(
		FInteriorEditorMode::ModeId,
		FText::FromString(TEXT("Interior")),
		FSlateIcon{},
		true
		);

	// Register detail customizations
	{
		auto& PropertyModule = FModuleManager::LoadModuleChecked< FPropertyEditorModule >("PropertyEditor");
		PropertyModule.RegisterCustomClassLayout(
			"InteriorGraphActor",
			FOnGetDetailCustomizationInstance::CreateStatic(&FInteriorGraphDetailsCustomization::MakeInstance)
			);

		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

void FInteriorEditorModule::ShutdownModule()
{
	FEditorModeRegistry::Get().UnregisterMode(
		FInteriorEditorMode::ModeId
		);
}


IMPLEMENT_MODULE(FInteriorEditorModule, InteriorEditor)


