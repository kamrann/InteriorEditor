// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "InteriorNodeActor.generated.h"


/*
Actor representing a node in the interior graph
*/
UCLASS() // NotPlaceable
class AInteriorNodeActor: public AActor
{
	GENERATED_BODY()

public:
	AInteriorNodeActor(FObjectInitializer const& OI);

public:
	UPROPERTY(EditAnywhere)
	FVector Extent;

public:
	inline FVector GetMin() const
	{
		return GetActorLocation() - 0.5f * Extent;
	}

	inline FVector GetMax() const
	{
		return GetActorLocation() + 0.5f * Extent;
	}

	inline FBox GetBox() const
	{
		return FBox{ GetMin(), GetMax() };
	}
};


