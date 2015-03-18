// Fill out your copyright notice in the Description page of Project Settings.

#include "InteriorEditorPrivatePCH.h"
#include "InteriorEditorConversion.h"
#include "InteriorEditorNodeFace.h"
#include "InteriorGraphActor.h"
#include "RawMesh.h"
#include "StaticMeshResources.h"
#include "AssetRegistryModule.h"


auto const Epsilon = 0.01f;

struct FPortalEdgeInfo
{
	// These are along the lateral axis
	float MinExtent;
	float MaxExtent;
};

struct FCombinedPortalEdgeInfo
{
	enum EType { End, Begin };

	float AxisValue;
	EType Type;
	TArray< FPortalEdgeInfo > Edges;

	inline bool operator< (FCombinedPortalEdgeInfo const& RHS) const
	{
		return AxisValue < RHS.AxisValue || (AxisValue == RHS.AxisValue && Type < RHS.Type);
	}
};

typedef TArray< FCombinedPortalEdgeInfo > CombinedPortalEdgeList;


CombinedPortalEdgeList GeneratePortalEdges(const AInteriorGraphActor* Graph, NodeIdType NId, FFaceId Face)
{
	auto const PA1 = FAxisUtils::OtherAxes[Face.Axis][0];
	auto const PA2 = FAxisUtils::OtherAxes[Face.Axis][1];

	auto PortalIds = Graph->GetConnectionsOnFace(NId, Face);
	CombinedPortalEdgeList Edges;
	for(auto PId : PortalIds)
	{
		auto const& Cn = Graph->GetConnectionData(PId);

		FPortalEdgeInfo Edge;
		Edge.MinExtent = Cn.Portal.Min[PA2];
		Edge.MaxExtent = Cn.Portal.Max[PA2];

		{
			FCombinedPortalEdgeInfo Combined;
			Combined.AxisValue = Cn.Portal.Min[PA1];
			Combined.Type = FCombinedPortalEdgeInfo::Begin;
			Combined.Edges.Add(Edge);
			Edges.Add(Combined);
		}

		{
			FCombinedPortalEdgeInfo Combined;
			Combined.AxisValue = Cn.Portal.Max[PA1];
			Combined.Type = FCombinedPortalEdgeInfo::End;
			Combined.Edges.Add(Edge);
			Edges.Add(Combined);
		}
	}

	// TODO: Compact/merge edges

	Edges.Sort();
	return Edges;
}

struct FSpan
{
	float Min, Max;	// Lateral extents
	float Start;	// Longitudinal starting value for the span
};

typedef TArray< FSpan > SpanList;

bool EdgeSeparatesSpans(FPortalEdgeInfo const& Edge, FSpan const& S1, FSpan const& S2)
{
	return FMath::IsNearlyEqual(Edge.MinExtent, S1.Max, Epsilon) &&
		FMath::IsNearlyEqual(Edge.MaxExtent, S2.Min, Epsilon);
}

FBox2D CreateSpanRect(FSpan const& S, float EndValue)
{
	FBox2D Rc;
	// X is longitudinal, Y lateral
	Rc.Min = FVector2D{ S.Start, S.Min };
	Rc.Max = FVector2D{ EndValue, S.Max };
	return Rc;
}

TArray< FBox2D > MergeSpans(SpanList& Spans, FPortalEdgeInfo const& Edge, float CurValue)
{
	for(int i = 0; i < Spans.Num() - 1; ++i)
	{
		auto j = i + 1;

		if(EdgeSeparatesSpans(Edge, Spans[i], Spans[j]))
		{
			TArray< FBox2D > Rects;
			Rects.Add(CreateSpanRect(Spans[i], CurValue));
			Rects.Add(CreateSpanRect(Spans[j], CurValue));

			Spans[i].Max = Spans[j].Max;
			Spans[i].Start = CurValue;
			Spans.RemoveAt(j);

			return Rects;
		}
	}

	check(false);
	return{};
}

