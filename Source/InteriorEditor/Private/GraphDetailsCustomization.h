// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IDetailCustomization.h"


class FInteriorGraphDetailsCustomization: public IDetailCustomization
{
public:
	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	//

	static TSharedRef< IDetailCustomization > MakeInstance();

protected:
	class FInteriorEditorMode* GetEditorMode() const;

private:
	// Keep a pointer for refreshing layout
	IDetailLayoutBuilder* DetailBuilderPtr;

	class AInteriorGraphActor* Graph;
};


