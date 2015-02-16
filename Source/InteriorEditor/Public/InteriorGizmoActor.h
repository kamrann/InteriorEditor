// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "InteriorGizmoActor.generated.h"


/*
Transient actor used for manipulating interior graphs
*/
UCLASS(NotPlaceable, Transient)
class AInteriorGizmoActor: public AActor
{
	GENERATED_BODY()

public:
	AInteriorGizmoActor(FObjectInitializer const& OI);

public:
};


