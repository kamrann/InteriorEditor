// Fill out your copyright notice in the Description page of Project Settings.

#include "InteriorEditorPrivatePCH.h"
#include "InteriorEditorMode.h"
#include "InteriorGraphActor.h"
#include "InteriorNodeActor.h"
#include "InteriorGizmoActor.h"
#include "InteriorEditorHitProxies.h"
#include "InteriorEditorModeSettings.h"
#include "SInteriorEditor.h"
#include "Editor/UnrealEd/Public/Toolkits/ToolkitManager.h"
#include "ScopedTransaction.h"

#include <algorithm>


const FName FInteriorEditorMode::ModeId = TEXT("Interior");
const float FInteriorEditorMode::Epsilon = 0.1f;
const float FInteriorEditorMode::MinimumNodeSize = 10.0f;
const float FInteriorEditorMode::DefaultNodeSize = 300.0f;

FInteriorEditorMode::FInteriorEditorMode()
{
	Graph = nullptr;
//	Gizmo = nullptr;

	Settings = ConstructObject< UInteriorEditorModeSettings >(
		UInteriorEditorModeSettings::StaticClass(), GetTransientPackage(), NAME_None, RF_Transactional);
	Settings->SetParent(this);
}

void FInteriorEditorMode::AddReferencedObjects(FReferenceCollector& Collector)
{
	// Call parent implementation
	FEdMode::AddReferencedObjects(Collector);

	Collector.AddReferencedObject(Settings);
}


void FInteriorEditorMode::Enter()
{
	FEdMode::Enter();

	UE_LOG(LogTemp, Log, TEXT("Entering InteriorEditorMode"));

	if(Graph == nullptr)
	{
		auto Itr = TActorIterator< AInteriorGraphActor >(GetWorld());
		if(Itr)
		{
			Graph = *Itr;
		}
	}

	GEditor->SelectNone(false, true);

//	CheckCreateGizmo();
	//GEditor->SelectActor(Gizmo, true, true);

	/*
	Repopulate our list of node actors, since when we were in a different mode, we do not know if
	node actors were created or destroyed.
	*/
//	UpdateNodeReferences();

	// Load settings from config file
	// TODO: Settings->Load();

	// Create the editor mode window
	if(!Toolkit.IsValid())
	{
		Toolkit = MakeShareable(new FInteriorToolKit);
		Toolkit->Init(Owner->GetToolkitHost());
	}

	SnapDelta = FVector::ZeroVector;
}

void FInteriorEditorMode::Exit()
{
	if(Toolkit.IsValid())
	{
		FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
		Toolkit.Reset();
	}

//	DestroyGizmo();

	SelectedNodes.Empty();
	SelectedFaces.Empty();// = FNodeFaceRef{};

	Redraw();

	FEdMode::Exit();
}

bool FInteriorEditorMode::IsSelectionAllowed(AActor* InActor, bool bInSelection) const
{
	// TODO: Probably want to be more strict depending on sel/desel and existing selection
	return false;// InActor->IsA< AInteriorGizmoActor >() || InActor->IsA< AInteriorNodeActor >();
}

void FInteriorEditorMode::ActorMoveNotify()
{
	SnapDelta = FVector::ZeroVector;

	FEdMode::ActorMoveNotify();
}

bool FInteriorEditorMode::AllowWidgetMove()
{
	return true;// false;
}

bool FInteriorEditorMode::DisallowMouseDeltaTracking() const
{
	return false;// true;
}

bool FInteriorEditorMode::UsesTransformWidget(FWidget::EWidgetMode InWidgetMode) const
{
	switch(InWidgetMode)
	{
		case FWidget::EWidgetMode::WM_Translate:
		case FWidget::EWidgetMode::WM_Scale:
		return true;

		default:
		return false;
	}
}

bool FInteriorEditorMode::ShouldDrawWidget() const
{
	return IsExclusiveSelection(ESelection::Node) ||
		IsExclusiveSelection(ESelection::Face) ||
		IsExclusiveSelection(ESelection::Portal);
}

