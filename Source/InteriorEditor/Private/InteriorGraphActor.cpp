// Fill out your copyright notice in the Description page of Project Settings.

#include "InteriorEditorPrivatePCH.h"
#include "InteriorGraphActor.h"
#include "InteriorNodeActor.h"
#include "InteriorConnectionActor.h"
#include "InteriorEditorUtil.h"
#include "Engine/World.h"
#include "VisibilityHelpers.h"

#include <algorithm>


struct TArrayPred
{
	TArrayPred(FConnectionKey key): key_(key)
	{}

	inline bool operator() (AInteriorConnectionActor* conn) const
	{
		return FConnectionKey{ conn->Node1, conn->Node2 } == key_;
	}

	FConnectionKey key_;
};


inline bool operator== (AInteriorConnectionActor const* Conn, FConnectionKey const& Key)
{
	return FConnectionKey{ Conn->Node1, Conn->Node2 } == Key;
}


AInteriorGraphActor::NodeIdList AInteriorGraphActor::GetAllNodes() const
{
	NodeIdList Ids;
	Ids.Init(NodeData.Num());
	NodeIdType i = 0;
	for(auto& Id : Ids)
	{
		Id = i++;
	}
	return Ids;
}

AInteriorGraphActor::ConnectionIdList AInteriorGraphActor::GetAllConnections() const
{
	ConnectionIdList Ids;
	Ids.Init(ConnData.Num());
	ConnectionIdType i = 0;
	for(auto& Id : Ids)
	{
		Id = i++;
	}
	return Ids;
}

AInteriorGraphActor::NodeIdList AInteriorGraphActor::GetAdjacentNodes(NodeIdType src) const
{
	auto const& N = NodeData[src];
	NodeIdList Adj;
	for(auto const& C : N.Outgoing)
	{
		Adj.Add(ConnData[C].Dest);
	}
	return Adj;
}

FNodeData const& AInteriorGraphActor::GetNodeData(NodeIdType id) const
{
	return NodeData[id];
}

FConnectionData const& AInteriorGraphActor::GetConnectionData(ConnectionIdType id) const
{
	return ConnData[id];
}

NodeIdType AInteriorGraphActor::GetNodeFromPosition(FVector const& pos) const
{
	// todo: naive
	for(NodeIdType Id = 0; Id < NodeData.Num(); ++Id)
	{
		if(PointInNode(pos, NodeData[Id]))
		{
			return Id;
		}
	}
	return NullNode;
}


bool AInteriorGraphActor::AddConnection(
	class AInteriorNodeActor* N1,
	class AInteriorNodeActor* N2,
	FAxisAlignedPlanarArea const& area
	)
{
//	if(Connections.Contains(FConnectionKey{ N1, N2 }))
	if(Connections.ContainsByPredicate(TArrayPred{ FConnectionKey{ N1, N2 } }))
	{
		return false;
	}

	auto World = GetWorld();
	auto Conn = CreateConnection(N1, N2, area);
	if(Conn)
	{
		Connections.Add(Conn);
		return true;
	}
	else
	{
		return false;
	}
}

bool AInteriorGraphActor::RemoveConnection(class AInteriorNodeActor* N1, class AInteriorNodeActor* N2)
{
	auto Key = FConnectionKey{ N1, N2 };
	/*	auto Id = Connections.FindId(Key);
	if(Id.IsValidId())
	{
	todo: destroy actor 
	Connections.Remove(Key);
	return true;
	}
	*/

	auto Conn = FindConnection(Key);
	if(Conn)
	{
		// TODO: Or RemoveActor()? What's difference?
		GetWorld()->DestroyActor(Conn);
		Connections.RemoveAll(TArrayPred{ Key });
		return true;
	}

	return false;
}
#if 0
bool AInteriorGraphActor::ToggleConnection(class AInteriorNodeActor* N1, class AInteriorNodeActor* N2)
{
	auto Key = FConnectionKey{ N1, N2 };
/*	auto Id = Connections.FindId(Key);
	if(Id.IsValidId())
	{
		Connections.Remove(Key);
		return false;
	}
	*/
	if(RemoveConnection(N1, N2))
	{
		return false;
	}
	else
	{
		auto Conn = CreateConnection(N1, N2);
		if(Conn)
		{
			Connections.Add(Conn);
			return true;
		}
		else
		{
			return false;
		}
	}
}
#endif

