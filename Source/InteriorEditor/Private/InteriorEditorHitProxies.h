// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "HitProxies.h"
#include "InteriorEditorNodeFace.h"


class HInteriorHitProxy: public HHitProxy
{
	DECLARE_HIT_PROXY(INTERIOREDITOR_API)
};

class HInteriorNodeHitProxy: public HHitProxy
{
	DECLARE_HIT_PROXY(INTERIOREDITOR_API)

	HInteriorNodeHitProxy(NodeIdType NId):
		Node(NId)
	{

	}

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		// TODO: If allow multiple graphs in a level, will need to store ref here
		//		Collector.AddReferencedObject(Face.Node);
	}

	virtual EMouseCursor::Type GetMouseCursor()
	{
		return EMouseCursor::Crosshairs;
	}

	NodeIdType Node;
};

class HInteriorNodeFaceHitProxy: public HInteriorNodeHitProxy
{
	DECLARE_HIT_PROXY(INTERIOREDITOR_API)

	HInteriorNodeFaceHitProxy(NodeIdType NId, EAxisIndex Axis, EAxisDirection Dir):
		HInteriorNodeHitProxy(NId)
		, Face(Axis, Dir)
	{

	}

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		// TODO: If allow multiple graphs in a level, will need to store ref here
//		Collector.AddReferencedObject(Face.Node);
	}

	virtual EMouseCursor::Type GetMouseCursor()
	{
		return EMouseCursor::Crosshairs;
	}

	FFaceId Face;
};

class HInteriorPortalHitProxy: public HHitProxy
{
	DECLARE_HIT_PROXY(INTERIOREDITOR_API)

	HInteriorPortalHitProxy(ConnectionIdType CId):
		Conn(CId)
	{

	}

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		// TODO: If allow multiple graphs in a level, will need to store ref here
		//		Collector.AddReferencedObject(Face.Node);
	}

	virtual EMouseCursor::Type GetMouseCursor()
	{
		return EMouseCursor::Crosshairs;
	}

	ConnectionIdType Conn;
};