EAxisList::Type FInteriorEditorMode::GetWidgetAxisToDraw(FWidget::EWidgetMode InWidgetMode) const
{
	switch(InWidgetMode)
	{
		case FWidget::EWidgetMode::WM_Translate:
		if(IsExclusiveSelection(ESelection::Node))
		{
			return EAxisList::All;
		}
		else if(IsExclusiveSelection(ESelection::Face))
		{
			switch(SelectedFaces[0].FaceId.Axis)
			{
				case EAxisIndex::X:	return EAxisList::X;
				case EAxisIndex::Y:	return EAxisList::Y;
				case EAxisIndex::Z:	return EAxisList::Z;
			}
		}
		else if(IsExclusiveSelection(ESelection::Portal))
		{
			// todo:
			return EAxisList::All;
		}
		break;

		case FWidget::EWidgetMode::WM_Scale:
		if(IsExclusiveSelection(ESelection::Portal))
		{
			// todo:
			return EAxisList::All;
		}
		break;
	}

	return EAxisList::None;
}

FVector FInteriorEditorMode::GetWidgetLocation() const
{
/*	auto Sel = GetFirstSelectedActorInstance();
	if(Sel && CurrentWidgetAxis != EAxisList::None)
	{
		return Sel->GetActorLocation();
	}
	else
*/	
	if(IsExclusiveSelection(ESelection::Node))
	{
		return Graph->GetNodeData(SelectedNodes[0]).Center();
	}
	else if(IsExclusiveSelection(ESelection::Face))
	{
		return Graph->GetNodeData(SelectedFaces[0].NId).FaceCenter(SelectedFaces[0].FaceId.Axis, SelectedFaces[0].FaceId.Dir);
	}
	else if(IsExclusiveSelection(ESelection::Portal))
	{
		return Graph->GetConnectionData(SelectedPortals[0]).Portal.GetCenter();
	}

	return FVector::ZeroVector;
}

bool FInteriorEditorMode::HandleClick(FEditorViewportClient* InViewport, HHitProxy* HitProxy, const FViewportClick& Click)
{
	// Ignore event since this method handles actual 'clicks' ie. down and up without movement
	// Use InputKey if need to handle raw click 
	if(/*Click.GetEvent() == EInputEvent::IE_Pressed &&*/ Click.GetKey() == EKeys::LeftMouseButton)
	{
		if(HitProxy && HitProxy->IsA(HInteriorHitProxy::StaticGetType()))
		{
			auto HP = static_cast<HInteriorHitProxy*>(HitProxy);
			auto bAlreadySelected = IsAlreadySelected(HP);

			if(bAlreadySelected)
			{
				if(Click.IsControlDown())
				{
					Deselect(HP);
					Redraw();
				}
				else
				{
					// Do nothing
				}
			}
			else
			{
				if(!Click.IsControlDown())
				{
					ClearSelection();
				}

				Select(HP);
				Redraw();
			}
			return true;
		}
		else
		{
			ClearSelection();
			Redraw();
			return true;
		}
	}
	else
	{
		return FEdMode::HandleClick(InViewport, HitProxy, Click);
	}
}

