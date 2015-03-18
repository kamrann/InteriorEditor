// Fill out your copyright notice in the Description page of Project Settings.

#include "InteriorEditorPrivatePCH.h"
#include "InteriorGraphRenderingComponent.h"
#include "InteriorGraphActor.h"
#include "InteriorEditorNodeFace.h"
#include "InteriorEditorHitProxies.h"
#include "InteriorEditorMode.h" 

#include <algorithm>


IMPLEMENT_HIT_PROXY(HInteriorHitProxy, HHitProxy)
IMPLEMENT_HIT_PROXY(HInteriorNodeHitProxy, HInteriorHitProxy)
IMPLEMENT_HIT_PROXY(HInteriorNodeFaceHitProxy, HInteriorNodeHitProxy)
IMPLEMENT_HIT_PROXY(HInteriorPortalHitProxy, HInteriorHitProxy)


const float NodeFaceOffset = 0.4f;
const float PortalOffset = 0.5f;

const float LineDepthBias = 1.0e-4f;


enum class ENodeRenderMode {
	Normal,
	Selected,
};

class FGraphSceneProxy: public FDebugRenderSceneProxy
{
public:
	FGraphSceneProxy(
		const UPrimitiveComponent* InComponent,
		const FString& InViewFlagName
		);

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) override;
	virtual HHitProxy* CreateHitProxies(UPrimitiveComponent* Component, TArray<TRefCountPtr<HHitProxy> >& OutHitProxies) override;
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;

protected:
	struct FNodeInfo
	{
		NodeIdType Id;
		FBox Box;
	};

	struct FConnInfo
	{
		ConnectionIdType Id;
		FBox PortalBox;
		EAxisIndex Axis;
	};

protected:
	static FColor ColorFromMode(ENodeRenderMode mode);

	void AddNodeFace(
		FMeshElementCollector& Collector,
		int32 ViewIndex,
		NodeIdType NId,
		FFaceId Face,
		FVector const& NdCenter,
		FVector const& NdExtent,
		ENodeRenderMode Mode,
		bool bWireframe) const;
	void AddNode(
		FMeshElementCollector& Collector,
		int32 ViewIndex,
		NodeIdType NId,
		FVector const& NdCenter,
		FVector const& NdExtent,
		ENodeRenderMode Mode,
		bool bWireframe) const;
	void AddPortal(
		FMeshElementCollector& Collector,
		int32 ViewIndex,
		FConnInfo const& Cn,
		ENodeRenderMode Mode,
		bool bWireframe) const;

protected:
	AInteriorGraphActor* Graph;
	UMaterialInterface* Mat;
	TArray< FNodeInfo > NodeInfo;
	TArray< FConnInfo > ConnInfo;

	NodeIdList SelectedNodes;
	TArray< FNodeFaceRef > SelectedFaces;
	ConnectionIdList SelectedPortals;
};


FGraphSceneProxy::FGraphSceneProxy(
	const UPrimitiveComponent* InComponent,
	const FString& InViewFlagName
	):
//	FPrimitiveSceneProxy(InComponent)
	FDebugRenderSceneProxy(InComponent)
{
	ViewFlagIndex = uint32(FEngineShowFlags::FindIndexByName(*InViewFlagName));
	ViewFlagName = InViewFlagName;

	Graph = nullptr;
	Mat = nullptr;
	auto RenderComp = Cast< UInteriorGraphRenderingComponent >(InComponent);
	if(RenderComp)
	{
		Graph = Cast< AInteriorGraphActor >(RenderComp->GetOwner());

		for(auto const& Nd : Graph->NodeMap)
		{
			FNodeInfo NI;
			NI.Id = Nd.Key;
			NI.Box = Graph->NodeData[Nd.Value].Box();
			NodeInfo.Add(std::move(NI));
		}

		for(auto const& Cn : Graph->ConnectionMap)
		{
			FConnInfo CI;
			CI.Id = Cn.Key;
			CI.PortalBox = Graph->GetConnectionData(Cn.Value).Portal;
			auto Sz = CI.PortalBox.GetSize();
			CI.Axis = Sz.X == 0.f ? EAxisIndex::X : (Sz.Y == 0.f ? EAxisIndex::Y : EAxisIndex::Z);
			ConnInfo.Add(std::move(CI));
		}

		//Mat = Graph->Mat;
		Mat = UMaterial::GetDefaultMaterial(EMaterialDomain::MD_Surface);

		if(GLevelEditorModeTools().IsModeActive(FInteriorEditorMode::ModeId))
		{
			auto EdMode = (FInteriorEditorMode*)GLevelEditorModeTools().GetActiveMode(FInteriorEditorMode::ModeId);
			SelectedNodes = EdMode->SelectedNodes;
			SelectedFaces = EdMode->SelectedFaces;
			SelectedPortals = EdMode->SelectedPortals;
		}
	}
}

