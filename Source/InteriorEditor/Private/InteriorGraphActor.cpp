// Fill out your copyright notice in the Description page of Project Settings.

#include "InteriorEditorPrivatePCH.h"
#include "InteriorGraphActor.h"
#include "InteriorGraphInstance.h"
#include "InteriorEditorUtil.h"
#include "InteriorEditorNodeFace.h"
#include "Engine/World.h"
#include "VisibilityHelpers.h"
#include "InteriorGraphRenderingComponent.h"

#include <algorithm>


struct TArrayPred
{
	TArrayPred(FConnectionKey key): key_(key)
	{}

/*	inline bool operator() (AInteriorConnectionActor* conn) const
	{
		return FConnectionKey{ conn->Node1, conn->Node2 } == key_;
	}
*/
	inline bool operator() (FConnectionData const& conn) const
	{
		return FConnectionKey{ conn.Src, conn.Dest } == key_;
	}

	FConnectionKey key_;
};


inline bool operator== (FConnectionData const& Conn, FConnectionKey const& Key)
{
	return FConnectionKey{ Conn.Src, Conn.Dest } == Key;
}


AInteriorGraphActor::AInteriorGraphActor():
NextNodeId(0),
NextConnectionId(0)
{
	RootComponent = CreateEditorOnlyDefaultSubobject< UInteriorGraphRenderingComponent >(TEXT("RenderComp"));
}

NodeIdList AInteriorGraphActor::GetAllNodes() const
{
	NodeIdList Ids;
	Ids.Reserve(NodeMap.Num());
	for(auto const& Entry: NodeMap)
	{
		Ids.Add(Entry.Key);
	}
	return Ids;
}

ConnectionIdList AInteriorGraphActor::GetAllConnections() const
{
	ConnectionIdList Ids;
	Ids.Reserve(ConnectionMap.Num());
	for(auto const& Entry : ConnectionMap)
	{
		Ids.Add(Entry.Key);
	}
	return Ids;
}

NodeIdList AInteriorGraphActor::GetAdjacentNodes(NodeIdType src) const
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
#if WITH_EDITOR
	return NodeData[NodeMap[id]];
#else
	return NodeData[id];
#endif
}

FConnectionData const& AInteriorGraphActor::GetConnectionData(ConnectionIdType id) const
{
#if WITH_EDITOR
	return ConnData[ConnectionMap[id]];
#else
	return ConnData[id];
#endif
}

FNodeData& AInteriorGraphActor::GetNodeDataRef(NodeIdType id)
{
#if WITH_EDITOR
	return NodeData[NodeMap[id]];
#else
	return NodeData[id];
#endif
}

FConnectionData& AInteriorGraphActor::GetConnectionDataRef(ConnectionIdType id)
{
#if WITH_EDITOR
	return ConnData[ConnectionMap[id]];
#else
	return ConnData[id];
#endif
}

NodeIdType AInteriorGraphActor::GetNodeFromPosition(FVector const& pos) const
{
	// todo: naive
	for(NodeIdType Id = 0; Id < NodeData.Num(); ++Id)
	{
		if(NodeData[Id].ContainsPoint(pos))
		{
			return Id;
		}
	}
	return NullNode;
}

ConnectionIdList AInteriorGraphActor::GetNodeOutConnections(NodeIdType id) const
{
	return GetNodeData(id).Outgoing;
}

ConnectionIdList AInteriorGraphActor::GetNodeInConnections(NodeIdType id) const
{
	ConnectionIdList In;
	for(auto const& Cn : ConnectionMap)
	{
		if(GetConnectionData(Cn.Value).Dest == id)
		{
			In.Add(Cn.Key);
		}
	}
	return In;
}

ConnectionIdList AInteriorGraphActor::GetAllNodeConnections(NodeIdType id) const
{
//	auto List = GetNodeOutConnections(id);
//	List += GetNodeInConnections(id);
	auto List = ConnectionIdList{};
	for(auto const& Cn : ConnectionMap)
	{
		if(GetConnectionData(Cn.Value).Src == id ||
			GetConnectionData(Cn.Value).Dest == id)
		{
			List.AddUnique(Cn.Key);
		}
	}
	return List;
}