bool FInteriorEditorMode::InputKey(
	FEditorViewportClient* ViewportClient,
	FViewport* Viewport,
	FKey Key,
	EInputEvent Event
	)
{
	EAxisIndex axis;
	EAxisDirection direction;

	if(Event == EInputEvent::IE_Pressed
		&& Key.ToString() == TEXT("N"))
	{
		FScopedTransaction Trans(TEXT("InteriorEditor"), FText::FromString(TEXT("Add Node")), Graph);
		Graph->Modify();

		auto Pos = FVector::ZeroVector;
		auto Extent = FVector{ DefaultNodeSize, DefaultNodeSize, DefaultNodeSize };

		auto SelNode = GetFirstSelectedNode();
		if(SelNode != NullNode)
		{
			auto const& ND = Graph->GetNodeData(SelNode);
			auto Offset = ND.Size();
			if(IsFaceSelection())
			{
				auto F = SelectedFaces[0].FaceId;
				Offset = FVector::ZeroVector;
				Offset[F.Axis] = ND.Size()[F.Axis] * GetDirectionMultiplier(F.Dir);
			}
			Pos = ND.Center() + Offset;
			Extent = ND.Size();
		}

		auto Min = Pos - Extent / 2;
		auto Max = Pos + Extent / 2;
		Graph->AddNode(Min, Max);
	}
	else if(Event == EInputEvent::IE_Pressed
		&& Key.ToString() == TEXT("Delete")
		&& IsNodeOnlySelection())
	{
		FScopedTransaction Trans(TEXT("InteriorEditor"), FText::FromString(TEXT("Delete Node(s)")), Graph);
		Graph->Modify();

		for(auto const& NId : SelectedNodes)
		{
			Graph->RemoveNode(NId);
		}
		ClearSelection();
	}
	else if(Event == EInputEvent::IE_Pressed
//		&& Viewport->KeyState(EKeys::LeftAlt)
		&& Key.ToString() == TEXT("X")
		&& IsNodeOnlySelection()
		&& SelectedNodes.Num() == 2)
	{
/*		auto bRemoved = Graph->RemoveConnections(SelectedNodes[0], SelectedNodes[1]);
		if(bRemoved)
		{
			return true;
		}
*/
		// Try to add
		FAxisAlignedPlanarArea Surface;
		if(TestForSharedSurface(
			Graph->GetNodeData(SelectedNodes[0]).Box(),
			Graph->GetNodeData(SelectedNodes[1]).Box(),
			Epsilon,
			&Surface))
		{
			FScopedTransaction Trans(TEXT("InteriorEditor"), FText::FromString(TEXT("Add Connection")), Graph);
			Graph->Modify();

			Graph->AddConnection(SelectedNodes[0], SelectedNodes[1], Surface);
			Redraw();
		}
		return true;
	}
	else if(Event == EInputEvent::IE_Pressed
		&& Viewport->KeyState(EKeys::LeftAlt)
		&& ParseMovementKey(Key, axis, direction))
	{
		SnapCameraToWorldAxis(ViewportClient->GetViewRotation(), axis, direction);
		if(IsExclusiveSelection(ESelection::Node))
		{
			SnapNodes(SelectedNodes, GetNodeComplement(SelectedNodes), axis, direction);
		}
		else if(IsExclusiveSelection(ESelection::Face) && SelectedFaces[0].FaceId.Axis == axis)
		{
			SnapSingleFace(SelectedFaces[0], GetNodeComplement(SelectedFaces[0].NId), direction);
		}
		else if(IsExclusiveSelection(ESelection::Portal))
		{
			SnapSinglePortal(SelectedPortals[0], axis, direction);
		}
		return true;
	}

	return FEdMode::InputKey(ViewportClient, Viewport, Key, Event);
}

bool FInteriorEditorMode::InputDelta(
	FEditorViewportClient* InViewportClient,
	FViewport* InViewport,
	FVector& InDrag,
	FRotator& InRot,
	FVector& InScale
	)
{
	if(GetCurrentWidgetAxis() != EAxisList::None)
	{
		if(InViewportClient->GetWidgetMode() == FWidget::EWidgetMode::WM_Translate
			&& IsExclusiveSelection(ESelection::Node))
		{
			TranslateNodes(SelectedNodes, InDrag);
		}
		else if(InViewportClient->GetWidgetMode() == FWidget::EWidgetMode::WM_Translate
			&& IsExclusiveSelection(ESelection::Face))
		{
			FScopedTransaction Trans(TEXT("InteriorEditor"), FText::FromString(TEXT("Resize Node(s)")), Graph);
			Graph->Modify();

			bool bAltered = false;
			for(auto const& F : SelectedFaces)	// TODO: Either enforce single selection for faces, or check here 
				// that we are not moving the same node multiple times
			{
				bAltered = bAltered || TranslateFace(F, InDrag[F.FaceId.Axis], false);
			}

			if(bAltered)
			{
				Redraw();
			}
			else
			{
				Trans.Cancel();
			}
		}
		else if(InViewportClient->GetWidgetMode() == FWidget::EWidgetMode::WM_Translate
			&& IsExclusiveSelection(ESelection::Portal))
		{
			TranslatePortal(SelectedPortals[0], InDrag, true);
		}
		else if(InViewportClient->GetWidgetMode() == FWidget::EWidgetMode::WM_Scale
			&& IsExclusiveSelection(ESelection::Portal))
		{
			ResizePortal(SelectedPortals[0], InScale, true);
		}
	}

#if 0
	// TODO: This is currently tied into snap settings and probably shouldn't be. Maybe better to store
	// accumulated deltas until we want to apply them.
	auto WgtAxes = InViewportClient->GetCurrentWidgetAxis();
	auto Sel = GetSelectedNodes();
	if(WgtAxes != EAxisList::None && Sel.Num() > 0)
	{
#if 0
		auto const SnapSize = 200.f;

		auto RootActor = GetFirstSelectedActorInstance();
		if(RootActor)
		{
			SnapDelta += InDrag;

			auto RootPos = RootActor->GetActorLocation();
			auto DragPos = RootPos + SnapDelta;
			FVector SnappedPos;
			for(int c = 0; c < 3; ++c)
			{
				auto Rem = FMath::Fmod(DragPos[c] + SnapSize / 2, SnapSize);
				SnappedPos[c] = DragPos[c] + SnapSize / 2 - Rem;
			}
			auto ActualOffset = SnappedPos - RootPos;

			if(!ActualOffset.IsNearlyZero())
			{
				//Owner->SetPivotLocation(RootPos + ActualOffset, false);

				auto nodes = GetSelectedNodes();
				for(auto n : nodes)
				{
					auto Pos = n->GetActorLocation();
					n->SetActorLocation(Pos + ActualOffset);
				}

				SnapDelta -= ActualOffset;
			}

			InDrag = ActualOffset;
		}
		return true;
#endif

#if 0
		auto Unsel = GetUnselectedNodes();

		if((WgtAxes & EAxisList::X) != 0 && !FMath::IsNearlyZero(InDrag.X, Epsilon))
		{
			InDrag.X = DetermineNextSnapOffset(Sel, Unsel, EAxisIndex::X, AxisDirectionFromValue(InDrag.X));
		}
		if((WgtAxes & EAxisList::Y) != 0 && !FMath::IsNearlyZero(InDrag.Y, Epsilon))
		{
			InDrag.Y = DetermineNextSnapOffset(Sel, Unsel, EAxisIndex::Y, AxisDirectionFromValue(InDrag.Y));
		}
		/*			if((WgtAxes & EAxisList::Z) != 0 && !FMath::IsNearlyZero(InDrag.Z, Epsilon))
					{
					InDrag.Z = DetermineNextSnapOffset(Sel, Unsel, EAxisIndex::Z, AxisDirectionFromValue(InDrag.Z));
					}
					*/
		//			InRot = FRotator::ZeroRotator;
		//			InScale = FVector::ZeroVector;	// NOTE: This is correct, it's some kind of relative percentage scale
		
		return false;
#endif
	}
#endif

	return FEdMode::InputDelta(InViewportClient, InViewport, InDrag, InRot, InScale);
}

