// Fill out your copyright notice in the Description page of Project Settings.

#include "InteriorEditorPrivatePCH.h"
#include "InteriorGraphActor.h"
#include "InteriorNodeActor.h"
#include "InteriorConnectionActor.h"
#include "InteriorEditorUtil.h"
#include "Engine/World.h"


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

bool AInteriorGraphActor::BuildGraph(int32 Subdivision)
{
	TMap< AInteriorNodeActor*, TArray< NodeIdType > > NodeMap;
	NodeIdType NId = 0;

	GatherNodes();

	auto const SubnodesPerNode = Subdivision * Subdivision;	// temp x-y only
	NodeData.Reset(Nodes.Num() * SubnodesPerNode);
	NodeNames.Reset(NodeData.Num());
	ConnData.Reset();
	for(auto const& N : Nodes)
	{
		auto Pos = N->GetActorLocation();
		auto SubExtent = N->Extent * FVector(1.f / Subdivision, 1.f / Subdivision, 1.f);
		auto Base = Pos - N->Extent * 0.5f;

		NodeMap.Add(N, TArray < NodeIdType > {});

		auto IdxBase = NodeData.Num();
		for(int32 x = 0; x < Subdivision; ++x)
		{
			for(int32 y = 0; y < Subdivision; ++y)
			{
				auto NData = FNodeData{
					Base + SubExtent * FVector(x, y, 0.f),
					Base + SubExtent * FVector(x + 1, y + 1, 1.f)
				};
				NodeData.Add(NData);

				FString Nm = FText::Format(
					FText::FromString(TEXT("{0}[{1}][{2}]")),
					FText::FromString(N->GetName()),
					FText::AsNumber(x),
					FText::AsNumber(y)
					).ToString();;
				NodeNames.Add(Nm);

				NodeMap[N].Add(NId++);
			}
		}

		for(int32 Idx = IdxBase; Idx < NodeData.Num(); ++Idx)
		{
			auto Offset = Idx - IdxBase;
			auto x = Offset / Subdivision;
			auto y = Offset % Subdivision;

			// TODO: Create connection portals!!!!!!!!!!!!!!!!!
			if(x != 0)
			{
				auto DstOffset = (x - 1) * Subdivision + y;
				auto DstIdx = IdxBase + DstOffset;
				auto CnIdx = ConnData.Add(FConnectionData{ Idx, DstIdx });
				NodeData[Idx].Outgoing.Add(CnIdx);
			}
			if(x != Subdivision - 1)
			{
				auto DstOffset = (x + 1) * Subdivision + y;
				auto DstIdx = IdxBase + DstOffset;
				auto CnIdx = ConnData.Add(FConnectionData{ Idx, DstIdx });
				NodeData[Idx].Outgoing.Add(CnIdx);
			}
			if(y != 0)
			{
				auto DstOffset = x * Subdivision + (y - 1);
				auto DstIdx = IdxBase + DstOffset;
				auto CnIdx = ConnData.Add(FConnectionData{ Idx, DstIdx });
				NodeData[Idx].Outgoing.Add(CnIdx);
			}
			if(y != Subdivision - 1)
			{
				auto DstOffset = x * Subdivision + (y + 1);
				auto DstIdx = IdxBase + DstOffset;
				auto CnIdx = ConnData.Add(FConnectionData{ Idx, DstIdx });
				NodeData[Idx].Outgoing.Add(CnIdx);
			}
		}
	}

	// Now add pre-established connections
	for(auto const& C : Connections)
	{
		auto const& PotentialN1 = NodeMap[C->Node1];
		auto const& PotentialN2 = NodeMap[C->Node2];
		auto Portal = FBox{ C->GetMin(), C->GetMax() };

		for(auto N1 : PotentialN1)
		{
			FAxisAlignedPlanarArea Surf1;
			if(TestForSharedSurface(NodeData[N1].Box(), Portal, 1.e-4f, &Surf1))
			{
				for(auto N2 : PotentialN2)
				{
					FAxisAlignedPlanarArea Surf2;
					if(TestForSharedSurface(NodeData[N2].Box(), Portal, 1.e-4f, &Surf2))
					{
						if(TestForSharedSurface(
							FBox{ Surf1.Min, Surf1.Max },
							FBox{ Surf2.Min, Surf2.Max },
							1.e-4f))
						{
							auto CnIdx = ConnData.Add(FConnectionData{ N1, N2, });
							NodeData[N1].Outgoing.Add(CnIdx);

							CnIdx = ConnData.Add(FConnectionData{ N2, N1 });
							NodeData[N2].Outgoing.Add(CnIdx);
						}
					}
				}
			}
		}
	}
	
	//
	for(NodeIdType NId = 0; NId < NodeData.Num(); ++NId)
	{
		FString Nm = FText::Format(
			FText::FromString(TEXT("{0}-({1})")),
			FText::FromString(NodeNames[NId]),
			FText::AsNumber(NodeData[NId].Outgoing.Num())
			).ToString();
		NodeNames[NId] = Nm;
	}
	//

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


void AInteriorGraphActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// TODO: Currently doing this here to ensure before BeginPlay of dependent actors.
	// Eventually, probably want to call this externally.
	BuildGraph(3);
}

void AInteriorGraphActor::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(InThis, Collector);

	int uu = 0;
}