void AInteriorGraphActor::GatherNodes()
{
	Nodes.Empty();
	for(auto It = TActorIterator< AInteriorNodeActor >(GetWorld()); It; ++It)
	{
		Nodes.Add(*It);
	}
}

void AInteriorGraphActor::RemoveHiddenNodes(TMap< NodeIdType, FNodeData >& BuildND, TMap< ConnectionIdType, FConnectionData >& BuildCD)
{
	for(auto It = BuildND.CreateIterator(); It; ++It)
	{
		auto& ND = It->Value;
		auto Pos = ND.Center();
		auto Hidden = UVisibilityHelpers::IsPointHidden(Pos, GetWorld(), ECollisionChannel::ECC_WorldStatic);

		if(Hidden)
		{
			for(auto Out : ND.Outgoing)
			{
				BuildCD.Remove(Out);
			}

			It.RemoveCurrent();
		}
	}
}

void AInteriorGraphActor::PackNodeAndConnectionData(TMap< NodeIdType, FNodeData >& BuildND, TMap< ConnectionIdType, FConnectionData >& BuildCD)
{
	TMap< NodeIdType, NodeIdType > NodeCondense;
	TMap< ConnectionIdType, ConnectionIdType > ConnCondense;

	// Sort for debugging consistency
	BuildND.KeySort([](NodeIdType A, NodeIdType B)
	{
		return A < B;
	});
	for(auto const& ND : BuildND)
	{
		NodeCondense.Add(ND.Key, NodeCondense.Num());
	}

	BuildCD.KeySort([](ConnectionIdType A, ConnectionIdType B)
	{
		return A < B;
	});
	for(auto const& CD : BuildCD)
	{
		ConnCondense.Add(CD.Key, ConnCondense.Num());
	}

	NodeData.Init(FNodeData{}, BuildND.Num());
	for(auto& ND : BuildND)
	{
		for(auto& Out : ND.Value.Outgoing)
		{
			Out = ConnCondense[Out];
		}

		NodeData[NodeCondense[ND.Key]] = std::move(ND.Value);
	}

	ConnData.Reset();
	for(auto& CD : BuildCD)
	{
		if(
			!NodeCondense.Contains(CD.Value.Src) ||
			!NodeCondense.Contains(CD.Value.Dest)
			)
		{
			// Node has been removed, so omit this connection
			continue;
		}

		CD.Value.Src = NodeCondense[CD.Value.Src];
		CD.Value.Dest = NodeCondense[CD.Value.Dest];

		ConnData.Add(std::move(CD.Value));
	}

	//
	TMap< NodeIdType, FString > NewNodeNames;
	for(auto& NdNm : NodeNames)
	{
		auto Ptr = NodeCondense.Find(NdNm.Key);
		if(Ptr)
		{
			auto MappedKey = *Ptr;
			FString Nm = FText::Format(
				FText::FromString(TEXT("{0}:{1}-({2})")),
				FText::FromString(NdNm.Value),
				FText::AsNumber(MappedKey),
				FText::AsNumber(NodeData[MappedKey].Outgoing.Num())
				).ToString();
			NewNodeNames.Add(MappedKey, Nm);
		}
	}
	NodeNames = std::move(NewNodeNames);
	//
}