void FInteriorEditorMode::DrawHUD(
	FEditorViewportClient* ViewportClient,
	FViewport* Viewport,
	const FSceneView* View,
	FCanvas* Canvas
	)
{
	FEdMode::DrawHUD(ViewportClient, Viewport, View, Canvas);

	auto ti = FCanvasTextItem{
		FVector2D{ 0.f, 0.f },
		FText::FromString(TEXT("Interior Editing Mode")),
		GetStatsFont(),
		FColor::White
	};
	ti.Draw(Canvas);
}

/*
void FInteriorEditorMode::CheckCreateGizmo()
{
	if(Gizmo)
	{
		return;
	}

	Gizmo = GetWorld()->SpawnActor< AInteriorGizmoActor >();
}

void FInteriorEditorMode::DestroyGizmo()
{
	if(Gizmo)
	{
		GetWorld()->DestroyActor(Gizmo);
		Gizmo = nullptr;
	}
}

void FInteriorEditorMode::UpdateNodeReferences()
{
	Nodes.Empty();
	for(TActorIterator< AInteriorNodeActor > Itr(GetWorld()); Itr; ++Itr)
	{
		Nodes.Add(*Itr);
	}
}
*/

#if 0
FInteriorEditorMode::FNodeList FInteriorEditorMode::GetSelectedNodes() const
{
	// TODO: May be better to just use this->Nodes and AActor::IsSelected?
	FNodeList Sel;
	GEditor->GetSelectedActors()->GetSelectedObjects< AInteriorNodeActor >(Sel);
	return Sel;
}

FInteriorEditorMode::FNodeList FInteriorEditorMode::GetUnselectedNodes() const
{
	auto Unsel = Nodes;
	auto Sel = GetSelectedNodes();

	struct ContainedInPred
	{
		ContainedInPred(FNodeList const& set):
			set_(set)
		{}

		inline bool operator() (AInteriorNodeActor* n) const
		{
			return set_.Contains(n);
		}

		FNodeList const& set_;
	};

	Unsel.RemoveAll(ContainedInPred{ Sel });
	return Unsel;
}
#endif

