// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/PrimitiveComponent.h"
#include "PrimitiveSceneProxy.h"

#include "InteriorMeshRenderingComponent.generated.h"


/**
 * 
 */
UCLASS(hidecategories = Object)
class UInteriorMeshRenderingComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	UInteriorMeshRenderingComponent(FObjectInitializer const& OI);

public:

public:
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform &LocalToWorld) const override;
	virtual void CreateRenderState_Concurrent() override;
	virtual void DestroyRenderState_Concurrent() override;
};
