// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "InteriorConnectionActor.generated.h"


/*
Actor representing a connection in the interior graph
*/
UCLASS(NotPlaceable)
class AInteriorConnectionActor: public AActor
{
	GENERATED_BODY()

public:
	AInteriorConnectionActor(FObjectInitializer const& OI);

public:
	UPROPERTY()
	class AInteriorNodeActor* Node1;

	UPROPERTY()
	class AInteriorNodeActor* Node2;

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
};


