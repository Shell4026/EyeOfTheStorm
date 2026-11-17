// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "VWArtistSetting.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VWActor.generated.h"

class ADirectionalLight;
class UBillboardComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

UCLASS(meta = (DisplayName = "Volumetric Weather"))
class VW_API AVWActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AVWActor();

	virtual void OnConstruction(const FTransform& Transform) override;
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
	void EnsureShadowTarget();
	void UpdateSunLight(const FVector& SunDirUE, float SunIlluminance, const FLinearColor& SunTransmittance);
	void UpdateShadowCapture(const FVector& SunDirUE);
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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weather|Shadow")
	TObjectPtr<USceneCaptureComponent2D> ShadowCapture;
protected:
	UPROPERTY(EditAnywhere, Category = "Weather|Shadow", meta = (ClampMin = "256", ClampMax = "4096"))
	int32 ShadowMapResolution = 2048;
	UPROPERTY(EditAnywhere, Category = "Weather|Shadow", meta = (ClampMin = "0.1"))
	float ShadowAreaKm = 1.f;
	UPROPERTY(EditAnywhere, Category = "Weather|Shadow", meta = (ClampMin = "0"))
	float ShadowDepthBiasCm = 200.f;
private:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Sprite;
#endif
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> ShadowDepthTarget;

	float LastAtmosphereThickness = -1.f;
	float LastRayleigh = -1.f;
	float LastMie = -1.f;
	FLinearColor LastMieColor = FLinearColor(-1.f, -1.f, -1.f);
	int32 LastTransmittanceSteps = -1;
	int32 LastMultiScatteringSteps = -1;
	FVector LastSunDirUE = FVector::ZeroVector;

	bool bDirty = false;
	bool bHasPushed = false;
	bool bShadowDirty = true;
};
