// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "InteriorEditorNodeFace.h"
#include "InteriorFaceSelectionProxy.generated.h"


UCLASS(Transient)
class UInteriorFaceSelectionProxy: public UObject
{
	GENERATED_BODY()

public:
	UInteriorFaceSelectionProxy();

public:
	UPROPERTY(EditInstanceOnly, Category = "Face")
	float AxisValue;

public:
	bool IsValid() const;
	FNodeFaceRef GetFace() const;
	void SetFace(FNodeFaceRef F);

	inline bool IsFace(FNodeFaceRef F) const
	{
		return Face == F;
	}

public:
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

protected:
	FNodeFaceRef Face;
};