FInteriorEditorMode::FAxisValueList FInteriorEditorMode::GetDistinctAxisValues(NodeList const& nodes, EAxisIndex axis, EAxisExtremity extremity) const
{
	FAxisValueList minimums, maximums;

	for(auto const& NId : nodes)
	{
		auto const& Nd = Graph->GetNodeData(NId);

		if(TestAny(extremity, EAxisExtremity::AnyMinimum))
		{
			auto value = Nd.Min[axis];
			if(minimums.Num() == 0 || TestAny(extremity, EAxisExtremity::IndividualMinimum))
			{
				minimums.Add(value);
			}
			else if(value < minimums[0])
			{
				minimums[0] = value;
			}
		}

		if(TestAny(extremity, EAxisExtremity::AnyMaximum))
		{
			auto value = Nd.Max[axis];
			if(maximums.Num() == 0 || TestAny(extremity, EAxisExtremity::IndividualMaximum))
			{
				maximums.Add(value);
			}
			else if(value < maximums[0])
			{
				maximums[0] = value;
			}
		}
	}

	FAxisValueList values;
	values.Append(minimums);
	values.Append(maximums);
	values.Sort();
	
	for(auto idx = 1; idx < values.Num(); )
	{
		if(FMath::IsNearlyEqual(values[idx], values[idx - 1], Epsilon))
		{
			values.RemoveAt(idx, 1, false);
		}
		else
		{
			++idx;
		}
	}

	return values;
}

float FInteriorEditorMode::DetermineNextSnapOffset(FAxisValueList vals, FAxisValueList ref_vals, EAxisIndex axis, EAxisDirection direction) const
{
	if(vals.Num() == 0 || ref_vals.Num() == 0)
	{
		return 0.f;
	}

	float multiplier = 1.f;
	if(direction == EAxisDirection::Negative)
	{
		std::reverse(&vals[0], &vals[0] + vals.Num());
		std::reverse(&ref_vals[0], &ref_vals[0] + ref_vals.Num());
		multiplier = -1.f;
	}

	/*
	Find the next snap offset in the given direction.
	(Probably a naive implementation)
	*/
	float closest = 0.f;
	int32 r_idx = 0;
	for(int32 v_idx = 0; v_idx < vals.Num(); ++v_idx)
	{
		auto const v1 = vals[v_idx] * multiplier;
		for(; r_idx < ref_vals.Num(); ++r_idx)
		{
			auto const v2 = ref_vals[r_idx] * multiplier;
			auto offset = v2 - v1;
			if(offset > Epsilon)
			{
				if(closest == 0.f || offset < closest)
				{
					closest = offset;
				}
				break;
			}
		}
	}

	return closest;
}

float FInteriorEditorMode::DetermineNextSnapOffset(NodeList const& nodes, NodeList const& ref_nodes, EAxisIndex axis, EAxisDirection direction) const
{
	if(nodes.Num() == 0 || ref_nodes.Num() == 0)
	{
		return 0.f;
	}

	/*
	Determine extremities along the axis of both the nodes and the reference nodes
	*/
	auto node_values = GetDistinctAxisValues(nodes, axis, EAxisExtremity::IndividualExtremes);
	auto ref_values = GetDistinctAxisValues(ref_nodes, axis, EAxisExtremity::IndividualExtremes);

	return DetermineNextSnapOffset(std::move(node_values), std::move(ref_values), axis, direction);
}

void FInteriorEditorMode::TranslateNodes(NodeList const& nodes, FVector const& offset)
{
	if(nodes.Num() == 0)
	{
		return;
	}

	FScopedTransaction Trans(TEXT("InteriorEditor"), FText::FromString(TEXT("Move Node(s)")), Graph);
	Graph->Modify();

	for(auto NId : nodes)
	{
		auto Nd = Graph->GetNodeData(NId);
		Nd.Offset(offset);
		Graph->SetNodeData(NId, std::move(Nd));
	}

	Redraw();
}

void FInteriorEditorMode::SnapNodes(NodeList const& nodes, NodeList const& ref_nodes, EAxisIndex axis, EAxisDirection direction)
{
	auto offset = DetermineNextSnapOffset(nodes, ref_nodes, axis, direction);
	if(offset != 0.f)
	{
		FVector vec = FVector::ZeroVector;
		vec[axis] = offset * DirectionMultiplier(direction);
		TranslateNodes(nodes, vec);
	}
}

bool FInteriorEditorMode::TranslateFace(FNodeFaceRef const& face, float offset, bool bUpdate)
{
	auto ND = Graph->GetNodeData(face.NId);
	if(face.FaceId.Dir == EAxisDirection::Negative)
	{
		offset = -offset;
	}
	ND.Extend(face.FaceId.Axis, face.FaceId.Dir, offset);

	if(ND.Size()[face.FaceId.Axis] > MinimumNodeSize)
	{
		FScopedTransaction Trans(TEXT("InteriorEditor"), FText::FromString(TEXT("Resize Node")), Graph);
		Graph->Modify();

		Graph->SetNodeData(face.NId, std::move(ND));

		if(bUpdate)
		{
			Redraw();
		}

		return true;
	}

	return false;
}

