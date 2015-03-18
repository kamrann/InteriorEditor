// Fill out your copyright notice in the Description page of Project Settings.

#include "InteriorEditorPrivatePCH.h"
#include "InteriorNodeSelectionProxy.h"

#include "InteriorEditorMode.h"


UInteriorNodeSelectionProxy::UInteriorNodeSelectionProxy():
	NodeId(NullNode)
{}

bool UInteriorNodeSelectionProxy::IsValid() const
{
	return NodeId != NullNode;
}

NodeIdType UInteriorNodeSelectionProxy::GetNode() const
{
	return NodeId;
}

void UInteriorNodeSelectionProxy::SetNode(NodeIdType NId)
{
	NodeId = NId;
}

void UInteriorNodeSelectionProxy::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	static const FName NAME_Name = GET_MEMBER_NAME_CHECKED(UInteriorNodeSelectionProxy, Name);
	static const FName NAME_Minimum = GET_MEMBER_NAME_CHECKED(UInteriorNodeSelectionProxy, Minimum);
	static const FName NAME_Maximum = GET_MEMBER_NAME_CHECKED(UInteriorNodeSelectionProxy, Maximum);

	if(PropertyChangedEvent.Property != nullptr)
	{
		const FName PropName = PropertyChangedEvent.MemberProperty->GetFName();

		auto Mode = (FInteriorEditorMode*)GLevelEditorModeTools().GetActiveMode(FInteriorEditorMode::ModeId);
		check(Mode);
		
		if(PropName == NAME_Name)
		{
			Mode->UpdateNodeFromSelectionProxy(this);
		}
		else if(PropName == NAME_Minimum)
		{
			Mode->SetNodeMinMax(NodeId, Minimum, Maximum);
		}
		else if(PropName == NAME_Maximum)
		{
			Mode->SetNodeMinMax(NodeId, Minimum, Maximum);
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}




