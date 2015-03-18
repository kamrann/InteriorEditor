// Fill out your copyright notice in the Description page of Project Settings.

#include "InteriorEditorPrivatePCH.h"
#include "InteriorFaceSelectionProxy.h"

#include "InteriorEditorMode.h"


UInteriorFaceSelectionProxy::UInteriorFaceSelectionProxy():
	Face()
{}

bool UInteriorFaceSelectionProxy::IsValid() const
{
	return (bool)Face;
}

FNodeFaceRef UInteriorFaceSelectionProxy::GetFace() const
{
	return Face;
}

void UInteriorFaceSelectionProxy::SetFace(FNodeFaceRef F)
{
	Face = F;
}

void UInteriorFaceSelectionProxy::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	static const FName NAME_AxisValue = GET_MEMBER_NAME_CHECKED(UInteriorFaceSelectionProxy, AxisValue);

	if(PropertyChangedEvent.Property != nullptr)
	{
		const FName PropName = PropertyChangedEvent.MemberProperty->GetFName();

		auto Mode = (FInteriorEditorMode*)GLevelEditorModeTools().GetActiveMode(FInteriorEditorMode::ModeId);
		check(Mode);
		
		if(PropName == NAME_AxisValue)
		{
			Mode->UpdateNodeFaceFromSelectionProxy(this);
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}




