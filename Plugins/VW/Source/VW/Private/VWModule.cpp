// Copyright Epic Games, Inc. All Rights Reserved.

#include "VWModule.h"
#include "VWEditorModeCommands.h"

#include <Interfaces/IPluginManager.h>
#include <Misc/Paths.h>
#include <ShaderCore.h>

#define LOCTEXT_NAMESPACE "VWModule"

void FVWModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FVWEditorModeCommands::Register();
	UE_LOG(LogTemp, Log, TEXT("[VW] 모듈 시작"));

	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("VW"));
	check(Plugin.IsValid());

	const FString ShaderDir = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/VW"), ShaderDir);
}

void FVWModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	UE_LOG(LogTemp, Log, TEXT("VW모듈 종료"));
	FVWEditorModeCommands::Unregister();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FVWModule, VW)