void FInteriorEditorMode::SnapSingleFace(FNodeFaceRef const& face, NodeList const& ref_nodes, EAxisDirection direction)
{
	auto face_vals = FAxisValueList{};
	auto const& ND = Graph->GetNodeData(face.NId);
	face_vals.Add(ND.FaceAxisValue(face.FaceId.Axis, face.FaceId.Dir));
	
	auto ref_vals = GetDistinctAxisValues(ref_nodes, face.FaceId.Axis, EAxisExtremity::IndividualExtremes);

	auto offset = DetermineNextSnapOffset(std::move(face_vals), std::move(ref_vals), face.FaceId.Axis, direction);
	if(offset != 0.f)
	{
		TranslateFace(face, offset * DirectionMultiplier(direction), true);
	}
}

bool FInteriorEditorMode::TranslatePortal(ConnectionIdType CId, FVector const& offset, bool bUpdate)
{
	auto CD = Graph->GetConnectionData(CId);
	auto const& N1 = Graph->GetNodeData(CD.Src);
	auto const& N2 = Graph->GetNodeData(CD.Dest);
	FAxisAlignedPlanarArea Surface;
	if(!TestForSharedSurface(
		N1.Box(),
		N2.Box(),
		Epsilon,
		&Surface))
	{
		// Shouldn't happen!
		ensure(false);
		return false;
	}

	CD.Portal = CD.Portal.ShiftBy(offset);

	auto const Axis1 = FAxisUtils::OtherAxes[Surface.FixedAxis][0];
	auto const Axis2 = FAxisUtils::OtherAxes[Surface.FixedAxis][1];
	if(
		FAxisUtils::IsBoxWithinBounds(CD.Portal, Axis1, Surface.Min[Axis1], Surface.Max[Axis1])
		&& FAxisUtils::IsBoxWithinBounds(CD.Portal, Axis2, Surface.Min[Axis2], Surface.Max[Axis2])
		)
	{
		FScopedTransaction Trans(TEXT("InteriorEditor"), FText::FromString(TEXT("Move Portal")), Graph);
		Graph->Modify();

		Graph->SetConnectionData(CId, std::move(CD));

		if(bUpdate)
		{
			Redraw();
		}

		return true;
	}

	return false;
}

bool FInteriorEditorMode::ResizePortal(ConnectionIdType CId, FVector const& scale, bool bUpdate)
{
	auto CD = Graph->GetConnectionData(CId);
	auto const& N1 = Graph->GetNodeData(CD.Src);
	auto const& N2 = Graph->GetNodeData(CD.Dest);
	FAxisAlignedPlanarArea Surface;
	if(!TestForSharedSurface(
		N1.Box(),
		N2.Box(),
		Epsilon,
		&Surface))
	{
		// Shouldn't happen!
		ensure(false);
		return false;
	}

	auto HalfSize = CD.Portal.GetExtent();
	HalfSize *= FVector{ 1, 1, 1, } + scale;
	CD.Portal = FBox::BuildAABB(CD.Portal.GetCenter(), HalfSize);

	auto const Axis1 = FAxisUtils::OtherAxes[Surface.FixedAxis][0];
	auto const Axis2 = FAxisUtils::OtherAxes[Surface.FixedAxis][1];
	if(
		FAxisUtils::IsBoxWithinBounds(CD.Portal, Axis1, Surface.Min[Axis1], Surface.Max[Axis1])
		&& FAxisUtils::IsBoxWithinBounds(CD.Portal, Axis2, Surface.Min[Axis2], Surface.Max[Axis2])
		)
	{
		FScopedTransaction Trans(TEXT("InteriorEditor"), FText::FromString(TEXT("Resize Portal")), Graph);
		Graph->Modify();

		Graph->SetConnectionData(CId, std::move(CD));

		if(bUpdate)
		{
			Redraw();
		}

		return true;
	}

	return false;
}

