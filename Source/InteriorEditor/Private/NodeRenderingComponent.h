// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/PrimitiveComponent.h"
#include "DebugRenderSceneProxy.h"

#include "NodeRenderingComponent.generated.h"


/**
 * 
 */
UCLASS(hidecategories = Object)
class UNodeRenderingComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	UNodeRenderingComponent(FObjectInitializer const& OI);

public:

public:
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform &LocalToWorld) const override;
	virtual void CreateRenderState_Concurrent() override;
	virtual void DestroyRenderState_Concurrent() override;
};
