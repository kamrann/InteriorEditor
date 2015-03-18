// Fill out your copyright notice in the Description page of Project Settings.

#include "InteriorEditorPrivatePCH.h"
#include "InteriorPortalSelectionProxy.h"

#include "InteriorEditorMode.h"


UInteriorPortalSelectionProxy::UInteriorPortalSelectionProxy():
	ConnId(NullConnection)
{}

bool UInteriorPortalSelectionProxy::IsValid() const
{
	return ConnId != NullConnection;
}

ConnectionIdType UInteriorPortalSelectionProxy::GetPortal() const
{
	return ConnId;
}

void UInteriorPortalSelectionProxy::SetPortal(ConnectionIdType CId)
{
	ConnId = CId;
}

void UInteriorPortalSelectionProxy::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	static const FName NAME_Name = GET_MEMBER_NAME_CHECKED(UInteriorPortalSelectionProxy, Name);
	static const FName NAME_Minimum = GET_MEMBER_NAME_CHECKED(UInteriorPortalSelectionProxy, Minimum);
	static const FName NAME_Maximum = GET_MEMBER_NAME_CHECKED(UInteriorPortalSelectionProxy, Maximum);

	if(PropertyChangedEvent.Property != nullptr)
	{
		const FName PropName = PropertyChangedEvent.MemberProperty->GetFName();

		auto Mode = (FInteriorEditorMode*)GLevelEditorModeTools().GetActiveMode(FInteriorEditorMode::ModeId);
		check(Mode);
		
		if(PropName == NAME_Name)
		{
			Mode->UpdatePortalFromSelectionProxy(this);
		}
		else if(PropName == NAME_Minimum)
		{
			Mode->UpdatePortalFromSelectionProxy(this);
		}
		else if(PropName == NAME_Maximum)
		{
			Mode->UpdatePortalFromSelectionProxy(this);
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}