FColor FGraphSceneProxy::ColorFromMode(ENodeRenderMode mode)
{
	switch(mode)
	{
		case ENodeRenderMode::Selected:
		return FColor::Yellow;

		default:
		return FColor::White;
	}
}

void FGraphSceneProxy::AddNodeFace(
	FMeshElementCollector& Collector, 
	int32 ViewIndex, 
	NodeIdType NId,
	FFaceId Face, 
	FVector const& NdCenter, 
	FVector const& NdExtent, 
	ENodeRenderMode Mode,
	bool bWireframe) const
{
/*	static FRotator FaceRotations[3] = {
		{ 0, 0, 0 },
		{ 0, 90, 0 },
		{ 0, 0, 90 }
	};
*/
	FDynamicMeshBuilder Mesh;
	FDynamicMeshVertex V;
	V.Color = FColor::White;

	// TODO: Need a struct with the methods of FNodeData, but that is more generic and does not include the out connections
	FNodeData ND;
	ND.Min = NdCenter + NdExtent * -0.5f;
	ND.Max = NdCenter + NdExtent * 0.5f;
	V.TangentX = ND.FaceXTangent(Face.Axis, Face.Dir);
		//FaceRotations[Face.Axis].RotateVector(FVector{ GetDirectionMultiplier(Face.Dir), 0, 0 });
	V.TangentZ = ND.FaceNormal(Face.Axis, Face.Dir);
		//FaceRotations[Face.Axis].RotateVector(FVector{ 0, 0, GetDirectionMultiplier(Face.Dir) });
	
	auto Norm = FVector::ZeroVector;
	/*
	Contract very slightly (NodeFaceOffset) so that coplanar faces do not z-fight
	*/
	Norm[Face.Axis] = (NdExtent[Face.Axis] * 0.5f - NodeFaceOffset) * GetDirectionMultiplier(Face.Dir);

	auto Planar1 = FVector::ZeroVector;
	Planar1[FAxisUtils::OtherAxes[Face.Axis][0]] = NdExtent[FAxisUtils::OtherAxes[Face.Axis][0]] * 0.5f - NodeFaceOffset;
	auto Planar2 = FVector::ZeroVector;
	Planar2[FAxisUtils::OtherAxes[Face.Axis][1]] = NdExtent[FAxisUtils::OtherAxes[Face.Axis][1]] * 0.5f - NodeFaceOffset;
	auto Base = NdCenter + Norm;

	auto HitPrxy = new HInteriorNodeFaceHitProxy(NId, Face.Axis, Face.Dir);

	auto const EdgeProp = 0.9f;
	auto const InnerProp = 0.2f;

	FVector const OuterQuadVerts[4] = {
		Base - Planar1 - Planar2,
		Base - Planar1 + Planar2,
		Base + Planar1 + Planar2,
		Base + Planar1 - Planar2,
	};

	V.Position = OuterQuadVerts[0];
	Mesh.AddVertex(V);
	V.Position = OuterQuadVerts[1];
	Mesh.AddVertex(V);
	V.Position = OuterQuadVerts[2];
	Mesh.AddVertex(V);
	V.Position = OuterQuadVerts[3];
	Mesh.AddVertex(V);

	V.Position = Base + (-Planar1 - Planar2) * EdgeProp;
	Mesh.AddVertex(V);
	V.Position = Base + (-Planar1 + Planar2) * EdgeProp;
	Mesh.AddVertex(V);
	V.Position = Base + (Planar1 + Planar2) * EdgeProp;
	Mesh.AddVertex(V);
	V.Position = Base + (Planar1 - Planar2) * EdgeProp;
	Mesh.AddVertex(V);

	FVector const CenterQuadVerts[4] = {
		Base + (-Planar1 - Planar2) * InnerProp,
		Base + (-Planar1 + Planar2) * InnerProp,
		Base + (Planar1 + Planar2) * InnerProp,
		Base + (Planar1 - Planar2) * InnerProp,
	};

	V.Position = CenterQuadVerts[0];
	Mesh.AddVertex(V);
	V.Position = CenterQuadVerts[1];
	Mesh.AddVertex(V);
	V.Position = CenterQuadVerts[2];
	Mesh.AddVertex(V);
	V.Position = CenterQuadVerts[3];
	Mesh.AddVertex(V);

	Mesh.AddTriangle(0, 5, 1);
	Mesh.AddTriangle(0, 4, 5);
	Mesh.AddTriangle(1, 6, 2);
	Mesh.AddTriangle(1, 5, 6);
	Mesh.AddTriangle(2, 7, 3);
	Mesh.AddTriangle(2, 6, 7);
	Mesh.AddTriangle(3, 4, 0);
	Mesh.AddTriangle(3, 7, 4);

	Mesh.AddTriangle(8, 10, 9);
	Mesh.AddTriangle(8, 11, 10);

	if(Mode == ENodeRenderMode::Selected)
	{
		auto PDI = Collector.GetPDI(ViewIndex);
		PDI->SetHitProxy(HitPrxy);

		PDI->DrawLine(OuterQuadVerts[0], OuterQuadVerts[1], FColor::Yellow, SDPG_World, 0.0f, LineDepthBias);
		PDI->DrawLine(OuterQuadVerts[1], OuterQuadVerts[2], FColor::Yellow, SDPG_World, 0.0f, LineDepthBias);
		PDI->DrawLine(OuterQuadVerts[2], OuterQuadVerts[3], FColor::Yellow, SDPG_World, 0.0f, LineDepthBias);
		PDI->DrawLine(OuterQuadVerts[3], OuterQuadVerts[0], FColor::Yellow, SDPG_World, 0.0f, LineDepthBias);

		PDI->DrawLine(CenterQuadVerts[0], CenterQuadVerts[1], FColor::Yellow, SDPG_World, 0.0f, LineDepthBias);
		PDI->DrawLine(CenterQuadVerts[1], CenterQuadVerts[2], FColor::Yellow, SDPG_World, 0.0f, LineDepthBias);
		PDI->DrawLine(CenterQuadVerts[2], CenterQuadVerts[3], FColor::Yellow, SDPG_World, 0.0f, LineDepthBias);
		PDI->DrawLine(CenterQuadVerts[3], CenterQuadVerts[0], FColor::Yellow, SDPG_World, 0.0f, LineDepthBias);

		PDI->SetHitProxy(nullptr);
	}

	auto Material = bWireframe ? (UMaterialInterface*)GEngine->WireframeMaterial :
		(Mat ? Mat : (UMaterialInterface*)GEngine->ArrowMaterial);
	auto MatProxy = new FColoredMaterialRenderProxy{
		Material->GetRenderProxy(false),
		ColorFromMode(Mode)
	};

	Collector.RegisterOneFrameMaterialProxy(MatProxy);

	Mesh.GetMesh(FMatrix::Identity, MatProxy, SDPG_World, true, false, false, ViewIndex, Collector, HitPrxy);
}