bool AInteriorGraphActor::BuildGraph(int32 Subdivision, int32 SubdivisionZ)
{
	TMap< AInteriorNodeActor*, TArray< NodeIdType > > NodeMap;

	GatherNodes();

	TMap< NodeIdType, FNodeData > BuildND;
	TMap< ConnectionIdType, FConnectionData > BuildCD;
	NodeIdType NId = 0;
	ConnectionIdType CId = 0;

	for(auto const& N : Nodes)
	{
		auto Pos = N->GetActorLocation();
		auto SubExtent = N->Extent * FVector(1.f / Subdivision, 1.f / Subdivision, 1.f / SubdivisionZ);
		auto Base = Pos - N->Extent * 0.5f;

		NodeMap.Add(N, TArray < NodeIdType > {});

		auto IdxBase = BuildND.Num();
		for(int32 x = 0; x < Subdivision; ++x)
		{
			for(int32 y = 0; y < Subdivision; ++y)
			{
				for(int32 z = 0; z < SubdivisionZ; ++z)
				{
					auto NData = FNodeData{
						Base + SubExtent * FVector(x, y, z),
						Base + SubExtent * FVector(x + 1, y + 1, z + 1)
					};
					BuildND.Add(NId, NData);
					NodeNames.Add(NId, N->GetName());

					NodeMap[N].Add(NId);

					++NId;
				}
			}
		}

		/* TODO:
		for(int32 Idx = IdxBase; Idx < BuildND.Num(); ++Idx)
		{
			auto Offset = Idx - IdxBase;
			auto x = Offset / Subdivision;
			auto y = Offset % Subdivision;

			// TODO: Create connection portals!!!!!!!!!!!!!!!!!
			if(x != 0)
			{
				auto DstOffset = (x - 1) * Subdivision + y;
				auto DstIdx = IdxBase + DstOffset;
				BuildCD.Add(CId, FConnectionData{ Idx, DstIdx });
				BuildND[Idx].Outgoing.Add(CId);
				++CId;
			}
			if(x != Subdivision - 1)
			{
				auto DstOffset = (x + 1) * Subdivision + y;
				auto DstIdx = IdxBase + DstOffset;
				BuildCD.Add(CId, FConnectionData{ Idx, DstIdx });
				BuildND[Idx].Outgoing.Add(CId);
				++CId;
			}
			if(y != 0)
			{
				auto DstOffset = x * Subdivision + (y - 1);
				auto DstIdx = IdxBase + DstOffset;
				BuildCD.Add(CId, FConnectionData{ Idx, DstIdx });
				BuildND[Idx].Outgoing.Add(CId);
				++CId;
			}
			if(y != Subdivision - 1)
			{
				auto DstOffset = x * Subdivision + (y + 1);
				auto DstIdx = IdxBase + DstOffset;
				BuildCD.Add(CId, FConnectionData{ Idx, DstIdx });
				BuildND[Idx].Outgoing.Add(CId);
				++CId;
			}
		}
		*/
	}

/*	TODO:
	// Now add pre-established connections
	for(auto const& C : Connections)
	{
		auto const& PotentialN1 = NodeMap[C->Node1];
		auto const& PotentialN2 = NodeMap[C->Node2];
		auto Portal = FBox{ C->GetMin(), C->GetMax() };

		for(auto N1 : PotentialN1)
		{
			FAxisAlignedPlanarArea Surf1;
			if(TestForSharedSurface(BuildND[N1].Box(), Portal, 1.e-4f, &Surf1))
			{
				for(auto N2 : PotentialN2)
				{
					FAxisAlignedPlanarArea Surf2;
					if(TestForSharedSurface(BuildND[N2].Box(), Portal, 1.e-4f, &Surf2))
					{
						if(TestForSharedSurface(
							FBox{ Surf1.Min, Surf1.Max },
							FBox{ Surf2.Min, Surf2.Max },
							1.e-4f))
						{
							BuildCD.Add(CId, FConnectionData{ N1, N2, });
							BuildND[N1].Outgoing.Add(CId);
							++CId;

							BuildCD.Add(CId, FConnectionData{ N2, N1 });
							BuildND[N2].Outgoing.Add(CId);
							++CId;
						}
					}
				}
			}
		}
	}
	*/

	RemoveHiddenNodes(BuildND, BuildCD);
	PackNodeAndConnectionData(BuildND, BuildCD);
	return true;
}

AInteriorConnectionActor* AInteriorGraphActor::FindConnection(FConnectionKey const& Key) const
{
	/*
	return Connections.Find(Key);
	*/
	auto result = Connections.FindByKey(Key);
	return result == nullptr ? nullptr : *result;
}

AInteriorConnectionActor* AInteriorGraphActor::CreateConnection(
	class AInteriorNodeActor* N1,
	class AInteriorNodeActor* N2,
	FAxisAlignedPlanarArea const& area) const
{
	auto Conn = GetWorld()->SpawnActor< AInteriorConnectionActor >();
	if(Conn)
	{
		Conn->Node1 = N1;
		Conn->Node2 = N2;
		Conn->Extent = area.Max - area.Min;
		Conn->SetActorLocation((area.Min + area.Max) / 2.f);
		Cast< UPrimitiveComponent >(Conn->GetRootComponent())->MarkRenderStateDirty();
	}
	return Conn;
}


void AInteriorGraphActor::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	// TODO: Currently doing this here to ensure before BeginPlay of dependent actors.
	// !!! AND also before InitializeComponent() of all components of ALL actors.
	// Eventually, probably want to call this externally.
	BuildGraph(10, 3);
}

void AInteriorGraphActor::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(InThis, Collector);
}

