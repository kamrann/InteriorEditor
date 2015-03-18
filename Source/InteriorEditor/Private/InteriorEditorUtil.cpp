// Fill out your copyright notice in the Description page of Project Settings.

#include "InteriorEditorPrivatePCH.h"
#include "InteriorEditorUtil.h"


const EAxisIndex FAxisUtils::AllAxes[EAxisIndex::Count] = {
	EAxisIndex::X,
	EAxisIndex::Y,
	EAxisIndex::Z,
};

// Perhaps there is an ideal ordering defined by the handedness of the coordinate system?
const EAxisIndex FAxisUtils::OtherAxes[EAxisIndex::Count][2] = {
	{ EAxisIndex::Y, EAxisIndex::Z },
	{ EAxisIndex::Z, EAxisIndex::X },
	{ EAxisIndex::X, EAxisIndex::Y },
};

const EAxisDirection FAxisUtils::BothDirections[2] = {
	EAxisDirection::Positive,
	EAxisDirection::Negative,
};


float CalculateRelativeAxisSeparation(EAxisIndex axis, FBox const& Box1, FBox const& Box2)
{
	return Box1.Min[axis] < Box2.Min[axis] ?
		Box2.Min[axis] - Box1.Max[axis] :
		Box1.Min[axis] - Box2.Max[axis];
}

EAABBRelativeAxisState CalculateRelativeAxisState(EAxisIndex axis, FBox const& Box1, FBox const& Box2, float Epsilon)
{
	auto separation = CalculateRelativeAxisSeparation(axis, Box1, Box2);
	if(separation > Epsilon)
	{
		return EAABBRelativeAxisState::Disjoint;
	}
	else if(separation < -Epsilon)
	{
		return EAABBRelativeAxisState::Overlapping;
	}
	else
	{
		return EAABBRelativeAxisState::Aligned;
	}
}

bool TestForSharedSurface(
	FBox const& Box1,
	FBox const& Box2,
	float Epsilon,
	FAxisAlignedPlanarArea* OutArea
	)
{
	EAABBRelativeAxisState axis_states[3];
	EAxisIndex aligned_idx = EAxisIndex::None;
	EAxisIndex other_axes[3];
	int32 iOther = 0;
	for(int32 idx = 0; idx < EAxisIndex::Count; ++idx)
	{
		auto e_idx = (EAxisIndex)idx;
		axis_states[idx] = CalculateRelativeAxisState(e_idx, Box1, Box2, Epsilon);

		if(axis_states[idx] == EAABBRelativeAxisState::Aligned && aligned_idx == EAxisIndex::None)
		{
			aligned_idx = e_idx;
			continue;
		}

		if(axis_states[idx] != EAABBRelativeAxisState::Overlapping)
		{
			return false;
		}

		other_axes[iOther++] = e_idx;
	}

	if(aligned_idx == EAxisIndex::None)
	{
		return false;
	}

	/*
	We have 1 aligned axis and 2 overlapping axes, so we have a shared surface area.
	*/
	if(OutArea)
	{
		OutArea->FixedAxis = aligned_idx;
		if(Box1.Min[aligned_idx] < Box2.Min[aligned_idx])
		{
			OutArea->FixedAxisValue = Box1.Max[aligned_idx];
		}
		else
		{
			OutArea->FixedAxisValue = Box2.Max[aligned_idx];
		}

		auto sep1 = CalculateRelativeAxisSeparation(other_axes[0], Box1, Box2);
		auto sep2 = CalculateRelativeAxisSeparation(other_axes[1], Box1, Box2);
		OutArea->Min[OutArea->FixedAxis] = OutArea->FixedAxisValue;
		OutArea->Max[OutArea->FixedAxis] = OutArea->FixedAxisValue;
		for(int32 other_idx = 0; other_idx < 2; ++other_idx)
		{
			auto ax = other_axes[other_idx];
			OutArea->Min[ax] = (Box1.Min[ax] < Box2.Min[ax]) ?
				Box2.Min[ax] :
				Box1.Min[ax];
			OutArea->Max[ax] = (Box1.Max[ax] < Box2.Max[ax]) ?
				Box1.Max[ax] :
				Box2.Max[ax];
		}
	}

	return true;
}

