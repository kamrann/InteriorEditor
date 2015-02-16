// Fill out your copyright notice in the Description page of Project Settings.

#include "InteriorEditorPrivatePCH.h"
#include "InteriorEditorMode.h"
#include "InteriorGraphActor.h"
#include "InteriorNodeActor.h"
#include "InteriorGizmoActor.h"
#include "InteriorEditorModeSettings.h"
#include "SInteriorEditor.h"
#include "Editor/UnrealEd/Public/Toolkits/ToolkitManager.h"

#include <algorithm>


const FName FInteriorEditorMode::ModeId = TEXT("Interior");
const float FInteriorEditorMode::Epsilon = 0.1f;

FInteriorEditorMode::FInteriorEditorMode()
{
	Graph = nullptr;
	Gizmo = nullptr;

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

	CheckCreateGizmo();
	//GEditor->SelectActor(Gizmo, true, true);

	/*
	Repopulate our list of node actors, since when we were in a different mode, we do not know if
	node actors were created or destroyed.
	*/
	UpdateNodeReferences();

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

	DestroyGizmo();

	FEdMode::Exit();
}

bool FInteriorEditorMode::IsSelectionAllowed(AActor* InActor, bool bInSelection) const
{
	// TODO: Probably want to be more strict depending on sel/desel and existing selection
	return InActor->IsA< AInteriorGizmoActor >() || InActor->IsA< AInteriorNodeActor >();
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

FVector FInteriorEditorMode::GetWidgetLocation() const
{
/*	auto Sel = GetFirstSelectedActorInstance();
	if(Sel && CurrentWidgetAxis != EAxisList::None)
	{
		return Sel->GetActorLocation();
	}
	else
*/	{
		return FEdMode::GetWidgetLocation();
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
		&& Viewport->KeyState(EKeys::LeftAlt)
		&& Key.ToString() == TEXT("X"))
	{
		auto Sel = GetSelectedNodes();
		if(Sel.Num() == 2)
		{
			//auto bAdded = Graph->ToggleConnection(Sel[0], Sel[1]);
			auto bRemoved = Graph->RemoveConnection(Sel[0], Sel[1]);
			if(bRemoved)
			{
				return true;
			}

			// Try to add
			FAxisAlignedPlanarArea Surface;
			if(TestForSharedSurface(Sel[0]->GetBox(), Sel[1]->GetBox(), Epsilon, &Surface))
			{
				Graph->AddConnection(Sel[0], Sel[1], Surface);
			}
			return true;
		}
	}
	else if(Event == EInputEvent::IE_Pressed
		&& Viewport->KeyState(EKeys::LeftAlt)
		&& ParseMovementKey(Key, axis, direction))
	{
		auto sel = GetSelectedNodes();
		auto ref = GetUnselectedNodes();

		SnapCameraToWorldAxis(ViewportClient->GetViewRotation(), axis, direction);
		SnapNodes(sel, ref, axis, direction);
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

FInteriorEditorMode::FAxisValueList FInteriorEditorMode::GetDistinctAxisValues(FNodeList const& nodes, EAxisIndex axis, EAxisExtremity extremity)
{
	FAxisValueList minimums, maximums;

	for(auto const& node : nodes)
	{
		if(TestAny(extremity, EAxisExtremity::AnyMinimum))
		{
			auto value = node->GetMin()[axis];
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
			auto value = node->GetMax()[axis];
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

float FInteriorEditorMode::DetermineNextSnapOffset(FNodeList const& nodes, FNodeList const& ref_nodes, EAxisIndex axis, EAxisDirection direction)
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

	float multiplier = 1.f;
	if(direction == EAxisDirection::Negative)
	{
		std::reverse(&node_values[0], &node_values[0] + node_values.Num());
		std::reverse(&ref_values[0], &ref_values[0] + ref_values.Num());
		multiplier = -1.f;
	}

	// Naive implementation
	float closest = 0.f;
	int32 r_idx = 0;
	for(int32 n_idx = 0; n_idx < node_values.Num(); ++n_idx)
	{
		auto const v1 = node_values[n_idx] * multiplier;
		for(; r_idx < ref_values.Num(); ++r_idx)
		{
			auto const v2 = ref_values[r_idx] * multiplier;
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

void FInteriorEditorMode::TranslateNodes(FNodeList const& nodes, FVector const& offset)
{
	for(auto node : nodes)
	{
		auto current = node->GetActorLocation();
		node->SetActorLocation(current + offset);
		node->Modify();
		node->PostEditMove(true);
	}
}

void FInteriorEditorMode::SnapNodes(FNodeList const& nodes, FNodeList const& ref_nodes, EAxisIndex axis, EAxisDirection direction)
{
	auto offset = DetermineNextSnapOffset(nodes, ref_nodes, axis, direction);
	if(offset != 0.f)
	{
		FVector vec = FVector::ZeroVector;
		vec[axis] = offset * DirectionMultiplier(direction);
		TranslateNodes(nodes, vec);
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

