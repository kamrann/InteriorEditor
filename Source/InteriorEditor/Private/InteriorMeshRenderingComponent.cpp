// Fill out your copyright notice in the Description page of Project Settings.

#include "InteriorEditorPrivatePCH.h"
#include "InteriorMeshRenderingComponent.h"
#include "InteriorGraphInstance.h"
#include "InteriorEditorMode.h" 

#include <algorithm>


class FGraphMeshSceneProxy: public FPrimitiveSceneProxy
{
public:
	FGraphMeshSceneProxy(
		const UPrimitiveComponent* InComponent
		);

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) override;
	virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) const override;

protected:
	FRawMesh Mesh;
};


FGraphMeshSceneProxy::FGraphMeshSceneProxy(
	const UPrimitiveComponent* InComponent
	):
	FPrimitiveSceneProxy(InComponent)
{
	auto RenderComp = Cast< UInteriorMeshRenderingComponent >(InComponent);
	if(RenderComp)
	{
		auto Graph = Cast< AInteriorGraphActor >(RenderComp->GetOwner());

		// store instance id in render component
		// store instance pointers inside graph actor
		// generate raw mesh here and store, then render in DrawStaticElements()
	}
}

FPrimitiveViewRelevance FGraphMeshSceneProxy::GetViewRelevance(const FSceneView* View)
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View);
	Result.bStaticRelevance = true;
	Result.bDynamicRelevance = false;
	Result.bNormalTranslucencyRelevance = IsShown(View);
	return Result;
}

void FGraphMeshSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) const
{

}


UInteriorMeshRenderingComponent::UInteriorMeshRenderingComponent(FObjectInitializer const& OI):
Super(OI)
{
}

FPrimitiveSceneProxy* UInteriorMeshRenderingComponent::CreateSceneProxy()
{
	auto Graph = Cast< AInteriorGraphActor >(GetOwner());
	if(Graph)
	{
		return new FGraphMeshSceneProxy(this);
	}
	else
	{
		return nullptr;
	}
}

FBoxSphereBounds UInteriorMeshRenderingComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	// TODO:
	static FBox BoundingBox(FVector(-HALF_WORLD_MAX), FVector(HALF_WORLD_MAX));
	return FBoxSphereBounds(BoundingBox);
}

void UInteriorMeshRenderingComponent::CreateRenderState_Concurrent()
{
	Super::CreateRenderState_Concurrent();

	if(SceneProxy)
	{
		static_cast<FGraphMeshSceneProxy*>(SceneProxy)->RegisterDebugDrawDelgate();
	}
}

void UInteriorMeshRenderingComponent::DestroyRenderState_Concurrent()
{
	if(SceneProxy)
	{
		static_cast<FGraphMeshSceneProxy*>(SceneProxy)->UnregisterDebugDrawDelgate();
	}

	Super::DestroyRenderState_Concurrent();
}



