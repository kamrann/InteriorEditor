// Fill out your copyright notice in the Description page of Project Settings.

#include "InteriorEditorPrivatePCH.h"
#include "GraphDetailsCustomization.h"
#include "InteriorGraphActor.h"
#include "InteriorEditorMode.h"
#include "InteriorNodeSelectionProxy.h"
#include "InteriorFaceSelectionProxy.h"
#include "InteriorPortalSelectionProxy.h"
#include "PropertyEditing.h"

#include <functional>

#define LOCTEXT_NAMESPACE "InteriorEditor"


TSharedRef< IDetailCustomization > FInteriorGraphDetailsCustomization::MakeInstance()
{
	return MakeShareable(new FInteriorGraphDetailsCustomization);
}

FInteriorEditorMode* FInteriorGraphDetailsCustomization::GetEditorMode() const
{
	return (FInteriorEditorMode*)GLevelEditorModeTools().GetActiveMode(FInteriorEditorMode::ModeId);
}

void FInteriorGraphDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray< TWeakObjectPtr< UObject > > CustObjects;
	DetailBuilder.GetObjectsBeingCustomized(CustObjects);
	if(CustObjects.Num() != 1)
	{
		// Only deal with single instance
		return;
	}

	Graph = Cast< AInteriorGraphActor >(CustObjects[0].Get());
	if(Graph == nullptr)
	{
		// Not expected object type
		return;
	}

	auto Mode = GetEditorMode();
	if(Mode == nullptr)
	{
		// If not in Interior editor mode, no customization for now
		return;
	}

	// TODO: Perhaps selection should be handled by the graph actor rather than the mode??
	UClass* SelProxyClass = nullptr;
	TArray< UObject* > SelProxies;
	if(Mode->IsNodeOnlySelection())
	{
		SelProxyClass = UInteriorNodeSelectionProxy::StaticClass();
		for(auto Elem : Mode->NodeSelectionProxies)
		{
			SelProxies.Add(Elem);
		}
	}
	else if(Mode->IsFaceOnlySelection())
	{
		SelProxyClass = UInteriorFaceSelectionProxy::StaticClass();
		for(auto Elem : Mode->FaceSelectionProxies)
		{
			SelProxies.Add(Elem);
		}
	}
	else if(Mode->IsPortalOnlySelection())
	{
		SelProxyClass = UInteriorPortalSelectionProxy::StaticClass();
		for(auto Elem : Mode->PortalSelectionProxies)
		{
			SelProxies.Add(Elem);
		}
	}

	if(SelProxyClass)
	{
		if(SelProxies.Num() == 1)
		{
			for(TFieldIterator< UProperty > PropIt(SelProxyClass); PropIt; ++PropIt)
			{
				UProperty* Prop = *PropIt;

				//		if(!TestProperty->HasMetaData("HideInAwareness"))
				{
					FName CategoryName(*Prop->GetMetaData(TEXT("Category")));
					auto& Category = DetailBuilder.EditCategory(CategoryName);

					//Group.AddPropertyRow();

					if(IDetailPropertyRow* ExternalRow = Category.AddExternalProperty(SelProxies, Prop->GetFName()))
					{
						//ExternalRow->Visibility(InternalInstanceVis);

						//TSharedRef<IPropertyHandle> ImportSkeletalProp = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFbxImportUI, bImportAsSkeletal));
						//ImportSkeletalProp->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FFbxImportUIDetails::MeshImportModeChanged));
					}
				}
			}
		}
	}

	if(Mode->IsNodeOnlySelection())
	{
		if(SelProxies.Num() == 1)
		{
			auto& NodeCategory = DetailBuilder.EditCategory(TEXT("Node"));
			auto const& ND = Graph->GetNodeData(Mode->NodeSelectionProxies[0]->GetNode());
			FNumberFormattingOptions Fmt;
			Fmt.SetMaximumFractionalDigits(0);
			NodeCategory.AddCustomRow(FText::FromString(TEXT("Node")))
				.NameContent()
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Dimensions")))
				]
			.ValueContent()
				[
					SNew(STextBlock).Text(FText::Format(FText::FromString(TEXT("{0} x {1} x {2}")),
					FText::AsNumber(ND.Size().X, &Fmt), FText::AsNumber(ND.Size().Y, &Fmt), FText::AsNumber(ND.Size().Z, &Fmt)
					))
				];
		}
		else if(SelProxies.Num() == 2)
		{
			auto NId1 = Mode->NodeSelectionProxies[0]->GetNode();
			auto NId2 = Mode->NodeSelectionProxies[1]->GetNode();
			/*
			TODO: May need to force a refresh for this, or always create but with a visibility delegate??
			*/
			FAxisAlignedPlanarArea Surface;
			if(TestForSharedSurface(
				Graph->GetNodeData(NId1).Box(),
				Graph->GetNodeData(NId2).Box(),
				0.1f,// TODO: Epsilon,
				&Surface))
			{
				auto& OpsCategory = DetailBuilder.EditCategory(TEXT("Operations"));
				auto const& ND = Graph->GetNodeData(Mode->NodeSelectionProxies[0]->GetNode());
				FNumberFormattingOptions Fmt;
				Fmt.SetMaximumFractionalDigits(0);
				OpsCategory.AddCustomRow(FText::FromString(TEXT("Operations")))
					.WholeRowContent()
					[
						SNew(SButton)
						.Text(FText::FromString(TEXT("Create Portal")))
						.OnClicked_Lambda([Mode, NId1, NId2]() -> FReply
						{
							Mode->TryAddPortal(NId1, NId2);
							return FReply::Handled();
						})
					];
			}
		}
	}

	DetailBuilderPtr = &DetailBuilder;
}
/*
void FAwarenessComponentCustomization::OnObjectSelected()
{
	Component->SelectedLPD = nullptr;
	IPerceptionTrackable* PT = nullptr;

	if(Component->SelectedActor)
	{
		PT = Cast< IPerceptionTrackable >(Component->SelectedActor);
		Component->SelectedLPD = Component->tracked_objects_.Find(PT)->LPD;
	}

	auto SSIt = TActorIterator< ASearchableSpace >(Component->GetWorld());
	if(SSIt)
	{
		auto Actor = *SSIt;	//Component->GetOwner();
		auto RenderComp = Actor->FindComponentByClass< UAwarenessRenderingComponent >();
		if(RenderComp)
		{
			RenderComp->CurrentTrackedObject = PT;
			RenderComp->MarkRenderStateDirty();
		}
	}

	DetailBuilderPtr->ForceRefreshDetails();
}
*/


#undef LOCTEXT_NAMESPACE