void FInteriorEditorMode::SnapSinglePortal(ConnectionIdType CId, EAxisIndex axis, EAxisDirection direction)
{
	auto CD = Graph->GetConnectionData(CId);
	auto const& N1 = Graph->GetNodeData(CD.Src);
	auto const& N2 = Graph->GetNodeData(CD.Dest);
	FAxisAlignedPlanarArea Surface;
	if(!TestForSharedSurface(
		N1.Box(),
		N2.Box(),
		Epsilon,
		&Surface))
	{
		// Shouldn't happen!
		ensure(false);
		return;
	}

	if(axis == Surface.FixedAxis)
	{
		// Can't move along the fixed axis
		return;
	}

	float Delta = 0.0f;
	switch(direction)
	{
		case EAxisDirection::Positive:
		Delta = Surface.Max[axis] - CD.Portal.Max[axis];
		break;

		case EAxisDirection::Negative:
		Delta = Surface.Min[axis] - CD.Portal.Min[axis];
		break;
	}

	if(!FMath::IsNearlyZero(Delta, 1.0e-4f))
	{
		auto Offset = FVector::ZeroVector;
		Offset[axis] = Delta;
		ensure(TranslatePortal(CId, Offset, true));
	}
}

void FInteriorEditorMode::Redraw() const
{
	Graph->GetRootComponent()->MarkRenderStateDirty();
}

bool FInteriorEditorMode::IsNodeSelection() const
{
	return SelectedNodes.Num() > 0;
}

bool FInteriorEditorMode::IsFaceSelection() const
{
	return SelectedFaces.Num() > 0;
}

bool FInteriorEditorMode::IsPortalSelection() const
{
	return SelectedPortals.Num() > 0;
}

bool FInteriorEditorMode::IsSelection(SelectionType ST) const
{
	if((ST & ESelection::Node) && IsNodeSelection())
	{
		return true;
	}
	if((ST & ESelection::Face) && IsFaceSelection())
	{
		return true;
	}
	if((ST & ESelection::Portal) && IsPortalSelection())
	{
		return true;
	}
	return false;
}

bool FInteriorEditorMode::IsExclusiveSelection(SelectionType ST) const
{
	return IsSelection(ST) && !IsSelection(ESelection::Any ^ ST);
}

bool FInteriorEditorMode::IsNodeOnlySelection() const
{
	return IsExclusiveSelection(ESelection::Node);
}

bool FInteriorEditorMode::IsFaceOnlySelection() const
{
	return IsExclusiveSelection(ESelection::Face);
}

bool FInteriorEditorMode::IsPortalOnlySelection() const
{
	return IsExclusiveSelection(ESelection::Portal);
}

int FInteriorEditorMode::SelectionCount(SelectionType ST) const
{
	int Count = 0;
	
	if(ST & ESelection::Node)
	{
		Count += SelectedNodes.Num();
	}
	if(ST & ESelection::Face)
	{
		Count += SelectedFaces.Num();
	}
	if(ST & ESelection::Portal)
	{
		Count += SelectedPortals.Num();
	}

	return Count;
}

NodeIdType FInteriorEditorMode::GetFirstSelectedNode() const
{
	if(IsNodeSelection())
	{
		return SelectedNodes[0];
	}
	else if(IsFaceSelection())
	{
		return SelectedFaces[0].NId;
	}
	else
	{
		return NullNode;
	}
}

bool FInteriorEditorMode::IsAlreadySelected(HHitProxy* HP) const
{
	if(HP->IsA(HInteriorNodeFaceHitProxy::StaticGetType()))
	{
		auto FaceHP = static_cast< HInteriorNodeFaceHitProxy* >(HP);
		return SelectedFaces.Contains(FNodeFaceRef{ FaceHP->Node, FaceHP->Face });
	}
	else if(HP->IsA(HInteriorPortalHitProxy::StaticGetType()))
	{
		auto PortalHP = static_cast< HInteriorPortalHitProxy* >(HP);
		return SelectedPortals.Contains(PortalHP->Conn);
	}
	else if(HP->IsA(HInteriorNodeHitProxy::StaticGetType()))
	{
		auto NodeHP = static_cast< HInteriorNodeHitProxy* >(HP);
		return SelectedNodes.Contains(NodeHP->Node);
	}
	else
	{
		return false;
	}
}

void FInteriorEditorMode::Select(HHitProxy* HP)
{
	if(HP->IsA(HInteriorNodeFaceHitProxy::StaticGetType()))
	{
		auto FaceHP = static_cast< HInteriorNodeFaceHitProxy* >(HP);
		SelectedFaces.Add(FNodeFaceRef{ FaceHP->Node, FaceHP->Face });
	}
	else if(HP->IsA(HInteriorPortalHitProxy::StaticGetType()))
	{
		auto PortalHP = static_cast< HInteriorPortalHitProxy* >(HP);
		SelectedPortals.Add(PortalHP->Conn);
	}
	else if(HP->IsA(HInteriorNodeHitProxy::StaticGetType()))
	{
		auto NodeHP = static_cast< HInteriorNodeHitProxy* >(HP);
		SelectedNodes.Add(NodeHP->Node);
	}
}