ConnectionIdList AInteriorGraphActor::GetConnectionsOnFace(NodeIdType NId, FFaceId const& Face) const
{
	auto Conns = GetAllNodeConnections(NId);
	// TODO: Will want to differentiate between physical portals (just one even if it is bidirectional) and
	// directed connections (of which there would be 2 referring to the same portal in the case of bidirectional)
		
	auto const& ND = GetNodeData(NId);
	TArray< ConnectionIdType > FaceConns;
	for(auto const& CId : Conns)
	{
		auto CD = GetConnectionData(CId);

		auto PortalCenter = CD.Portal.GetCenter();
		// TODO: This is a bit of a hacky test, should really just store more face/axis info with the portal
		if(FMath::IsNearlyEqual(PortalCenter[Face.Axis], ND.FaceAxisValue(Face.Axis, Face.Dir), 0.01f))
		{
			FaceConns.Add(CId);
		}
	}

	return FaceConns;
}


/*
Editing interface implementation
*/
NodeIdType AInteriorGraphActor::AddNode(FVector const& Min, FVector const& Max)
{
	FNodeData Nd;
	Nd.Min = Min;
	Nd.Max = Max;

	auto Idx = NodeData.Add(Nd);
	auto Id = NextNodeId++;
	NodeMap.Add(Id, Idx);

#if WITH_EDITOR
	FString Nm = TEXT("Node ");
	Nm.AppendInt(Id);
	NodeNames.Add(Id, Nm);
#endif

	return Id;
}

ConnectionIdType AInteriorGraphActor::AddConnection(
	//class AInteriorNodeActor* N1,
	//class AInteriorNodeActor* N2,
	NodeIdType N1,
	NodeIdType N2,
	FAxisAlignedPlanarArea const& area
	)
{
/*	if(ConnData.ContainsByPredicate(TArrayPred{ FConnectionKey{ N1, N2 } }))
	{
		return NullConnection;
	}
*/
	return CreateConnection(N1, N2, area);
}

int AInteriorGraphActor::RemoveConnections(NodeIdType N1, NodeIdType N2)
{
	auto Key = FConnectionKey{ N1, N2 };

	ConnectionIdType CId;
	int Count = 0;
	while((CId = FindFirstConnection(Key)) != NullConnection)
	{
		//ConnData.RemoveAll(TArrayPred{ Key });
		auto bRemoved = RemoveConnection(CId);
		check(bRemoved);
		++Count;
	}

	return Count;
}

void AInteriorGraphActor::SetNodeData(NodeIdType id, FNodeData&& ND)
{
	auto& Node = GetNodeDataRef(id);
	Node = std::move(ND);
}