void FGraphSceneProxy::AddNode(
	FMeshElementCollector& Collector,
	int32 ViewIndex,
	NodeIdType NId,
	FVector const& NdCenter,
	FVector const& NdExtent,
	ENodeRenderMode Mode,
	bool bWireframe) const
{
	FDynamicMeshBuilder Mesh;
	FDynamicMeshVertex V;
	V.Color = FColor::White;

	auto HitPrxy = new HInteriorNodeHitProxy(NId);

	auto BaseIdx = 0;

	const EAxisIndex Axes[] = { EAxisIndex::X, EAxisIndex::Y, EAxisIndex::Z };
	for(auto Axis : Axes)
	{
		const EAxisDirection Directions[] = { EAxisDirection::Positive, EAxisDirection::Negative };
		for(auto Dir : Directions)
		{
			// TODO: Need a struct with the methods of FNodeData, but that is more generic and does not include the out connections
			FNodeData ND;
			ND.Min = NdCenter + NdExtent * -0.5f;
			ND.Max = NdCenter + NdExtent * 0.5f;
			V.TangentX = ND.FaceXTangent(Axis, Dir);
			V.TangentZ = ND.FaceNormal(Axis, Dir);

			auto const BoxProp = 0.2f;
			auto Norm = FVector::ZeroVector;
			Norm[Axis] = NdExtent[Axis] * 0.5f * BoxProp * GetDirectionMultiplier(Dir);
			auto Planar1 = FVector::ZeroVector;
			Planar1[FAxisUtils::OtherAxes[Axis][0]] = NdExtent[FAxisUtils::OtherAxes[Axis][0]] * 0.5f * BoxProp;
			auto Planar2 = FVector::ZeroVector;
			Planar2[FAxisUtils::OtherAxes[Axis][1]] = NdExtent[FAxisUtils::OtherAxes[Axis][1]] * 0.5f * BoxProp;
			auto Base = NdCenter + Norm;
			
			FVector const QuadVerts[4] = {
				Base - Planar1 - Planar2,
				Base - Planar1 + Planar2,
				Base + Planar1 + Planar2,
				Base + Planar1 - Planar2,
			};

			V.Position = QuadVerts[0];
			Mesh.AddVertex(V);
			V.Position = QuadVerts[1];
			Mesh.AddVertex(V);
			V.Position = QuadVerts[2];
			Mesh.AddVertex(V);
			V.Position = QuadVerts[3];
			Mesh.AddVertex(V);

			Mesh.AddTriangle(BaseIdx + 0, BaseIdx + 2, BaseIdx + 1);
			Mesh.AddTriangle(BaseIdx + 0, BaseIdx + 3, BaseIdx + 2);

			if(Mode == ENodeRenderMode::Selected)
			{
				auto PDI = Collector.GetPDI(ViewIndex);
				PDI->SetHitProxy(HitPrxy);

				PDI->DrawLine(QuadVerts[0], QuadVerts[1], FColor::Yellow, SDPG_World, 0.0f, LineDepthBias);
				PDI->DrawLine(QuadVerts[1], QuadVerts[2], FColor::Yellow, SDPG_World, 0.0f, LineDepthBias);
				PDI->DrawLine(QuadVerts[2], QuadVerts[3], FColor::Yellow, SDPG_World, 0.0f, LineDepthBias);
				PDI->DrawLine(QuadVerts[3], QuadVerts[0], FColor::Yellow, SDPG_World, 0.0f, LineDepthBias);

				PDI->SetHitProxy(nullptr);
			}

			BaseIdx += 4;
		}
	}

	auto Material = bWireframe ? (UMaterialInterface*)GEngine->WireframeMaterial :
		(Mat ? Mat : (UMaterialInterface*)GEngine->ArrowMaterial);
	auto MatProxy = new FColoredMaterialRenderProxy{
		Material->GetRenderProxy(false),
		ColorFromMode(Mode)
	};

	Collector.RegisterOneFrameMaterialProxy(MatProxy);

	Mesh.GetMesh(FMatrix::Identity, MatProxy, SDPG_World, true, false, false, ViewIndex, Collector, HitPrxy);
}

