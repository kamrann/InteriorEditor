// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "InteriorEditorUtil.h"


typedef int32 NodeIdType;
typedef int32 ConnectionIdType;

static const NodeIdType NullNode = -1;
static const ConnectionIdType NullConnection = -1;


struct FNodeData
{
	FVector Min;
	FVector Max;

//	TArray< struct FConnectionData* > Outgoing;
	TArray< ConnectionIdType > Outgoing;

	inline FVector Center() const
	{
		return (Min + Max) * 0.5f;
	}

	inline FVector Size() const
	{
		return Max - Min;
	}

	inline FVector HalfSize() const
	{
		return Size() * 0.5f;
	}

	inline FBox Box() const
	{
		return FBox{ Min, Max };
	}

	inline double Volume() const
	{
		auto Extent = Max - Min;
		return (double)Extent.X * (double)Extent.Y * (double)Extent.Z;
	}

	inline float FaceAxisValue(EAxisIndex Axis, EAxisDirection Dir) const
	{
		return Center()[Axis] + HalfSize()[Axis] * GetDirectionMultiplier(Dir);
	}

	inline FVector FaceCenter(EAxisIndex Axis, EAxisDirection Dir) const
	{
		auto FaceOffset = FVector::ZeroVector;
		FaceOffset[Axis] = HalfSize()[Axis] * GetDirectionMultiplier(Dir);
		return Center() + FaceOffset;
	}

	inline FVector FaceNormal(EAxisIndex Axis, EAxisDirection Dir) const
	{
		auto Norm = FVector::ZeroVector;
		Norm[Axis] = GetDirectionMultiplier(Dir);
		return Norm;
	}

	// Currently this just returns a random basis vector in the plane of the face
	inline FVector FaceXTangent(EAxisIndex Axis, EAxisDirection Dir) const
	{
		auto Tan = FVector::ZeroVector;
		auto OtherAxis = (Axis + 1) % 3;
		Tan[OtherAxis] = 1.0f;
		return Tan;
	}

	inline void Offset(FVector const& Off)
	{
		Min += Off;
		Max += Off;
	}

	inline void Extend(EAxisIndex Axis, EAxisDirection Dir, float Delta)
	{
		auto Offset = FVector::ZeroVector;
		Offset[Axis] = Delta;

		switch(Dir)
		{
			case EAxisDirection::Positive:
			Max[Axis] += Delta;
			break;
			case EAxisDirection::Negative:
			Min[Axis] -= Delta;
			break;
		}
	}
};

struct FConnectionData
{
//	const FNodeData* Src;
//	const FNodeData* Dest;
	NodeIdType Src;
	NodeIdType Dest;

	FBox Portal;
};