bool SpanEncompassesEdge(FPortalEdgeInfo const& Edge, FSpan const& S)
{
	// todo: use Epsilon here??
	return S.Min <= Edge.MinExtent && S.Max >= Edge.MaxExtent;
}

TArray< FBox2D > SplitOrContractSpans(SpanList& Spans, FPortalEdgeInfo const& Edge, float CurValue)
{
	TArray< FBox2D > Rects;
	for(int i = 0; i < Spans.Num(); ++i)
	{
		if(SpanEncompassesEdge(Edge, Spans[i]))
		{
			// This span contains the new edge fully, so split span into one either side
			Rects.Add(CreateSpanRect(Spans[i], CurValue));

			FSpan NewSpan;
			NewSpan.Start = CurValue;
			NewSpan.Min = Spans[i].Min;
			NewSpan.Max = Edge.MinExtent;

			Spans[i].Start = CurValue;
			Spans[i].Min = Edge.MaxExtent;
			// Max unchanged

			Spans.Insert(NewSpan, i);

			// No other spans can hit the edge
			break;
		}
		else if(Spans[i].Min < Edge.MinExtent && Spans[i].Max > Edge.MinExtent)
		{
			Rects.Add(CreateSpanRect(Spans[i], CurValue));

			// Contract the span
			Spans[i].Start = CurValue;
			Spans[i].Max = Edge.MinExtent;
		}
		else if(Spans[i].Min < Edge.MaxExtent && Spans[i].Max > Edge.MaxExtent)
		{
			Rects.Add(CreateSpanRect(Spans[i], CurValue));

			// Contract the span
			Spans[i].Start = CurValue;
			Spans[i].Min = Edge.MaxExtent;
		}
	}

	check(Rects.Num() > 0);
	return Rects;
}

