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

bool UVWSubSystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Type::Editor || WorldType == EWorldType::Type::PIE || WorldType == EWorldType::Type::Game;
}
