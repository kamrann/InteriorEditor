// Fill out your copyright notice in the Description page of Project Settings.

#include "InteriorEditorPrivatePCH.h"
#include "InteriorEditor.h"


void FInteriorEditorModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("%s"), TEXT("InteriorEditor module startup"));

	FEditorModeRegistry::Get().RegisterMode< >();
}

void FInteriorEditorModule::ShutdownModule()
{

}


IMPLEMENT_MODULE(FInteriorEditorModule, InteriorEditor)


