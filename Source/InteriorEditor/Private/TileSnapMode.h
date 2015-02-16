// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UnrealEd.h"
#include "Editor.h"


/*
Editor mode for creating and modifying interiors
*/
class FTileSnapMode: public FEdMode
{
public:
	static const FName ModeId;

	FTileSnapMode();

public:
	// FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

public:
	/*
	FEdMode overrides
	*/
	virtual void Enter() override;
	virtual void Exit() override;

	virtual bool InputKey(
		FEditorViewportClient* ViewportClient,
		FViewport* Viewport,
		FKey Key,
		EInputEvent Event
		) override;
	virtual bool InputDelta(
		FEditorViewportClient* InViewportClient,
		FViewport* InViewport,
		FVector& InDrag,
		FRotator& InRot,
		FVector& InScale
		) override;
		
	virtual void DrawHUD(
		FEditorViewportClient* ViewportClient,
		FViewport* Viewport,
		const FSceneView* View,
		FCanvas* Canvas
		) override;

protected:

public:
//	class UInteriorEditorModeSettings* Settings;
};