void FGraphSceneProxy::AddPortal(
	FMeshElementCollector& Collector,
	int32 ViewIndex,
	FConnInfo const& Cn,
	ENodeRenderMode Mode,
	bool bWireframe) const
{
	FDynamicMeshBuilder Mesh;
	FDynamicMeshVertex V;
	V.Color = FColor::White;

	// todo:
	V.TangentX = FVector::ZeroVector;// ND.FaceXTangent(Face.Axis, Face.Dir);
	V.TangentZ = FVector::ZeroVector;// ND.FaceNormal(Face.Axis, Face.Dir);
	auto Norm = FVector::ZeroVector;
	Norm[Cn.Axis] = 1.f;
	V.TangentZ = Norm;

	auto const CornerProp = 0.7f;

	auto HalfSize = Cn.PortalBox.GetExtent();
	auto Planar1 = FVector::ZeroVector;
	Planar1[FAxisUtils::OtherAxes[Cn.Axis][0]] = HalfSize[FAxisUtils::OtherAxes[Cn.Axis][0]] - NodeFaceOffset;
	auto Planar2 = FVector::ZeroVector;
	Planar2[FAxisUtils::OtherAxes[Cn.Axis][1]] = HalfSize[FAxisUtils::OtherAxes[Cn.Axis][1]] - NodeFaceOffset;

	auto HitPrxy = new HInteriorPortalHitProxy(Cn.Id);

	auto const Directions = { EAxisDirection::Positive, EAxisDirection::Negative };
	int BaseIdx = 0;
	for(auto Dir : Directions)
	{
		auto Base = Cn.PortalBox.GetCenter();
		Base += Norm * PortalOffset * GetDirectionMultiplier(Dir);

		auto Cnr1 = Base - Planar1 - Planar2;
		V.Position = Cnr1;
		Mesh.AddVertex(V);
		V.Position = Base - Planar1 * CornerProp - Planar2;
		Mesh.AddVertex(V);
		V.Position = Base - Planar1 * CornerProp - Planar2 * CornerProp;
		Mesh.AddVertex(V);
		V.Position = Base - Planar1 - Planar2 * CornerProp;
		Mesh.AddVertex(V);

		auto Cnr2 = Base - Planar1 + Planar2;
		V.Position = Cnr2;
		Mesh.AddVertex(V);
		V.Position = Base - Planar1 + Planar2 * CornerProp;
		Mesh.AddVertex(V);
		V.Position = Base - Planar1 * CornerProp + Planar2 * CornerProp;
		Mesh.AddVertex(V);
		V.Position = Base - Planar1 * CornerProp + Planar2;
		Mesh.AddVertex(V);

		auto Cnr3 = Base + Planar1 + Planar2;
		V.Position = Cnr3;
		Mesh.AddVertex(V);
		V.Position = Base + Planar1 * CornerProp + Planar2;
		Mesh.AddVertex(V);
		V.Position = Base + Planar1 * CornerProp + Planar2 * CornerProp;
		Mesh.AddVertex(V);
		V.Position = Base + Planar1 + Planar2 * CornerProp;
		Mesh.AddVertex(V);

		auto Cnr4 = Base + Planar1 - Planar2;
		V.Position = Cnr4;
		Mesh.AddVertex(V);
		V.Position = Base + Planar1 - Planar2 * CornerProp;
		Mesh.AddVertex(V);
		V.Position = Base + Planar1 * CornerProp - Planar2 * CornerProp;
		Mesh.AddVertex(V);
		V.Position = Base + Planar1 * CornerProp - Planar2;
		Mesh.AddVertex(V);

		Mesh.AddTriangle(BaseIdx + 0, BaseIdx + 1, BaseIdx + 2);
		Mesh.AddTriangle(BaseIdx + 0, BaseIdx + 2, BaseIdx + 3);
		Mesh.AddTriangle(BaseIdx + 4, BaseIdx + 5, BaseIdx + 6);
		Mesh.AddTriangle(BaseIdx + 4, BaseIdx + 6, BaseIdx + 7);
		Mesh.AddTriangle(BaseIdx + 8, BaseIdx + 9, BaseIdx + 10);
		Mesh.AddTriangle(BaseIdx + 8, BaseIdx + 10, BaseIdx + 11);
		Mesh.AddTriangle(BaseIdx + 12, BaseIdx + 13, BaseIdx + 14);
		Mesh.AddTriangle(BaseIdx + 12, BaseIdx + 14, BaseIdx + 15);

		auto PDI = Collector.GetPDI(ViewIndex);
		PDI->SetHitProxy(HitPrxy);
		auto Color = Mode == ENodeRenderMode::Selected ? FColor::Yellow : FColor(FLinearColor(0.f, 0.5f, 0.f));

		PDI->DrawLine(Cnr1, Cnr2, Color, SDPG_World, 3.0f, 0.5f);
		PDI->DrawLine(Cnr2, Cnr3, Color, SDPG_World, 3.0f, 0.5f);
		PDI->DrawLine(Cnr3, Cnr4, Color, SDPG_World, 3.0f, 0.5f);
		PDI->DrawLine(Cnr4, Cnr1, Color, SDPG_World, 3.0f, 0.5f);

		PDI->SetHitProxy(nullptr);

		BaseIdx += 16;	// Added vertex count
	}

	auto Material = bWireframe ? (UMaterialInterface*)GEngine->WireframeMaterial :
		Mat;// (UMaterialInterface*)GEngine->DebugMeshMaterial;
	auto MatProxy = new FColoredMaterialRenderProxy{
		Material->GetRenderProxy(false),
		FColor::Green//ColorFromMode(Mode)
	};

	Collector.RegisterOneFrameMaterialProxy(MatProxy);

	Mesh.GetMesh(FMatrix::Identity, MatProxy, SDPG_World, true, false, false, ViewIndex, Collector, HitPrxy);
}

