#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "VWArtistSetting.h"
#include "StormWeatherSubsystem.generated.h"

class AExponentialHeightFog;
class AVWActor;
class UStormWeatherPreset;

// UE 높이 안개에서 프리셋이 바꾸는 값만 모은 것
struct FStormFogState
{
	float Density = 0.f;
	float HeightFalloff = 0.2f;
	FLinearColor Color = FLinearColor::Black;
	float MaxOpacity = 1.f;
	float StartDistance = 0.f;
};

UCLASS()
class EYEOFTHESTORM_API UStormWeatherSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	// 레벨의 날씨 액터와 높이 안개를 지금 상태에서 Preset으로 TransitionDuration 동안 바꾼다. 전환 중에 다시 부르면 그 순간의 날씨에서 이어서 바꾼다
	UFUNCTION(BlueprintCallable, Category = "Storm|Weather")
	void TransitionTo(UStormWeatherPreset* Preset, bool bInstant = false);

	UFUNCTION(BlueprintPure, Category = "Storm|Weather")
	bool IsTransitioning() const { return TargetPreset != nullptr; }
protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
private:
	void ApplyBlend(float Alpha);
	// 장소에 딸린 값(위도, 자전축, 월, 시간 속도, 대기 두께)은 From을 그대로 두고 나머지를 섞는다
	static FVWArtistSetting LerpArtist(const FVWArtistSetting& From, const FVWArtistSetting& To, float Alpha);
	static FStormFogState LerpFog(const FStormFogState& From, const FStormFogState& To, float Alpha);

	AVWActor* GetWeatherActor();
	// 레벨에 없으면 하나 만든다. 안개가 필요 없는 프리셋이면 만들지 않는다
	AExponentialHeightFog* GetFogActor(bool bCreateIfMissing);
	static FStormFogState GetFogState(const AExponentialHeightFog* Fog);
	static FStormFogState GetFogState(const UStormWeatherPreset* Preset);
private:
	UPROPERTY(Transient)
	TObjectPtr<UStormWeatherPreset> TargetPreset;

	TWeakObjectPtr<AVWActor> WeatherActor;
	TWeakObjectPtr<AExponentialHeightFog> FogActor;
	FVWArtistSetting FromArtist;
	FStormFogState FromFog;
	float Elapsed = 0.f;
	float Duration = 0.f;
};
