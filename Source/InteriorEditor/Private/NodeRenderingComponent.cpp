// Fill out your copyright notice in the Description page of Project Settings.

#include "InteriorEditorPrivatePCH.h"
#include "NodeRenderingComponent.h"
#include "InteriorNodeActor.h"


enum class ENodeRenderMode {
	Normal,
	Selected,
};

class FNodeSceneProxy: public FDebugRenderSceneProxy
{
public:
	FNodeSceneProxy(
		const UPrimitiveComponent* InComponent,
		const FString& InViewFlagName,
		FVector const& Extent
		);

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) override;
//	virtual void PreRenderView(const FSceneViewFamily * ViewFamily, const uint32 VisibilityMap, int32 FrameNumber) override;
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;

protected:
	static FColor ColorFromMode(ENodeRenderMode mode);

protected:
	FVector Extent;
};


FNodeSceneProxy::FNodeSceneProxy(
	const UPrimitiveComponent* InComponent,
	const FString& InViewFlagName,
	FVector const& Extent
	):
	FDebugRenderSceneProxy(InComponent)
{
	ViewFlagIndex = uint32(FEngineShowFlags::FindIndexByName(*InViewFlagName));
	ViewFlagName = InViewFlagName;

	this->Extent = Extent;
}

FColor FNodeSceneProxy::ColorFromMode(ENodeRenderMode mode)
{
	switch(mode)
	{
		case ENodeRenderMode::Selected:
		return FColor::Yellow;

		default:
		return FColor::White;
	}
}

FPrimitiveViewRelevance FNodeSceneProxy::GetViewRelevance(const FSceneView* View)
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance =
		View->Family->EngineShowFlags.GetSingleFlag(ViewFlagIndex)
		&& IsShown(View)
		//&& SearchingActor->IsSelected()
		;
	Result.bDynamicRelevance = true;
	Result.bNormalTranslucencyRelevance = IsShown(View);
	return Result;
}
/*
void FNodeSceneProxy::PreRenderView(const FSceneViewFamily * ViewFamily, const uint32 VisibilityMap, int32 FrameNumber)
{
	auto mode = IsSelected() ? ENodeRenderMode::Selected : ENodeRenderMode::Normal;	// todo: if more modes???
	Boxes[0].Color = ColorFromMode(mode);

	FDebugRenderSceneProxy::PreRenderView(ViewFamily, VisibilityMap, FrameNumber);
}
*/
void FNodeSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	auto mode = IsSelected() ? ENodeRenderMode::Selected : ENodeRenderMode::Normal;	// todo: if more modes???
	
//	if(AllowDebugViewmodes())	TODO: Maybe needs module dependency
	{
		for(int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if(VisibilityMap & (1 << ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];
				FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

				if(DrawType & SolidAndWireMeshes || DrawType & WireMesh)
				{
					auto Box = FBox{ GetActorPosition() - Extent * 0.5f, GetActorPosition() + Extent * 0.5f };
					auto Color = ColorFromMode(mode);
					DrawWireBox(PDI, Box, Color, SDPG_World, DrawType & SolidAndWireMeshes ? 2 : 0, 0, true);
				}
/*				if(DrawType & SolidAndWireMeshes || DrawType & SolidMesh)
				{
					GetBoxMesh(FTransform(Box.Box.GetCenter()).ToMatrixNoScale(), Box.Box.GetExtent(), MaterialCache[Box.Color.WithAlpha(60)], SDPG_World, ViewIndex, Collector);
				}
*/			}
		}
	}
}


UNodeRenderingComponent::UNodeRenderingComponent(FObjectInitializer const& OI):
Super(OI)
{
}

FPrimitiveSceneProxy* UNodeRenderingComponent::CreateSceneProxy()
{
	auto Node = Cast< AInteriorNodeActor >(GetOwner());
	if(Node)
	{
		return new FNodeSceneProxy(
			this,
			"GameplayDebug",
			Node->Extent
			);
	}
	else
	{
		return nullptr;
	}
}

FBoxSphereBounds UNodeRenderingComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	auto Node = Cast< AInteriorNodeActor >(GetOwner());
	FBox BoundingBox(Node->GetMin(), Node->GetMax());
	return FBoxSphereBounds(BoundingBox);
}

void UNodeRenderingComponent::CreateRenderState_Concurrent()
{
	Super::CreateRenderState_Concurrent();

	if(SceneProxy)
	{
		static_cast<FNodeSceneProxy*>(SceneProxy)->RegisterDebugDrawDelgate();
	}
}

void UNodeRenderingComponent::DestroyRenderState_Concurrent()
{
	if(SceneProxy)
	{
		static_cast<FNodeSceneProxy*>(SceneProxy)->UnregisterDebugDrawDelgate();
	}

	Super::DestroyRenderState_Concurrent();
}