void AInteriorGraphActor::SetConnectionData(ConnectionIdType id, FConnectionData&& CD)
{
	auto& Conn = GetConnectionDataRef(id);
	Conn = std::move(CD);
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

void AInteriorGraphActor::RemoveHiddenNodes(
	TMap< NodeIdType, FNodeData >& BuildND,
	TMap< ConnectionIdType, FConnectionData >& BuildCD,
	UWorld* World
	)
{
	for(auto It = BuildND.CreateIterator(); It; ++It)
	{
		auto& ND = It->Value;
		auto Pos = ND.Center();
		auto Hidden = UVisibilityHelpers::IsPointHidden(Pos, World, ECollisionChannel::ECC_WorldStatic);

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

void AInteriorGraphActor::PackNodeAndConnectionData(
	TSharedPtr< FInteriorGraphInstance > Inst,
	TMap< NodeIdType, FNodeData >& BuildND,
	TMap< ConnectionIdType, FConnectionData >& BuildCD
	)
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

	Inst->NodeData.Init(FNodeData{}, BuildND.Num());
	for(auto& ND : BuildND)
	{
		for(auto& Out : ND.Value.Outgoing)
		{
			Out = ConnCondense[Out];
		}

		Inst->NodeData[NodeCondense[ND.Key]] = std::move(ND.Value);
	}

	Inst->ConnData.Reset();
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

		Inst->ConnData.Add(std::move(CD.Value));
	}

	//
	TMap< NodeIdType, FString > NewNodeNames;
	for(auto& NdNm : Inst->NodeNames)
	{
		auto Ptr = NodeCondense.Find(NdNm.Key);
		if(Ptr)
		{
			auto MappedKey = *Ptr;
			FString Nm = FText::Format(
				FText::FromString(TEXT("{0}:{1}-({2})")),
				FText::FromString(NdNm.Value),
				FText::AsNumber(MappedKey),
				FText::AsNumber(Inst->NodeData[MappedKey].Outgoing.Num())
				).ToString();
			NewNodeNames.Add(MappedKey, Nm);
		}
	}
	Inst->NodeNames = std::move(NewNodeNames);
	//
}

TSharedPtr< FInteriorGraphInstance > AInteriorGraphActor::BuildGraph(int32 Subdivision, int32 SubdivisionZ)
{
	// Map from original Id to instance node Id
	TMap< NodeIdType, TArray< NodeIdType > > OriginalNodeMap;

	TMap< NodeIdType, FNodeData > BuildND;
	TMap< ConnectionIdType, FConnectionData > BuildCD;
	NodeIdType NId = 0;
	ConnectionIdType CId = 0;

	TSharedPtr< FInteriorGraphInstance > Inst = MakeShareable(new FInteriorGraphInstance);

	for(auto const& N : NodeMap)
	{
		auto const& ND = NodeData[N.Value];

		auto SubExtent = ND.Size() * FVector(1.f / Subdivision, 1.f / Subdivision, 1.f / SubdivisionZ);
		auto Base = ND.Min;

		OriginalNodeMap.Add(N.Key, TArray < NodeIdType > {});

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

					FString Nm = NodeNames[N.Key];
					Nm += FText::Format(
						FText::FromString(TEXT("[{0}][{1}][{2}]")),
						FText::AsNumber(x),
						FText::AsNumber(y),
						FText::AsNumber(z)
						).ToString();
					Inst->NodeNames.Add(NId, Nm);

					OriginalNodeMap[N.Key].Add(NId);

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

	RemoveHiddenNodes(BuildND, BuildCD, GetWorld());
	PackNodeAndConnectionData(Inst, BuildND, BuildCD);
	return Inst;
}

bool AInteriorGraphActor::RemoveNode(NodeIdType Id)
{
	if(!NodeMap.Contains(Id))
	{
		return false;
	}

	// Remove all connection into and out of this node
	auto Conns = GetAllNodeConnections(Id);
	for(auto CId : Conns)
	{
		RemoveConnection(CId);
	}

	// Finally, remove the node itself
	// Note we are currently just removing the map key without condensing the array and remapping
	NodeData[NodeMap[Id]] = FNodeData{};
	NodeNames.Remove(Id);
	NodeMap.Remove(Id);

	return true;
}

bool AInteriorGraphActor::RemoveConnection(ConnectionIdType Id)
{
	if(!ConnectionMap.Contains(Id))
	{
		return false;
	}

	auto& Cn = ConnData[ConnectionMap[Id]];
	
	// TODO: Decide what to do with in/out and bidirectional
	GetNodeDataRef(Cn.Src).Outgoing.Remove(Id);
	GetNodeDataRef(Cn.Dest).Outgoing.Remove(Id);

	Cn = FConnectionData{};
	ConnectionMap.Remove(Id);
	return true;
}

ConnectionIdType AInteriorGraphActor::FindFirstConnection(FConnectionKey const& Key) const
{
	auto ElemPtr = ConnData.FindByPredicate(TArrayPred{ Key });
	if(ElemPtr)
	{
		auto Idx = ElemPtr - ConnData.GetData();
		auto KeyPtr = ConnectionMap.FindKey(Idx);
		check(KeyPtr);
		return *KeyPtr;
	}

	return NullConnection;
}

ConnectionIdType AInteriorGraphActor::CreateConnection(
	NodeIdType N1,
	NodeIdType N2,
	FAxisAlignedPlanarArea const& area)
{
	FConnectionData Conn;
	Conn.Src = N1;
	Conn.Dest = N2;
	Conn.Portal = FBox{ area.Min, area.Max };

	auto Idx = ConnData.Add(Conn);
	auto Id = NextConnectionId++;
	ConnectionMap.Add(Id, Idx);

#if WITH_EDITOR
	FString Nm = TEXT("Connection ");
	Nm.AppendInt(Id);
	ConnNames.Add(Id, Nm);
#endif

	return Id;
}

void AInteriorGraphActor::GetPackedData(
	TArray< FNodeData >& PackedNodes,
	TArray< FConnectionData >& PackedConnections,
	TArray< FString >& NodeNameAr,
	TArray< FString >& ConnNameAr
	) const
{
	PackedNodes.Empty(NodeMap.Num());
	PackedConnections.Empty(ConnectionMap.Num());

/*	NodeMap.KeySort([](NodeIdType A, NodeIdType B)
	{
		return A < B;
	});
*/	TMap< NodeIdType, int32 > NodeIdMap;
	for(auto const& Nd : NodeMap)
	{
		NodeIdMap.Add(Nd.Key, NodeIdMap.Num());
	}

/*	ConnectionMap.KeySort([](ConnectionIdType A, ConnectionIdType B)
	{
		return A < B;
	});
*/	TMap< ConnectionIdType, int32 > ConnectionIdMap;
	for(auto const& Cn : ConnectionMap)
	{
		ConnectionIdMap.Add(Cn.Key, ConnectionIdMap.Num());
	}

	for(auto const& Nd : NodeMap)
	{
		auto ND = NodeData[Nd.Value];
		for(auto& Out : ND.Outgoing)
		{
			Out = ConnectionIdMap[Out];
		}

		auto NId = PackedNodes.Add(ND);
		NodeNameAr.Add(NodeNames[Nd.Key]);
//		NewNodeMap.Add(NId, NId);
	}

	for(auto const& Cn : ConnectionMap)
	{
		auto CD = ConnData[Cn.Value];
		CD.Src = NodeIdMap[CD.Src];
		CD.Dest = NodeIdMap[CD.Dest];
		
		auto CId = PackedConnections.Add(CD);
		ConnNameAr.Add(ConnNames[Cn.Key]);
//		NewConnectionMap.Add(CId, CId);
	}

/*	NodeData = std::move(NewNodes);
	ConnData = std::move(NewConnections);
	NodeMap = std::move(NewNodeMap);
	ConnectionMap = std::move(NewConnectionMap);
	*/
}


struct NodeRecord
{
	FVector Min, Max;
};

FArchive& operator<< (FArchive& Ar, NodeRecord& Rec)
{
	Ar << Rec.Min << Rec.Max;
	return Ar;
}

struct ConnectionRecord
{
	int32 Src, Dest;
	FBox Portal;
};

FArchive& operator<< (FArchive& Ar, ConnectionRecord& Rec)
{
	Ar << Rec.Src << Rec.Dest << Rec.Portal;
	return Ar;
}

void AInteriorGraphActor::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	// TODO: Ideally, we should serialize a flag specifying whether or not the graph has been saved with or
	// without node/connection names.

	TArray< NodeRecord > NodeRecords;
	TArray< ConnectionRecord > ConnectionRecords;
	TArray< FString > NodeNameArray;
	TArray< FString > ConnNameArray;
	if(Ar.IsSaving())
	{
		// Ensure ids are reduced to 0-based with no gaps. This way we don't need to serialize them, since
		// they are just equivalent to the position within the serialized array.
		TArray< FNodeData > PackedNodes;
		TArray< FConnectionData > PackedConnections;
		GetPackedData(PackedNodes, PackedConnections, NodeNameArray, ConnNameArray);

		for(auto const& Nd : PackedNodes)
		{
			NodeRecord NR;
			NR.Min = Nd.Min;
			NR.Max = Nd.Max;
			NodeRecords.Add(NR);
		}

		for(auto const& Cn : PackedConnections)
		{
			ConnectionRecord CR;
			CR.Src = Cn.Src;
			CR.Dest = Cn.Dest;
			CR.Portal = Cn.Portal;
			ConnectionRecords.Add(CR);
		}
	}

	Ar << NodeRecords;
	Ar << ConnectionRecords;
	
//	if(Ar.IsSaving())
	{
		Ar << NodeNameArray;
		Ar << ConnNameArray;
	}

	if(Ar.IsLoading())
	{
		NodeData.Empty(NodeRecords.Num());
		NodeMap.Empty(NodeRecords.Num());
		for(auto const& NR : NodeRecords)
		{
			FNodeData Nd;
			Nd.Min = NR.Min;
			Nd.Max = NR.Max;
			auto Idx = NodeData.Add(Nd);

			NodeMap.Add(Idx, Idx);

			//
//			NodeNames.Add(Idx, FString{});
			//
		}

		NodeNames.Empty(NodeRecords.Num());
		for(auto const& NNm : NodeNameArray)
		{
			NodeNames.Add(NodeNames.Num(), NNm);
		}

		ConnData.Empty(ConnectionRecords.Num());
		ConnectionMap.Empty(ConnectionRecords.Num());
		for(auto const& CR : ConnectionRecords)
		{
			FConnectionData Cn;
			Cn.Src = CR.Src;
			Cn.Dest = CR.Dest;
			Cn.Portal = CR.Portal;
			auto Idx = ConnData.Add(Cn);

			ConnectionMap.Add(Idx, Idx);

			NodeData[Cn.Src].Outgoing.Add(Idx);

			//
//			ConnNames.Add(Idx, FString{});
			//
		}

		ConnNames.Empty(ConnectionRecords.Num());
		for(auto const& CNm : ConnNameArray)
		{
			ConnNames.Add(ConnNames.Num(), CNm);
		}

		NextNodeId = NodeMap.Num();
		NextConnectionId = ConnectionMap.Num();
	}
}

void AInteriorGraphActor::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AInteriorGraphActor::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(InThis, Collector);
}

