// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "InteriorEditorUtil.h"
#include "InteriorGraphTypes.h"


struct FFaceId
{
	EAxisIndex Axis;
	EAxisDirection Dir;

	FFaceId()
	{}

	FFaceId(EAxisIndex Ax, EAxisDirection D):
		Axis(Ax),
		Dir(D)
	{}
};

struct FNodeFaceRef
{
	NodeIdType NId;
	FFaceId FaceId;

	FNodeFaceRef():
		NId(NullNode)
	{}

	FNodeFaceRef(NodeIdType Id, EAxisIndex Ax, EAxisDirection D):
		NId(Id),
		FaceId(Ax, D)
	{}

	FNodeFaceRef(NodeIdType Id, FFaceId F):
		NId(Id),
		FaceId(F)
	{}

	inline operator bool() const
	{
		return NId != NullNode;
	}

	inline bool operator== (FNodeFaceRef const& Rhs) const
	{
		return NId == Rhs.NId && FaceId.Axis == Rhs.FaceId.Axis && FaceId.Dir == Rhs.FaceId.Dir;
	}
};


