// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "InteriorGraphTypes.h"
#include "Set.h"
#include "InteriorGraphActor.generated.h"


#define INTERIOR_GRAPH_DEBUG_NAMES 1


struct FConnectionKey
{
	NodeIdType Src;
	NodeIdType Dest;

	inline bool operator== (FConnectionKey const& K) const
	{
		return Src == K.Src && Dest == K.Dest;
	}
};

#if 0
struct FConnKeyFuncs: BaseKeyFuncs< class AInteriorConnectionActor*, FConnectionKey, false >
{
	typedef class AInteriorConnectionActor* ElementType;
	typedef FConnectionKey KeyType;
	typedef TCallTraits< KeyType >::ParamType KeyInitType;
	typedef TCallTraits< ElementType >::ParamType ElementInitType;

	/**
	* @return The key used to index the given element.
	*/
	// NOTE: Copied from Set.h, however KeyInitType is reference which can't work with locally constructed variable
	static FORCEINLINE /*KeyInitType*/ KeyType GetSetKey(ElementInitType Element)
	{
		return FConnectionKey{ Element->Node1, Element->Node2 };
	}

	/**
	* @return True if the keys match.
	*/
	static FORCEINLINE bool Matches(KeyInitType A, KeyInitType B)
	{
		return A == B;
	}

	/** Calculates a hash index for a key. */
	static FORCEINLINE uint32 GetKeyHash(KeyInitType Key)
	{
		return PointerHash(Key.N1);	// todo: bit random, but probably not important..?
	}
};
#endif

/*
Actor representing the interior graph.
*/
UCLASS(HideCategories = (Transform, Rendering, Input, Actor))
class INTERIOREDITOR_API AInteriorGraphActor: public AActor
{
	GENERATED_BODY()

public:
	/*
	When a level is being played, ***IdType is considered to be an index into these arrays.
	*/
	//UPROPERTY()
	TArray< FNodeData > NodeData;

	//UPROPERTY()
	TArray< FConnectionData > ConnData;

public:
	AInteriorGraphActor();

public:
	/*
	Interface to the graph for when in-game
	*/
	NodeIdList GetAllNodes() const;
	ConnectionIdList GetAllConnections() const;

	FNodeData const& GetNodeData(NodeIdType id) const;
	FConnectionData const& GetConnectionData(ConnectionIdType id) const;

	FNodeData& GetNodeDataRef(NodeIdType id);
	FConnectionData& GetConnectionDataRef(ConnectionIdType id);

	NodeIdList GetAdjacentNodes(NodeIdType src) const;
	NodeIdType GetNodeFromPosition(FVector const& pos) const;
	ConnectionIdList GetNodeOutConnections(NodeIdType id) const;
	ConnectionIdList GetNodeInConnections(NodeIdType id) const;
	ConnectionIdList GetAllNodeConnections(NodeIdType id) const;
	ConnectionIdList GetConnectionsOnFace(NodeIdType NId, struct FFaceId const& Face) const;

public:
	/*
	Interface to the graph when editing
	*/
	NodeIdType AddNode(FVector const& Min, FVector const& Max);
	ConnectionIdType AddConnection(NodeIdType N1, NodeIdType N2, struct FAxisAlignedPlanarArea const& area);
	bool RemoveNode(NodeIdType Id);
	bool RemoveConnection(ConnectionIdType Id);
	int RemoveConnections(NodeIdType N1, NodeIdType N2);

	void SetNodeData(NodeIdType id, FNodeData&& ND);
	void SetConnectionData(ConnectionIdType id, FConnectionData&& CD);

	TSharedPtr< class FInteriorGraphInstance > BuildGraph(int32 Subdivision = 1, int32 SubdivisionZ = 1);

private:
	ConnectionIdType FindFirstConnection(FConnectionKey const& Key) const;
	ConnectionIdType CreateConnection(NodeIdType N1, NodeIdType N2, FAxisAlignedPlanarArea const& area);
	void GetPackedData(
		TArray< FNodeData >& PackedNodes,
		TArray< FConnectionData >& PackedConnections,
		TArray< FString >& NodeNameAr,
		TArray< FString >& ConnNameAr
		) const;

/*	inline NodeIdType GetNodeId(const FNodeData* nd) const
	{
		return nd - NodeData.GetData();
	}
*/

	static void RemoveHiddenNodes(
		TMap< NodeIdType, FNodeData >& BuildND,
		TMap< ConnectionIdType, FConnectionData >& BuildCD,
		UWorld* World
		);
	static void PackNodeAndConnectionData(
		TSharedPtr< FInteriorGraphInstance > Inst,
		TMap< NodeIdType, FNodeData >& BuildND,
		TMap< ConnectionIdType, FConnectionData >& BuildCD
		);

public:
	// Overrides
	virtual void Serialize(FArchive& Ar) override;
	virtual void PreInitializeComponents() override;

	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

protected:
#if WITH_EDITOR
	NodeIdType NextNodeId;
	ConnectionIdType NextConnectionId;

	/*
	In editor, a ***IdType value must be passed through these maps to retrieve the index of the node/connection
	in the NodeData/ConnectionData array.
	*/
	TMap< NodeIdType, int32 > NodeMap;
	TMap< ConnectionIdType, int32 > ConnectionMap;
#endif

#if INTERIOR_GRAPH_DEBUG_NAMES
public:
	TMap< NodeIdType, FString > NodeNames;
	TMap< ConnectionIdType, FString > ConnNames;
#endif

	friend class FGraphSceneProxy;
};


