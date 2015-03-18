// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "InteriorGraphBaseTypes.h"
#include "InteriorPortalSelectionProxy.generated.h"


UCLASS(Transient)
class UInteriorPortalSelectionProxy: public UObject
{
	GENERATED_BODY()

public:
	UInteriorPortalSelectionProxy();

public:
	UPROPERTY(EditInstanceOnly, Category = "Portal")
	FString Name;

	UPROPERTY(EditInstanceOnly, Category = "Portal")
	FVector Minimum;

	UPROPERTY(EditInstanceOnly, Category = "Portal")
	FVector Maximum;

public:
	bool IsValid() const;
	ConnectionIdType GetPortal() const;
	void SetPortal(ConnectionIdType CId);

	inline bool IsPortal(ConnectionIdType CId) const
	{
		return ConnId == CId;
	}

public:
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

protected:
	ConnectionIdType ConnId;
};