void FInteriorEditorMode::Deselect(HHitProxy* HP)
{
	if(HP->IsA(HInteriorNodeFaceHitProxy::StaticGetType()))
	{
		auto FaceHP = static_cast< HInteriorNodeFaceHitProxy* >(HP);
		SelectedFaces.Remove(FNodeFaceRef{ FaceHP->Node, FaceHP->Face });
	}
	else if(HP->IsA(HInteriorPortalHitProxy::StaticGetType()))
	{
		auto PortalHP = static_cast< HInteriorPortalHitProxy* >(HP);
		SelectedPortals.Remove(PortalHP->Conn);
	}
	else if(HP->IsA(HInteriorNodeHitProxy::StaticGetType()))
	{
		auto NodeHP = static_cast< HInteriorNodeHitProxy* >(HP);
		SelectedNodes.Remove(NodeHP->Node);
	}
}

FInteriorEditorMode::NodeList FInteriorEditorMode::GetNodeComplement(NodeList const& Nodes) const
{
	auto AllNodes = Graph->GetAllNodes();
	return AllNodes.FilterByPredicate([&Nodes](NodeIdType NId)
	{
		return Nodes.Contains(NId) == false;
	});
}

FInteriorEditorMode::NodeList FInteriorEditorMode::GetNodeComplement(NodeIdType const& Nd) const
{
	NodeList Nodes;
	Nodes.Add(Nd);
	return GetNodeComplement(Nodes);
}

FInteriorEditorMode::FaceList FInteriorEditorMode::GetSelectionComplement(FaceList const& Sel) const
{
	// TODO:
	return{};
}

void FInteriorEditorMode::ClearSelection(SelectionType ST)
{
	bool bChanged = false;
	if(ST & ESelection::Node)
	{
		SelectedNodes.Empty();
		bChanged = true;
	}
	if(ST & ESelection::Face)
	{
		SelectedFaces.Empty();
		bChanged = true;
	}
	if(ST & ESelection::Portal)
	{
		SelectedPortals.Empty();
		bChanged = true;
	}

	if(bChanged)
	{
		Redraw();
	}
}

bool FInteriorEditorMode::ParseMovementKey(FKey Key, EAxisIndex& axis, EAxisDirection& direction)
{
	/*
	TODO: This will need modifying to work with various ortho views
	*/
	auto k = Key.ToString();
	if(k == TEXT("Left"))
	{
		axis = EAxisIndex::Y;
		direction = EAxisDirection::Negative;
	}
	else if(k == TEXT("Right"))
	{
		axis = EAxisIndex::Y;
		direction = EAxisDirection::Positive;
	}
	else if(k == TEXT("Up"))
	{
		axis = EAxisIndex::X;
		direction = EAxisDirection::Positive;
	}
	else if(k == TEXT("Down"))
	{
		axis = EAxisIndex::X;
		direction = EAxisDirection::Negative;
	}
	else if(k == TEXT("PageUp"))
	{
		axis = EAxisIndex::Z;
		direction = EAxisDirection::Positive;
	}
	else if(k == TEXT("PageDown"))
	{
		axis = EAxisIndex::Z;
		direction = EAxisDirection::Negative;
	}
	else
	{
		return false;
	}

	return true;
}

void FInteriorEditorMode::SnapCameraToWorldAxis(FRotator const& CamRot, EAxisIndex& axis, EAxisDirection& direction)
{
	/*
	Since camera is yaw/pitch only, Z axis is considered absolute
	*/
	if(axis == EAxisIndex::Z)
	{
		return;
	}

	/*
	Get the world space axis vector
	*/
	auto cam_local_axis = (axis == EAxisIndex::X) ? FVector::ForwardVector : FVector{ 0.f, 1.f, 0.f };
	cam_local_axis *= DirectionMultiplier(direction);
	auto axis_world = CamRot.RotateVector(cam_local_axis);

	/*
	Clamp it to world axes
	*/
	axis = (FMath::Abs(axis_world.X) > FMath::Abs(axis_world.Y)) ? EAxisIndex::X : EAxisIndex::Y;
	direction = AxisDirectionFromValue(axis_world[axis]);
}

