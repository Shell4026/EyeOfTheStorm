// Fill out your copyright notice in the Description page of Project Settings.


#include "VWActor.h"

#include <Components/BillboardComponent.h>

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

#if WITH_EDITOR
void AVWActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UE_LOG(LogTemp, Log, TEXT("[VW] %s 변경됨. 시간 = %.2f"), *PropertyChangedEvent.GetPropertyName().ToString(), Hour);
}
#endif

void AVWActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (TimeSpeed > 0.f)
	{
		const int32 PrevHour = FMath::FloorToInt(Hour);
		Hour = FMath::Fmod(Hour + TimeSpeed * DeltaTime, 24.f);
		if (FMath::FloorToInt(Hour) != PrevHour)
			UE_LOG(LogTemp, Log, TEXT("[VW] %d시"), FMath::FloorToInt(Hour));
	}
}
void AVWActor::BeginPlay()
{
	Super::BeginPlay();
}