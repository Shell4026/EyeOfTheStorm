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

// 트랙의 기준점. 트랙 위치가 Position이면 날씨가 Preset이 되고, 두 기준점 사이에서는 두 프리셋을 섞는다
USTRUCT()
struct EYEOFTHESTORM_API FStormWeatherKey
{
	GENERATED_BODY()

	UPROPERTY()
	float Position = 1.f;
	UPROPERTY()
	TObjectPtr<UStormWeatherPreset> Preset;
};

UCLASS()
class EYEOFTHESTORM_API UStormWeatherSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	// 레벨의 날씨 액터와 높이 안개를 지금 상태에서 Preset으로 TransitionDuration 동안 바꾼다. 전환 중에 다시 부르면 그 순간의 날씨에서 이어서 바꾼다. 진행 중인 트랙은 멈춘다
	UFUNCTION(BlueprintCallable, Category = "Storm|Weather")
	void TransitionTo(UStormWeatherPreset* Preset, bool bInstant = false);
	// 기준점 사이를 섞는 트랙을 시작하고 Position의 날씨를 바로 적용한다. Position이 기준점 범위 밖이면 가장 가까운 기준점의 날씨. 진행 중인 전환은 멈춘다
	void StartTrack(const TArray<FStormWeatherKey>& Keys, float Position, float FollowRate);
	// 날씨는 이 위치를 향해 FollowRate(초당 위치 변화) 속도로 따라간다. FollowRate가 0이면 바로 간다
	void SetTrackTarget(float Position);

	UFUNCTION(BlueprintPure, Category = "Storm|Weather")
	bool IsTransitioning() const { return TargetPreset != nullptr; }
protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
private:
	void ApplyBlend(float Alpha);
	void ApplyTrack();
	// From과 To를 Alpha로 섞어 적용한다. 장소에 딸린 값은 레벨의 날씨 액터 값을 그대로 둔다
	void ApplyBetween(const UStormWeatherPreset* From, const UStormWeatherPreset* To, float Alpha);
	void ApplyFog(const FStormFogState& State, bool bCreateIfMissing);
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
	// Position 내림차순. 비어 있으면 트랙을 쓰지 않는다
	UPROPERTY(Transient)
	TArray<FStormWeatherKey> TrackKeys;

	TWeakObjectPtr<AVWActor> WeatherActor;
	TWeakObjectPtr<AExponentialHeightFog> FogActor;
	FVWArtistSetting FromArtist;
	FStormFogState FromFog;
	float Elapsed = 0.f;
	float Duration = 0.f;

	// 트랙

	float TrackPosition = 1.f;
	float TrackTarget = 1.f;
	float TrackFollowRate = 0.f;
	// 날씨 값이 바뀌면 플러그인이 LUT를 다시 구우므로, 마지막에 적용한 구간과 비율이 같으면 다시 적용하지 않는다
	int32 AppliedSegment = INDEX_NONE;
	float AppliedAlpha = -1.f;
};
