// Fill out your copyright notice in the Description page of Project Settings.

#include "InteriorEditorPrivatePCH.h"
#include "InteriorGraphTypes.h"
#include "InteriorGraphInstance.h"


FInteriorGraphInstance::FInteriorGraphInstance()
{

}

int32 FInteriorGraphInstance::NodeCount() const
{
	return NodeData.Num();
}

int32 FInteriorGraphInstance::ConnectionCount() const
{
	return ConnData.Num();
}

FNodeData const& FInteriorGraphInstance::GetNodeData(NodeIdType id) const
{
	return NodeData[id];
}

FConnectionData const& FInteriorGraphInstance::GetConnectionData(ConnectionIdType id) const
{
	return ConnData[id];
}

FNodeData& FInteriorGraphInstance::GetNodeDataRef(NodeIdType id)
{
	return NodeData[id];
}

FConnectionData& FInteriorGraphInstance::GetConnectionDataRef(ConnectionIdType id)
{
	return ConnData[id];
}

/* TODO:
Too much stuff here duplicated from AInteriorGraphActor
*/

NodeIdList FInteriorGraphInstance::GetAdjacentNodes(NodeIdType src) const
{
	// TODO:
	check(false);
	return{};
}

NodeIdType FInteriorGraphInstance::GetNodeFromPosition(FVector const& pos) const
{
	// todo: naive and duplicated from AInteriorNodeActor
	for(NodeIdType Id = 0; Id < NodeData.Num(); ++Id)
	{
		if(NodeData[Id].ContainsPoint(pos))
		{
			return Id;
		}
	}
	return NullNode;
}

ConnectionIdList FInteriorGraphInstance::GetNodeOutConnections(NodeIdType id) const
{
	return GetNodeData(id).Outgoing;
}

ConnectionIdList FInteriorGraphInstance::GetNodeInConnections(NodeIdType id) const
{
	ConnectionIdList In;
	for(ConnectionIdType CId = 0; CId < ConnData.Num(); ++CId)
	{
		if(ConnData[CId].Dest == id)
		{
			In.Add(CId);
		}
	}
	return In;
}

ConnectionIdList FInteriorGraphInstance::GetAllNodeConnections(NodeIdType id) const
{
	//	auto List = GetNodeOutConnections(id);
	//	List += GetNodeInConnections(id);
	auto List = ConnectionIdList{};
	for(ConnectionIdType CId = 0; CId < ConnData.Num(); ++CId)
	{
		if(ConnData[CId].Src == id || ConnData[CId].Dest == id)
		{
			List.AddUnique(CId);
		}
	}
	return List;
}

ConnectionIdList FInteriorGraphInstance::GetConnectionsOnFace(NodeIdType NId, struct FFaceId const& Face) const
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