bool ConvertInteriorGraphToRawMesh(AInteriorGraphActor* Graph, FRawMesh& Mesh)
{
	auto const TexRepeatUnits = 100.f;

	enum MatTypes { Floor, Wall, Ceiling };

	Mesh.Empty();

	TArray< FVector > VertexPositions;

	auto NodeIds = Graph->GetAllNodes();
	for(auto NId : NodeIds)
	{
		auto const& Nd = Graph->GetNodeData(NId);

		for(EAxisIndex Axis : FAxisUtils::AllAxes)
		{
			for(EAxisDirection Dir : FAxisUtils::BothDirections)
			{
				auto const PA1 = FAxisUtils::OtherAxes[Axis][0];
				auto const PA2 = FAxisUtils::OtherAxes[Axis][1];

				// TODO: This ordering is currently assumed by below generation function call.
				// Better to let the function decide, and return the ordering.
				auto const LongAx = PA1;
				auto const LatAx = PA2;
				/*
				Traverse along the first planar axis (PA1) and find all portal edges in the second planar axis (PA2)
				*/
				auto CombinedEdges = GeneratePortalEdges(Graph, NId, FFaceId{ Axis, Dir });

				float LongBase = Nd.Min[LongAx];
				TArray< FSpan > ActiveSpans;
				ActiveSpans.Add(FSpan{ Nd.Min[LatAx], Nd.Max[LatAx], LongBase });

				TArray< FBox2D > Rects;

				// Iterate over the combined edges in order (progressing longitudinally)
				for(int i = 0; i < CombinedEdges.Num(); ++i)
				{
					auto const& CEdge = CombinedEdges[i];
					switch(CEdge.Type)
					{
						case FCombinedPortalEdgeInfo::End:
						{
							// End of portal(s), merge together active spans separated by the portals that just ended
							for(auto const& Edge : CEdge.Edges)
							{
								auto ResultingRects = MergeSpans(ActiveSpans, Edge, CEdge.AxisValue);
								Rects.Append(ResultingRects);
							}
						}
						break;

						case FCombinedPortalEdgeInfo::Begin:
						{
							// Portal begins. Some spans will need to be modified or split.
							for(auto const& Edge : CEdge.Edges)
							{
								auto ResultingRects = SplitOrContractSpans(ActiveSpans, Edge, CEdge.AxisValue);
								Rects.Append(ResultingRects);
							}
						}
						break;
					}
				}

				// Should be a single remaining active span, which needs closing off
				check(ActiveSpans.Num() == 1);
				Rects.Add(CreateSpanRect(ActiveSpans[0], Nd.Max[LongAx]));

				for(auto const& Rc : Rects)
				{
					if(FMath::IsNearlyZero(Rc.GetSize().X, Epsilon) ||
						FMath::IsNearlyZero(Rc.GetSize().Y, Epsilon))
					{
						continue;
					}

					FBox Box3D;
					Box3D.Min[PA1] = Rc.Min[0];
					Box3D.Min[PA2] = Rc.Min[1];
					Box3D.Min[Axis] = Nd.FaceAxisValue(Axis, Dir);
					Box3D.Max[PA1] = Rc.Max[0];
					Box3D.Max[PA2] = Rc.Max[1];
					Box3D.Max[Axis] = Nd.FaceAxisValue(Axis, Dir);

					FVector Planar1 = FVector::ZeroVector;
					Planar1[PA1] = Box3D.Max[PA1] - Box3D.Min[PA1];
					FVector Planar2 = FVector::ZeroVector;
					Planar2[PA2] = Box3D.Max[PA2] - Box3D.Min[PA2];
					if(Dir == EAxisDirection::Negative)
					{
						Swap(Planar1, Planar2);
					}


					auto VertexBase = VertexPositions.Num();
					VertexPositions.Add(Box3D.Min);
					VertexPositions.Add(Box3D.Min + Planar2);
					VertexPositions.Add(Box3D.Max);
					VertexPositions.Add(Box3D.Min + Planar1);

					auto FaceMaskIdx = (int)Axis + (Dir == EAxisDirection::Positive ? 0 : (int)EAxisIndex::Count);
					auto MatIdx = Axis == EAxisIndex::Z ? (Dir == EAxisDirection::Positive ? MatTypes::Ceiling : MatTypes::Floor) : MatTypes::Wall;

					// Ensure that texture coordinates always use V for Z if Z exists
					auto TA1 = FMath::Min(PA1, PA2);
					auto TA2 = FMath::Max(PA1, PA2);
//					auto TA1 = Axis == EAxisIndex::Z ? PA1 : (Axis == EAxisIndex::X ? EAxisIndex::Y : EAxisIndex::X);
//					auto TA2 = Axis == EAxisIndex::Z ? PA2 : EAxisIndex::Z;

					Mesh.FaceMaterialIndices.Add(MatIdx);
					Mesh.FaceSmoothingMasks.Add(1 << FaceMaskIdx);
					auto VIdx = VertexBase + 0;
					Mesh.WedgeIndices.Add(VIdx);
					Mesh.WedgeTexCoords[0].Add(FVector2D{ VertexPositions[VIdx][TA1], VertexPositions[VIdx][TA2] });
					VIdx = VertexBase + 2;
					Mesh.WedgeIndices.Add(VIdx);
					Mesh.WedgeTexCoords[0].Add(FVector2D{ VertexPositions[VIdx][TA1], VertexPositions[VIdx][TA2] });
					VIdx = VertexBase + 1;
					Mesh.WedgeIndices.Add(VIdx);
					Mesh.WedgeTexCoords[0].Add(FVector2D{ VertexPositions[VIdx][TA1], VertexPositions[VIdx][TA2] });
					Mesh.FaceMaterialIndices.Add(MatIdx);
					Mesh.FaceSmoothingMasks.Add(1 << FaceMaskIdx);
					VIdx = VertexBase + 0;
					Mesh.WedgeIndices.Add(VIdx);
					Mesh.WedgeTexCoords[0].Add(FVector2D{ VertexPositions[VIdx][TA1], VertexPositions[VIdx][TA2] });
					VIdx = VertexBase + 3;
					Mesh.WedgeIndices.Add(VIdx);
					Mesh.WedgeTexCoords[0].Add(FVector2D{ VertexPositions[VIdx][TA1], VertexPositions[VIdx][TA2] });
					VIdx = VertexBase + 2;
					Mesh.WedgeIndices.Add(VIdx);
					Mesh.WedgeTexCoords[0].Add(FVector2D{ VertexPositions[VIdx][TA1], VertexPositions[VIdx][TA2] });
				}
			}
		}
	}

	// TODO: Merging, as in StaticMeshEdit.cpp
	Mesh.VertexPositions = VertexPositions;

	for(auto& Tex : Mesh.WedgeTexCoords[0])
	{
		Tex /= TexRepeatUnits;
	}

//	Mesh.WedgeColors.Init(FColor::White, Mesh.WedgeIndices.Num());

	return Mesh.IsValidOrFixable();
}


