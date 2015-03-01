// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UnrealEd.h"
#include "Editor.h"


/*
Editor mode for creating and modifying interiors
*/
class FInteriorEditorMode: public FEdMode
{
public:
	static const FName ModeId;

	FInteriorEditorMode();

public:
	// FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

public:
	/*
	FEdMode overrides
	*/
	virtual void Enter() override;
	virtual void Exit() override;

	virtual bool FInteriorEditorMode::IsSelectionAllowed(
		AActor* InActor,
		bool bInSelection
		) const override;
	
	virtual void ActorMoveNotify() override;
	virtual bool AllowWidgetMove() override;
	virtual bool DisallowMouseDeltaTracking() const override;
	virtual FVector GetWidgetLocation() const override;

	virtual bool HandleClick(FEditorViewportClient* InViewport, HHitProxy* HitProxy, const FViewportClick& Click) override;

	virtual bool InputKey(
		FEditorViewportClient* ViewportClient,
		FViewport* Viewport,
		FKey Key,
		EInputEvent Event
		) override;
	virtual bool InputDelta(
		FEditorViewportClient* InViewportClient,
		FViewport* InViewport,
		FVector& InDrag,
		FRotator& InRot,
		FVector& InScale
		) override;
		
	virtual void DrawHUD(
		FEditorViewportClient* ViewportClient,
		FViewport* Viewport,
		const FSceneView* View,
		FCanvas* Canvas
		) override;

protected:
	typedef TArray< class AInteriorNodeActor* > FNodeList;
	typedef TArray< float > FAxisValueList;

	static inline float DirectionMultiplier(EAxisDirection direction)
	{
		return direction == EAxisDirection::Positive ? 1.f : -1.f;
	}

	static inline EAxisDirection AxisDirectionFromValue(float value)
	{
		return value > 0.f ? EAxisDirection::Positive : (value < 0.f ? EAxisDirection::Negative : EAxisDirection::None);
	}

	static inline EAxisDirection OppositeDirection(EAxisDirection direction)
	{
		switch(direction)
		{
			case EAxisDirection::Positive:		return EAxisDirection::Negative;
			case EAxisDirection::Negative:		return EAxisDirection::Positive;
			default:							return EAxisDirection::None;
		}
	}

	enum class EAxisExtremity {
		OverallMinimum = 1 << 0,
		OverallMaximum = 1 << 1,
		OverallExtremes = OverallMinimum | OverallMaximum,
		// The following are relevant only when dealing with multiple nodes
		IndividualMinimum = (1 << 2) | OverallMinimum,
		IndividualMaximum = (1 << 3) | OverallMaximum,
		IndividualExtremes = IndividualMinimum | IndividualMaximum,

		AnyMinimum = OverallMinimum | IndividualMinimum,
		AnyMaximum = OverallMaximum | IndividualMaximum,
	};

	static inline bool TestAny(EAxisExtremity value, EAxisExtremity flags)
	{
		return ((uint32)value & (uint32)flags) != 0u;
	}

	static inline bool TestAll(EAxisExtremity value, EAxisExtremity flags)
	{
		return ((uint32)value & (uint32)flags) == (uint32)flags;
	}

	//ENUM_CLASS_FLAGS(EAxisExtremity)

	static const float Epsilon;

protected:
	void CheckCreateGizmo();
	void DestroyGizmo();
	void UpdateNodeReferences();
	FNodeList GetSelectedNodes() const;
	FNodeList GetUnselectedNodes() const;

//	static float GetAxisExtremities(class AInteriorNodeActor* node, EAxis::Type axis, EAxisExtremity extremity);

	static bool ParseMovementKey(FKey Key, EAxisIndex& axis, EAxisDirection& direction);
	static void SnapCameraToWorldAxis(FRotator const& CamRot, EAxisIndex& axis, EAxisDirection& direction);

	/*
	Returns an ordered (increasing) list of unique axis values on the given axis, generated from
	the extremities of the provided node list.
	*/
	static FAxisValueList GetDistinctAxisValues(FNodeList const& nodes, EAxisIndex axis, EAxisExtremity extremity);

	static float DetermineNextSnapOffset(FNodeList const& nodes, FNodeList const& ref_nodes, EAxisIndex axis, EAxisDirection direction);

	void TranslateNodes(FNodeList const& nodes, FVector const& offset);
	void SnapNodes(FNodeList const& nodes, FNodeList const& ref_nodes, EAxisIndex axis, EAxisDirection direction);

protected:
	class AInteriorGraphActor* Graph;
	FNodeList Nodes;

	class AInteriorGizmoActor* Gizmo;

public:
	class UInteriorEditorModeSettings* Settings;

	//
	FVector SnapDelta;
};


