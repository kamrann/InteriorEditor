// Fill out your copyright notice in the Description page of Project Settings.

#include "InteriorEditorPrivatePCH.h"
#include "ConnectionRenderingComponent.h"
#include "InteriorConnectionActor.h"


class FConnectionSceneProxy: public FDebugRenderSceneProxy
{
public:
	FConnectionSceneProxy(
		const UPrimitiveComponent* InComponent,
		const FString& InViewFlagName,
		FVector const& Extent
		);

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) override;
//	virtual void PreRenderView(const FSceneViewFamily * ViewFamily, const uint32 VisibilityMap, int32 FrameNumber) override;
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;

protected:
	FVector Extent;
};


FConnectionSceneProxy::FConnectionSceneProxy(
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

FPrimitiveViewRelevance FConnectionSceneProxy::GetViewRelevance(const FSceneView* View)
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

void FConnectionSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
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
					auto Color = FColor::Green;
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


UConnectionRenderingComponent::UConnectionRenderingComponent(FObjectInitializer const& OI):
Super(OI)
{
}

FPrimitiveSceneProxy* UConnectionRenderingComponent::CreateSceneProxy()
{
	auto Conn = Cast< AInteriorConnectionActor >(GetOwner());
	if(Conn)
	{
		return new FConnectionSceneProxy(
			this,
			"GameplayDebug",
			Conn->Extent
			);
	}
	else
	{
		return nullptr;
	}
}

FBoxSphereBounds UConnectionRenderingComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	auto Conn = Cast< AInteriorConnectionActor >(GetOwner());
	FBox BoundingBox(Conn->GetMin(), Conn->GetMax());
	return FBoxSphereBounds(BoundingBox);
}

void UConnectionRenderingComponent::CreateRenderState_Concurrent()
{
	Super::CreateRenderState_Concurrent();

	if(SceneProxy)
	{
		static_cast<FConnectionSceneProxy*>(SceneProxy)->RegisterDebugDrawDelgate();
	}
}

void UConnectionRenderingComponent::DestroyRenderState_Concurrent()
{
	if(SceneProxy)
	{
		static_cast<FConnectionSceneProxy*>(SceneProxy)->UnregisterDebugDrawDelgate();
	}

	Super::DestroyRenderState_Concurrent();
}



