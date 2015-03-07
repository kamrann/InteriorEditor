// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

/*
TODO: Move some stuff to KantanUtil
*/


/*
Replacement for the 1-based EAxis
*/
enum EAxisIndex {
	X,
	Y,
	Z,

	Count,
	None = -1,
};

enum class EAxisDirection {
	Positive = 1,
	Negative = -1,
	None = 0,
};

struct FAxisUtils
{
	static const EAxisIndex OtherAxes[EAxisIndex::Count][2];

	static inline bool IsBoxWithinBounds(FBox const& Box, EAxisIndex Axis, float Min, float Max)
	{
		return Box.Min[Axis] >= Min && Box.Max[Axis] <= Max;
	}
};

struct FAxisAlignedPlanarArea
{
	EAxisIndex FixedAxis;
	float FixedAxisValue;

	/*
	Points on 2D plane defined by FixedAxis == FixedAxisValue.
	X and Y in this plane are defined with Z (the FixedAxis) coming out of the plane.
	*/
	//		FVector2D Min, Max;
	FVector Min, Max;
};

/*
Enum defining the relative states of two AABBs along a particular axis
*/
enum class EAABBRelativeAxisState {
	Disjoint,		// Max of one box < Min of other box
	Aligned,		// Max of one box == Min of other box (within epsilon)
	Overlapping,	// Max of one box > Min of other box
};


inline float GetDirectionMultiplier(EAxisDirection Dir)
{
	return Dir == EAxisDirection::Positive ? 1.0f : (Dir == EAxisDirection::Negative ? -1.0f : 0.0f);
}

float CalculateRelativeAxisSeparation(EAxisIndex axis, FBox const& Box1, FBox const& Box2);
EAABBRelativeAxisState CalculateRelativeAxisState(EAxisIndex axis, FBox const& Box1, FBox const& Box2, float Epsilon = 1.e-4f);
bool TestForSharedSurface(FBox const& Box1, FBox const& Box2, float Epsilon = 1.e-4f, FAxisAlignedPlanarArea* OutArea = nullptr);


