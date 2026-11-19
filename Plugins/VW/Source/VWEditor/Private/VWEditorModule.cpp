#include "VWEditorModule.h"
#include "VWEditorModeCommands.h"

void FVWEditorModule::StartupModule()
{
	FVWEditorModeCommands::Register();
}

void FVWEditorModule::ShutdownModule()
{
	FVWEditorModeCommands::Unregister();
}

IMPLEMENT_MODULE(FVWEditorModule, VWEditor)
