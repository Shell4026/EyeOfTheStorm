// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VWActor.generated.h"

class UBillboardComponent;

UCLASS(meta = (DisplayName = "Volumetric Weather"))
class EYEOFTHESTORM_API AVWActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AVWActor();

	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
protected:
	virtual void BeginPlay() override;
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", meta = (DisplayName = "시간", ClampMin = "0", clampMax = "24"))
	float Hour = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", meta = (DisplayName = "시간 속도(시간/초)", ClampMin = "0", UIMax = "2"))
	float TimeSpeed = 0.f;
private:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Sprite;
#endif
};
