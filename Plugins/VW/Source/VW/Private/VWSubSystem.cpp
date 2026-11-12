// Fill out your copyright notice in the Description page of Project Settings.


#include "VWSubSystem.h"
#include "VWViewExtension.h"

#include <RenderingThread.h>

void UVWSubSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ViewExtension = FSceneViewExtensions::NewExtension<FVWViewExtension>(GetWorld());
	UE_LOG(LogTemp, Log, TEXT("[VW] 서브 시스템 생성: %s, (PIE = %d)"), *GetWorld()->GetName(), GetWorld()->IsPlayInEditor() ? 1 : 0);
}

void UVWSubSystem::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("[VW] 서브 시스템 제거: %s, (PIE = %d)"), *GetWorld()->GetName(), GetWorld()->IsPlayInEditor() ? 1 : 0);

	ENQUEUE_RENDER_COMMAND(VWReleaseViewExtensin)(
		[Extension = MoveTemp(ViewExtension)](FRHICommandListImmediate&) mutable
		{
			Extension.Reset();
		}
	);

	Super::Deinitialize();
}

void UVWSubSystem::RegisterWeatherActor(AActor* Actor)
{
	WeatherActorPtr = Actor;

	ENQUEUE_RENDER_COMMAND(VWSetActive)(
		[Extension = ViewExtension](FRHICommandListImmediate&)
		{
			if (Extension)
				Extension->SetActive_RenderThread(true);
		}
	);
}

void UVWSubSystem::UnRegisterWeatherActor(AActor* Actor)
{
	if (WeatherActorPtr.Get() != Actor)
		return;
	WeatherActorPtr.Reset();
	ENQUEUE_RENDER_COMMAND(VWSetActive)(
		[Extension = ViewExtension](FRHICommandListImmediate&)
		{
			if (Extension)
				Extension->SetActive_RenderThread(false);
		}
	);
}

void UVWSubSystem::PushHelloTint(const FLinearColor& InColor)
{
	ENQUEUE_RENDER_COMMAND(VWPushHelloTint)(
		[Extension = ViewExtension, InColor](FRHICommandListImmediate&)
		{
			if (Extension)
				Extension->GetRenderer_RenderThread().SetHelloTint_RenderThread(InColor);
		}
	);
}

bool UVWSubSystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Type::Editor || WorldType == EWorldType::Type::PIE || WorldType == EWorldType::Type::Game;
}
