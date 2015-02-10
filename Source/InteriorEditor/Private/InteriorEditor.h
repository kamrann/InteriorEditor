// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IInteriorEditor.h"


/*
Module implementation
*/
class FInteriorEditorModule: public IInteriorEditor
{
public:


public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};


