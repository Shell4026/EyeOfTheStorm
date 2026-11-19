// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "VWRenderSettings.h"

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "VWSubSystem.generated.h"

class FVWViewExtension;
class UTextureRenderTarget2D;
class UTexture;
struct FVWShadowCaptureInfo;

UCLASS()
class UVWSubSystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void RegisterWeatherActor(AActor* Actor);
	void UnRegisterWeatherActor(AActor* Actor);

	void PushHelloTint(const FLinearColor& InColor);
	void PushSettings(const FVWRenderSettings& Settings, bool bStaticLUTDirty);
	void PushShadowCapture(UTextureRenderTarget2D* DepthTarget, const FVWShadowCaptureInfo& Info);
	void PushBlueNoise(UTexture* Texture);
protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
private:
	void LoadCloudNoise();
private:
	TSharedPtr<FVWViewExtension, ESPMode::ThreadSafe> ViewExtension;
	TWeakObjectPtr<AActor> WeatherActorPtr;

	bool bCloudNoiseLoaded = false;
};
