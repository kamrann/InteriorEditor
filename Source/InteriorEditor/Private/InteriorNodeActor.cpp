// Fill out your copyright notice in the Description page of Project Settings.

#include "InteriorEditorPrivatePCH.h"
#include "InteriorNodeActor.h"
//#include "NodeRenderingComponent.h"


AInteriorNodeActor::AInteriorNodeActor(FObjectInitializer const& OI):
Super(OI)
, Extent{ 100.f, 100.f, 100.f }
{
//	RootComponent = OI.CreateEditorOnlyDefaultSubobject< UNodeRenderingComponent >(this, TEXT("RenderComp"));
}



