// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "InteriorEditorNodeFace.h"
#include "InteriorNodeSelectionProxy.generated.h"


UCLASS(Transient)
class UInteriorNodeSelectionProxy: public UObject
{
	GENERATED_BODY()

public:
	UInteriorNodeSelectionProxy();

public:
	UPROPERTY(EditInstanceOnly, Category = "Node")
	FString Name;

	UPROPERTY(EditInstanceOnly, Category = "Node")
	FVector Minimum;

	UPROPERTY(EditInstanceOnly, Category = "Node")
	FVector Maximum;

public:
	bool IsValid() const;
	NodeIdType GetNode() const;
	void SetNode(NodeIdType NId);

	inline bool IsNode(NodeIdType NId) const
	{
		return NodeId == NId;
	}

public:
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

protected:
	NodeIdType NodeId;
};