UStaticMesh* CreateStaticMesh(struct FRawMesh& RawMesh, TArray<UMaterialInterface*>& Materials, UObject* InOuter, FName InName);

bool ConvertInteriorGraphToSMAsset(AInteriorGraphActor* Graph, FString const& PackageName)
{
	FRawMesh Raw;
	if(!ConvertInteriorGraphToRawMesh(Graph, Raw))
	{
		return false;
	}

	FName ObjName = *FPackageName::GetLongPackageAssetName(PackageName);
	UPackage* Pkg = CreatePackage(nullptr, *PackageName);
	check(Pkg != nullptr);

	TArray< UMaterialInterface* > Mats;
	Mats.Add(UMaterial::GetDefaultMaterial(EMaterialDomain::MD_Surface));	//GEditor->EditorBrushMaterial);
	Mats.Add(UMaterial::GetDefaultMaterial(EMaterialDomain::MD_Surface));
	Mats.Add(UMaterial::GetDefaultMaterial(EMaterialDomain::MD_Surface));

	UStaticMesh* SM = CreateStaticMesh(Raw, Mats, Pkg, ObjName);
	if(!SM)
	{
		return false;
	}

	FAssetRegistryModule::AssetCreated(SM);
	return true;
}


/**********************/
/*
Duplicated from Engine/Source/Editor/UnrealEd/Private/StaticMeshEdit.cpp, since the module does not expose it!
*/

/**
* Creates a static mesh object from raw triangle data.
*/
UStaticMesh* CreateStaticMesh(struct FRawMesh& RawMesh, TArray<UMaterialInterface*>& Materials, UObject* InOuter, FName InName)
{
	// Create the UStaticMesh object.
	FStaticMeshComponentRecreateRenderStateContext RecreateRenderStateContext(FindObject<UStaticMesh>(InOuter, *InName.ToString()));
	auto StaticMesh = NewNamedObject<UStaticMesh>(InOuter, InName, RF_Public | RF_Standalone);

	// Add one LOD for the base mesh
	FStaticMeshSourceModel* SrcModel = new(StaticMesh->SourceModels) FStaticMeshSourceModel();
	SrcModel->RawMeshBulkData->SaveRawMesh(RawMesh);
	StaticMesh->Materials = Materials;

	int32 NumSections = StaticMesh->Materials.Num();

	// Set up the SectionInfoMap to enable collision
	for(int32 SectionIdx = 0; SectionIdx < NumSections; ++SectionIdx)
	{
		FMeshSectionInfo Info = StaticMesh->SectionInfoMap.Get(0, SectionIdx);
		Info.MaterialIndex = SectionIdx;
		Info.bEnableCollision = true;
		StaticMesh->SectionInfoMap.Set(0, SectionIdx, Info);
	}

	StaticMesh->Build();
	StaticMesh->MarkPackageDirty();
	return StaticMesh;
}

