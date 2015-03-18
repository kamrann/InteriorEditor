// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "InteriorGraphTypes.h"


#define INTERIOR_GRAPH_DEBUG_NAMES 1


/*
Actor representing a built instance of an interior graph.
*/
class INTERIOREDITOR_API FInteriorGraphInstance
{
public:
	TArray< FNodeData > NodeData;
	TArray< FConnectionData > ConnData;

public:
	FInteriorGraphInstance();

public:
	typedef TArray< NodeIdType > NodeIdList;
	typedef TArray< ConnectionIdType > ConnectionIdList;

	/*
	Interface to the graph for when in-game
	*/
	int32 NodeCount() const;
	int32 ConnectionCount() const;

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

#if INTERIOR_GRAPH_DEBUG_NAMES
public:
	TMap< NodeIdType, FString > NodeNames;
	TMap< ConnectionIdType, FString > ConnNames;
#endif
};


