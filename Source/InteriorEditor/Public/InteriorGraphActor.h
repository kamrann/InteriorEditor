// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "InteriorGraphTypes.h"
#include "Set.h"
#include "InteriorGraphActor.generated.h"


#define INTERIOR_GRAPH_DEBUG_NAMES 1


struct FConnectionKey
{
	class AInteriorNodeActor* N1;
	class AInteriorNodeActor* N2;

	inline bool operator== (FConnectionKey const& K) const
	{
		return N1 == K.N1 && N2 == K.N2;
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
Actor representing the interior graph
*/
UCLASS()
class INTERIOREDITOR_API AInteriorGraphActor: public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray< class AInteriorNodeActor* > Nodes;

	UPROPERTY()
	TArray< class AInteriorConnectionActor* > Connections;
		// TODO: TSet not serializable. Will this come in future?
//	TSet< class AInteriorConnectionActor*, FConnKeyFuncs > Connections;

public:
	static const NodeIdType NullNode = -1;

	typedef TArray< NodeIdType > NodeIdList;

	static const ConnectionIdType NullConnection = -1;

	typedef TArray< ConnectionIdType > ConnectionIdList;

/*	struct FConnectionNodes
	{
		NodeIdType N1;
		NodeIdType N2;
	};
*/	
	NodeIdList GetAllNodes() const;
	ConnectionIdList GetAllConnections() const;

	FNodeData const& GetNodeData(NodeIdType id) const;
	FConnectionData const& GetConnectionData(ConnectionIdType id) const;
	NodeIdList GetAdjacentNodes(NodeIdType src) const;

	NodeIdType GetNodeFromPosition(FVector const& pos) const;

public:
	bool AddConnection(class AInteriorNodeActor* N1, class AInteriorNodeActor* N2, struct FAxisAlignedPlanarArea const& area);
	bool RemoveConnection(class AInteriorNodeActor* N1, class AInteriorNodeActor* N2);
//	bool ToggleConnection(class AInteriorNodeActor* N1, class AInteriorNodeActor* N2);

	void GatherNodes();
	bool BuildGraph(int32 Subdivision = 1, int32 SubdivisionZ = 1);

private:
	class AInteriorConnectionActor* FindConnection(FConnectionKey const& Key) const;
	class AInteriorConnectionActor* CreateConnection(class AInteriorNodeActor* N1, class AInteriorNodeActor* N2, FAxisAlignedPlanarArea const& area) const;

	inline NodeIdType GetNodeId(const FNodeData* nd) const
	{
		return nd - NodeData.GetData();
	}

	inline bool PointInNode(FVector const& pos, FNodeData const& nd) const
	{
		return
			pos.X >= nd.Min.X && pos.X < nd.Max.X &&
			pos.Y >= nd.Min.Y && pos.Y < nd.Max.Y &&
			pos.Z >= nd.Min.Z && pos.Z < nd.Max.Z
			;
	}

	void RemoveHiddenNodes(TMap< NodeIdType, FNodeData >& BuildND, TMap< ConnectionIdType, FConnectionData >& BuildCD);
	void PackNodeAndConnectionData(TMap< NodeIdType, FNodeData >& BuildND, TMap< ConnectionIdType, FConnectionData >& BuildCD);

public:
	// Overrides
	virtual void PreInitializeComponents() override;

	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

protected:
	TArray< FNodeData > NodeData;
	TArray< FConnectionData > ConnData;

#if INTERIOR_GRAPH_DEBUG_NAMES
public:
	TMap< NodeIdType, FString > NodeNames;
	TMap< ConnectionIdType, FString > ConnNames;
#endif
};


