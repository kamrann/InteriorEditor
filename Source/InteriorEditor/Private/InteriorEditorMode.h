// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UnrealEd.h"
#include "Editor.h"
#include "InteriorEditorNodeFace.h"


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
	virtual bool UsesTransformWidget(FWidget::EWidgetMode InWidgetMode) const override;
	virtual bool ShouldDrawWidget() const override;
	virtual EAxisList::Type GetWidgetAxisToDraw(FWidget::EWidgetMode InWidgetMode) const override;
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

	virtual void PostUndo() override;

public:
	void GenerateStaticMesh() const;

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

	enum ESelection {
		Node	= 1 << 0,
		Face	= 1 << 1,
		Portal	= 1 << 2,

		Any		= Node | Face | Portal
	};

	typedef uint32 SelectionType;

	typedef TArray< NodeIdType > NodeList;
	typedef TArray< FNodeFaceRef > FaceList;
	typedef TArray< ConnectionIdType > PortalList;

	static const float Epsilon;
	static const float MinimumNodeSize;
	static const float DefaultNodeSize;

protected:
//	void CheckCreateGizmo();
//	void DestroyGizmo();
//	void UpdateNodeReferences();
//	FNodeList GetSelectedNodes() const;
//	FNodeList GetUnselectedNodes() const;

//	static float GetAxisExtremities(class AInteriorNodeActor* node, EAxis::Type axis, EAxisExtremity extremity);

	bool IsNodeSelection() const;
	bool IsFaceSelection() const;
	bool IsPortalSelection() const;
	bool IsSelection(SelectionType ST = ESelection::Any) const;
	bool IsExclusiveSelection(SelectionType ST) const;
	bool IsNodeOnlySelection() const;
	bool IsFaceOnlySelection() const;
	bool IsPortalOnlySelection() const;
	int SelectionCount(SelectionType ST = ESelection::Any) const;

	/*
	Returns the primary selection (node at which the widget is displayed).
	If the current selection is a face, it returns the node of that face.
	*/
	NodeIdType GetFirstSelectedNode() const;
	bool IsAlreadySelected(class HHitProxy* HP) const;
	void Select(class HHitProxy* HP);
	void Deselect(class HHitProxy* HP);

	void UpdateNodeFromSelectionProxy(const class UInteriorNodeSelectionProxy* Prxy);
	bool UpdateSelectionProxyFromNode(NodeIdType NId);
	void UpdateNodeFaceFromSelectionProxy(const class UInteriorFaceSelectionProxy* Prxy);
	bool UpdateSelectionProxyFromNodeFace(FNodeFaceRef F);
	void UpdatePortalFromSelectionProxy(const class UInteriorPortalSelectionProxy* Prxy);
	bool UpdateSelectionProxyFromPortal(ConnectionIdType CId);

	void RegenerateSelectionProxies();

	NodeList GetNodeComplement(NodeList const& Nodes) const;
	NodeList GetNodeComplement(NodeIdType const& Nd) const;
	FaceList GetSelectionComplement(FaceList const& Sel) const;

	void ClearSelection(SelectionType ST = ESelection::Any);

	static bool ParseMovementKey(FKey Key, EAxisIndex& axis, EAxisDirection& direction);
	static void SnapCameraToWorldAxis(FRotator const& CamRot, EAxisIndex& axis, EAxisDirection& direction);

	/*
	Returns an ordered (increasing) list of unique axis values on the given axis, generated from
	the extremities of the provided node list.
	*/
	FAxisValueList GetDistinctAxisValues(NodeList const& nodes, EAxisIndex axis, EAxisExtremity extremity) const;
	
	/*
	Returns an ordered (increasing) list of unique axis values on the given axis, generated from
	the faces perpendicular to the axis within the provided node list.
	FAxisValueList GetDistinctFaceAxisValues(NodeList const& nodes, EAxisIndex axis, EAxisExtremity extremity) const;
*/
	float DetermineNextSnapOffset(FAxisValueList vals, FAxisValueList ref_vals, EAxisIndex axis, EAxisDirection direction) const;
	float DetermineNextSnapOffset(NodeList const& nodes, NodeList const& ref_nodes, EAxisIndex axis, EAxisDirection direction) const;

	void SetNodeMinMax(NodeIdType NId, FVector const& Min, FVector const& Max);
	void TranslateNodes(NodeList const& nodes, FVector const& offset, bool bUpdateProxy);
	void SnapNodes(NodeList const& nodes, NodeList const& ref_nodes, EAxisIndex axis, EAxisDirection direction);

	bool TranslateFace(FNodeFaceRef const& face, float offset, bool bUpdateProxy, bool bRedraw);
	void SnapSingleFace(FNodeFaceRef const& face, NodeList const& ref_nodes, EAxisDirection direction);

	bool TranslatePortal(ConnectionIdType CId, FVector const& offset, bool bUpdateProxy, bool bRedraw);
	bool ResizePortal(ConnectionIdType CId, FVector const& scale, bool bUpdateProxy, bool bRedraw);
	/*
	Snap the portal to the boundary of the shared surface between the two nodes
	*/
	void SnapSinglePortal(ConnectionIdType CId, EAxisIndex axis, EAxisDirection direction);

	bool TryAddPortal(NodeIdType N1, NodeIdType N2);

	void Redraw() const;

	void OnGenerateNamedStaticMesh(FString const& PkgName) const;

protected:
	class AInteriorGraphActor* Graph;
//	FNodeList Nodes;

	NodeList SelectedNodes;
	FaceList SelectedFaces;
	PortalList SelectedPortals;

	TArray< class UInteriorNodeSelectionProxy* > NodeSelectionProxies;
	TArray< class UInteriorFaceSelectionProxy* > FaceSelectionProxies;
	TArray< class UInteriorPortalSelectionProxy* > PortalSelectionProxies;

//	class AInteriorGizmoActor* Gizmo;

public:
	class UInteriorEditorModeSettings* Settings;

	//
	FVector SnapDelta;

	friend class FGraphSceneProxy;
	friend class SInteriorEditor;
	friend class FInteriorGraphDetailsCustomization;
	friend class UInteriorNodeSelectionProxy;
	friend class UInteriorFaceSelectionProxy;
	friend class UInteriorPortalSelectionProxy;
};


