// Fill out your copyright notice in the Description page of Project Settings.

#include "InteriorEditorPrivatePCH.h"
#include "InteriorConnectionActor.h"
#include "ConnectionRenderingComponent.h"


AInteriorConnectionActor::AInteriorConnectionActor(FObjectInitializer const& OI):
Super(OI)
, Extent{ 0.f, 0.f, 0.f }
{
	RootComponent = OI.CreateEditorOnlyDefaultSubobject< UConnectionRenderingComponent >(this, TEXT("RenderComp"));
}