FPrimitiveViewRelevance FGraphSceneProxy::GetViewRelevance(const FSceneView* View)
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

HHitProxy* FGraphSceneProxy::CreateHitProxies(UPrimitiveComponent* Component, TArray<TRefCountPtr<HHitProxy> >& OutHitProxies)
{
	// ????
	return FDebugRenderSceneProxy::CreateHitProxies(Component, OutHitProxies);
}

void FGraphSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	const FEngineShowFlags& EngineShowFlags = ViewFamily.EngineShowFlags;
	auto mode = /*IsSelected() ? ENodeRenderMode::Selected :*/ ENodeRenderMode::Normal;	// todo: if more modes???
	//auto Center = GetActorPosition();
	
//	if(AllowDebugViewmodes())	TODO: Maybe needs module dependency
	{
		for(int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if(VisibilityMap & (1 << ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];
//				FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

				for(auto const& Nd : NodeInfo)
				{
					auto bSel = SelectedNodes.Contains(Nd.Id);
					AddNode(
						Collector,
						ViewIndex,
						Nd.Id,
						Nd.Box.GetCenter(),
						Nd.Box.GetSize(),
						bSel ? ENodeRenderMode::Selected : ENodeRenderMode::Normal,
						EngineShowFlags.Wireframe);

					for(int axis = 0; axis < EAxisIndex::Count; ++axis)
					{
						auto FId = FFaceId{ (EAxisIndex)axis, EAxisDirection::Positive };
						auto bSel = SelectedFaces.Contains(FNodeFaceRef{ Nd.Id, FId.Axis, FId.Dir });
						AddNodeFace(
							Collector,
							ViewIndex,
							Nd.Id,
							FId,
							Nd.Box.GetCenter(),
							Nd.Box.GetSize(),
							bSel ? ENodeRenderMode::Selected : ENodeRenderMode::Normal,
							EngineShowFlags.Wireframe);

						FId = FFaceId{ (EAxisIndex)axis, EAxisDirection::Negative };
						bSel = SelectedFaces.Contains(FNodeFaceRef{ Nd.Id, FId.Axis, FId.Dir });
						AddNodeFace(
							Collector,
							ViewIndex,
							Nd.Id,
							FId,
							Nd.Box.GetCenter(),
							Nd.Box.GetSize(),
							bSel ? ENodeRenderMode::Selected : ENodeRenderMode::Normal,
							EngineShowFlags.Wireframe);
					}
				}

				for(auto const& Cn : ConnInfo)
				{
					auto bSel = SelectedPortals.Contains(Cn.Id);
					AddPortal(
						Collector,
						ViewIndex,
						Cn,
						bSel ? ENodeRenderMode::Selected : ENodeRenderMode::Normal,
						EngineShowFlags.Wireframe);
				}
			}
		}
	}
}


