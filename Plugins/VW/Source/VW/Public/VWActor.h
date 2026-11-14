// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "VWArtistSetting.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VWActor.generated.h"

class ADirectionalLight;
class UBillboardComponent;

UCLASS(meta = (DisplayName = "Volumetric Weather"))
class VW_API AVWActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AVWActor();

	virtual void PostRegisterAllComponents() override;
	virtual void PostUnregisterAllComponents() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
protected:
	virtual void BeginPlay() override;
private:
	void UpdateSunLight(const FVector& SunDirUE, float SunIlluminance, const FLinearColor& SunTransmittance);
	void PushToRenderer();
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	FVWArtistSetting Artist;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Quality")
	FVWQualityOptions Quality;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Debug", meta = (DisplayName = "Hello 색", HideAlphaChannel))
	FLinearColor HelloTint = FLinearColor{ 1.f, 0.3f, 0.3f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Scene")
	TObjectPtr<ADirectionalLight> SunLight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Scene", meta = (ClampMin = "0"))
	float SunLightIntensityScale = UE_PI;
private:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Sprite;
#endif
	bool bDirty = false;
};
