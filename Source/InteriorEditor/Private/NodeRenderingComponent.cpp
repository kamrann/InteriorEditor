// Fill out your copyright notice in the Description page of Project Settings.

#include "InteriorEditorPrivatePCH.h"

#if 0

#include "NodeRenderingComponent.h"
#include "InteriorNodeActor.h"
#include "InteriorEditorHitProxies.h"


IMPLEMENT_HIT_PROXY(HInteriorNodeFaceHitProxy, HHitProxy)


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
	virtual HHitProxy* CreateHitProxies(UPrimitiveComponent* Component, TArray<TRefCountPtr<HHitProxy> >& OutHitProxies) override;
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;

protected:
	static FColor ColorFromMode(ENodeRenderMode mode);

protected:
	AInteriorNodeActor* Node;
	FVector Extent;
};


FNodeSceneProxy::FNodeSceneProxy(
	const UPrimitiveComponent* InComponent,
	const FString& InViewFlagName,
	FVector const& Extent
	):
//	FPrimitiveSceneProxy(InComponent)
	FDebugRenderSceneProxy(InComponent)
{
	ViewFlagIndex = uint32(FEngineShowFlags::FindIndexByName(*InViewFlagName));
	ViewFlagName = InViewFlagName;

	this->Node = nullptr;
	auto RenderComp = Cast< UNodeRenderingComponent >(InComponent);
	if(RenderComp)
	{
		this->Node = Cast< AInteriorNodeActor >(RenderComp->GetOwner());
	}
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
/*		View->Family->EngineShowFlags.GetSingleFlag(ViewFlagIndex)
		&&*/ IsShown(View)
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

HHitProxy* FNodeSceneProxy::CreateHitProxies(UPrimitiveComponent* Component, TArray<TRefCountPtr<HHitProxy> >& OutHitProxies)
{
	// ????
	return nullptr;
}

void FNodeSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	auto mode = IsSelected() ? ENodeRenderMode::Selected : ENodeRenderMode::Normal;	// todo: if more modes???
	auto Center = GetActorPosition();
	
//	if(AllowDebugViewmodes())	TODO: Maybe needs module dependency
	{
		for(int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if(VisibilityMap & (1 << ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];
				FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
				bool bHitTesting = true;// PDI->IsHitTesting();

				for(int axis = 0; axis < EAxisIndex::Count; ++axis)
				{
					if(bHitTesting)
					{
						PDI->SetHitProxy(new HInteriorNodeFaceHitProxy(Node, (EAxisIndex)axis, EAxisDirection::Positive));
					}

					auto Norm = FVector::ZeroVector;
					Norm[axis] = Extent[axis] * 0.5f;
					auto Planar1 = FVector::ZeroVector;
					Planar1[FAxisUtils::OtherAxes[axis][0]] = Extent[FAxisUtils::OtherAxes[axis][0]] * 0.5f;
					auto Planar2 = FVector::ZeroVector;
					Planar2[FAxisUtils::OtherAxes[axis][1]] = Extent[FAxisUtils::OtherAxes[axis][1]] * 0.5f;
					auto Color = ColorFromMode(mode);
					auto Base = Center + Norm;
					PDI->DrawLine(Base - Planar1 - Planar2, Base - Planar1 + Planar2, Color, SDPG_World);
					PDI->DrawLine(Base - Planar1 + Planar2, Base + Planar1 + Planar2, Color, SDPG_World);
					PDI->DrawLine(Base + Planar1 + Planar2, Base + Planar1 - Planar2, Color, SDPG_World);
					PDI->DrawLine(Base + Planar1 - Planar2, Base - Planar1 - Planar2, Color, SDPG_World);

					if(bHitTesting)
					{
						PDI->SetHitProxy(new HInteriorNodeFaceHitProxy(Node, (EAxisIndex)axis, EAxisDirection::Negative));
					}

					Color = ColorFromMode(mode);
					Base = Center - Norm;
					PDI->DrawLine(Base - Planar1 - Planar2, Base - Planar1 + Planar2, Color, SDPG_World);
					PDI->DrawLine(Base - Planar1 + Planar2, Base + Planar1 + Planar2, Color, SDPG_World);
					PDI->DrawLine(Base + Planar1 + Planar2, Base + Planar1 - Planar2, Color, SDPG_World);
					PDI->DrawLine(Base + Planar1 - Planar2, Base - Planar1 - Planar2, Color, SDPG_World);

					if(bHitTesting)
					{
						PDI->SetHitProxy(nullptr);
					}
				}
			}
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


#endif
