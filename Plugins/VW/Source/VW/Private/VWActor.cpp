// Fill out your copyright notice in the Description page of Project Settings.


#include "VWActor.h"
#include "VWSubSystem.h"
#include "VWParameterMapper.h"

#include "Components/BillboardComponent.h"
#include "Engine/World.h"

// Sets default values
AVWActor::AVWActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

#if WITH_EDITORONLY_DATA
	Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (Sprite)
		Sprite->SetupAttachment(RootComponent);
#endif
}

void AVWActor::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	UWorld* const WorldPtr = GetWorld();
	if (UVWSubSystem* SubSystem = WorldPtr ? WorldPtr->GetSubsystem<UVWSubSystem>() : nullptr)
	{
		SubSystem->RegisterWeatherActor(this);
		PushToRenderer();
	}
}

void AVWActor::PostUnregisterAllComponents()
{
	UWorld* const WorldPtr = GetWorld();
	if (UVWSubSystem* SubSystem = WorldPtr ? WorldPtr->GetSubsystem<UVWSubSystem>() : nullptr)
		SubSystem->UnRegisterWeatherActor(this);

	Super::PostUnregisterAllComponents();
}

#if WITH_EDITOR
void AVWActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	bDirty = true;
	PushToRenderer();
}
#endif

void AVWActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (Artist.TimeSpeed > 0.f)
	{
		Artist.Hour = FMath::Fmod(Artist.Hour + Artist.TimeSpeed * DeltaTime, 24.f);
		bDirty = true;
	}

	if (bDirty)
		PushToRenderer();
}
void AVWActor::BeginPlay()
{
	Super::BeginPlay();
}

void AVWActor::PushToRenderer()
{
	UWorld* const WorldPtr = GetWorld();
	UVWSubSystem* const SubSystem = WorldPtr ? WorldPtr->GetSubsystem<UVWSubSystem>() : nullptr;
	if (SubSystem == nullptr)
		return;
	const FVWRenderSettings Settings = VWParameterMapper::ConvertRenderSetting(Artist, Quality);
	SubSystem->PushSettings(Settings);
	SubSystem->PushHelloTint(HelloTint);

	bDirty = false;
}