// Fill out your copyright notice in the Description page of Project Settings.

#include "InteriorEditorPrivatePCH.h"
#include "TileSnapMode.h"


const FName FTileSnapMode::ModeId = TEXT("TileSnap");

FTileSnapMode::FTileSnapMode()
{
/*	Settings = ConstructObject< UInteriorEditorModeSettings >(
		UInteriorEditorModeSettings::StaticClass(), GetTransientPackage(), NAME_None, RF_Transactional);
	Settings->SetParent(this);
	*/
}

void FTileSnapMode::AddReferencedObjects(FReferenceCollector& Collector)
{
	// Call parent implementation
	FEdMode::AddReferencedObjects(Collector);

//	Collector.AddReferencedObject(Settings);
}


void FTileSnapMode::Enter()
{
	FEdMode::Enter();
}

void FTileSnapMode::Exit()
{
	FEdMode::Exit();
}

bool FTileSnapMode::InputKey(
	FEditorViewportClient* ViewportClient,
	FViewport* Viewport,
	FKey Key,
	EInputEvent Event
	)
{
	return FEdMode::InputKey(ViewportClient, Viewport, Key, Event);
}

bool FTileSnapMode::InputDelta(
	FEditorViewportClient* InViewportClient,
	FViewport* InViewport,
	FVector& InDrag,
	FRotator& InRot,
	FVector& InScale
	)
{
	return FEdMode::InputDelta(InViewportClient, InViewport, InDrag, InRot, InScale);
}

void FTileSnapMode::DrawHUD(
	FEditorViewportClient* ViewportClient,
	FViewport* Viewport,
	const FSceneView* View,
	FCanvas* Canvas
	)
{
	FEdMode::DrawHUD(ViewportClient, Viewport, View, Canvas);
}

/*
FInteriorEditorMode::FNodeList FInteriorEditorMode::GetSelectedNodes() const
{
	// TODO: May be better to just use this->Nodes and AActor::IsSelected?
	FNodeList Sel;
	GEditor->GetSelectedActors()->GetSelectedObjects< AInteriorNodeActor >(Sel);
	return Sel;
}

void FInteriorEditorMode::TranslateNodes(FNodeList const& nodes, FVector const& offset)
{
	for(auto node : nodes)
	{
		auto current = node->GetActorLocation();
		node->SetActorLocation(current + offset);
		node->Modify();
		node->PostEditMove(true);
	}
}
*/