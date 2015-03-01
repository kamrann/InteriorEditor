// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "HitProxies.h"


class HInteriorNodeFaceHitProxy: public HHitProxy
{
	DECLARE_HIT_PROXY(INTERIOREDITOR_API)

	HInteriorNodeFaceHitProxy(AInteriorNodeActor* Nd, EAxisIndex Axis, EAxisDirection Dir):
		Node(Nd), FaceAxis(Axis), FaceDir(Dir)
	{

	}

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObject(Node);
	}

	virtual EMouseCursor::Type GetMouseCursor()
	{
		return EMouseCursor::Crosshairs;
	}

	AInteriorNodeActor* Node;
	EAxisIndex FaceAxis;
	EAxisDirection FaceDir;
};


