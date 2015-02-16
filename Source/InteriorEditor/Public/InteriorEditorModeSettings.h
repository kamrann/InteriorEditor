// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "InteriorEditorModeSettings.generated.h"


/*
UObject used to store the persistent settings for the editor mode itself (as opposed to anything that
persists within the object instance(s) being edited).
*/
UCLASS()
class UInteriorEditorModeSettings: public UObject
{
	GENERATED_BODY()

public:
	class FInteriorEditorMode* ParentMode;

public:
	UPROPERTY(Category = "Test Category", EditAnywhere, NonTransactional, meta = (ShowForTools = "Paint,Sculpt,Smooth,Flatten,Erosion,HydraErosion,Noise,Mask,CopyPaste", ClampMin = "0", ClampMax = "10", UIMin = "0", UIMax = "1"))
	float TestModeProperty;

public:
	void SetParent(class FInteriorEditorMode* Mode)
	{
		ParentMode = Mode;
	}

public:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
};


