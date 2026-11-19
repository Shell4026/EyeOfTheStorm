// Copyright Epic Games, Inc. All Rights Reserved.

#include "VWEditorModeToolkit.h"
#include "VWEditorMode.h"
#include "Engine/Selection.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "VWEditorModeToolkit"

FVWEditorModeToolkit::FVWEditorModeToolkit()
{
}

void FVWEditorModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);
}

void FVWEditorModeToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	PaletteNames.Add(NAME_Default);
}


FName FVWEditorModeToolkit::GetToolkitFName() const
{
	return FName("VWEditorMode");
}

FText FVWEditorModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("DisplayName", "VWEditorMode Toolkit");
}

#undef LOCTEXT_NAMESPACE