UInteriorGraphRenderingComponent::UInteriorGraphRenderingComponent(FObjectInitializer const& OI):
Super(OI)
{
}

FPrimitiveSceneProxy* UInteriorGraphRenderingComponent::CreateSceneProxy()
{
	auto Node = Cast< AInteriorGraphActor >(GetOwner());
	if(Node)
	{
		return new FGraphSceneProxy(
			this,
			"GameplayDebug"
			);
	}
	else
	{
		return nullptr;
	}
}

FBoxSphereBounds UInteriorGraphRenderingComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	// TODO:
	static FBox BoundingBox(FVector(-HALF_WORLD_MAX), FVector(HALF_WORLD_MAX));
	return FBoxSphereBounds(BoundingBox);
}

void UInteriorGraphRenderingComponent::CreateRenderState_Concurrent()
{
	Super::CreateRenderState_Concurrent();

	if(SceneProxy)
	{
		static_cast<FGraphSceneProxy*>(SceneProxy)->RegisterDebugDrawDelgate();
	}
}

void UInteriorGraphRenderingComponent::DestroyRenderState_Concurrent()
{
	if(SceneProxy)
	{
		static_cast<FGraphSceneProxy*>(SceneProxy)->UnregisterDebugDrawDelgate();
	}

	Super::DestroyRenderState_Concurrent();
}



