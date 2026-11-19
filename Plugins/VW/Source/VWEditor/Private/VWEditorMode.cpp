// Copyright Epic Games, Inc. All Rights Reserved.

#include "VWEditorMode.h"
#include "VWEditorModeToolkit.h"
#include "EdModeInteractiveToolsContext.h"
#include "InteractiveToolManager.h"
#include "VWEditorModeCommands.h"
#include "Modules/ModuleManager.h"


//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
// AddYourTool Step 1 - include the header file for your Tools here
//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
#include "Tools/VWSimpleTool.h"
#include "Tools/VWInteractiveTool.h"

// step 2: register a ToolBuilder in FVWEditorMode::Enter() below


#define LOCTEXT_NAMESPACE "VWEditorMode"

const FEditorModeID UVWEditorMode::EM_VWEditorModeId = TEXT("EM_VWEditorMode");

FString UVWEditorMode::SimpleToolName = TEXT("VW_ActorInfoTool");
FString UVWEditorMode::InteractiveToolName = TEXT("VW_MeasureDistanceTool");


UVWEditorMode::UVWEditorMode()
{
	FModuleManager::Get().LoadModule("EditorStyle");

	// appearance and icon in the editing mode ribbon can be customized here
	Info = FEditorModeInfo(UVWEditorMode::EM_VWEditorModeId,
		LOCTEXT("ModeName", "VW"),
		FSlateIcon(),
		true);
}


UVWEditorMode::~UVWEditorMode()
{
}


void UVWEditorMode::ActorSelectionChangeNotify()
{
}

void UVWEditorMode::Enter()
{
	UEdMode::Enter();

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	// AddYourTool Step 2 - register the ToolBuilders for your Tools here.
	// The string name you pass to the ToolManager is used to select/activate your ToolBuilder later.
	//////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////// 
	const FVWEditorModeCommands& SampleToolCommands = FVWEditorModeCommands::Get();

	RegisterTool(SampleToolCommands.SimpleTool, SimpleToolName, NewObject<UVWSimpleToolBuilder>(this));
	RegisterTool(SampleToolCommands.InteractiveTool, InteractiveToolName, NewObject<UVWInteractiveToolBuilder>(this));

	// active tool type is not relevant here, we just set to default
	GetToolManager()->SelectActiveToolType(EToolSide::Left, SimpleToolName);
}

void UVWEditorMode::CreateToolkit()
{
	Toolkit = MakeShareable(new FVWEditorModeToolkit);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> UVWEditorMode::GetModeCommands() const
{
	return FVWEditorModeCommands::Get().GetCommands();
}

#undef LOCTEXT_NAMESPACE
