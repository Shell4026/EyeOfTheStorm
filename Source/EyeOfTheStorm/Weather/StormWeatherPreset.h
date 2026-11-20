#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VWArtistSetting.h"
#include "StormWeatherPreset.generated.h"

UCLASS(BlueprintType)
class EYEOFTHESTORM_API UStormWeatherPreset : public UDataAsset
{
	GENERATED_BODY()
public:
	// 가시거리를 UE 높이 안개의 FogDensity로 바꾼다. 가시거리가 0 이하면 0(안개 없음)
	float GetFogDensity() const;
public:
	// 위도, 자전축, 월, 시간 속도, 대기 두께는 쓰지 않는다. 장소에 딸린 값이라 레벨의 날씨 액터 값을 그대로 둔다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storm|Weather")
	FVWArtistSetting Artist;
	// TransitionTo로 이 프리셋으로 바꿀 때 걸리는 시간. 0이면 바로 바뀐다. 보스는 체력 비율 트랙을 써서 이 값을 쓰지 않는다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storm|Weather", meta = (ClampMin = "0"))
	float TransitionDuration = 5.f;

	// 높이 안개

	// 안개 액터 높이에서 검은 물체가 배경과 거의 구분되지 않는(대비 2%) 거리. 0이면 안개 없음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storm|Weather|Fog", meta = (ClampMin = "0", Units = "Meters"))
	float FogVisibilityMeters = 0.f;
	// 클수록 위로 갈 때 빨리 옅어진다. UE 높이 안개의 Fog Height Falloff와 같은 값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storm|Weather|Fog", meta = (ClampMin = "0.001"))
	float FogHeightFalloff = 0.2f;
	// 하늘은 VW 합성이 덮어써서 안개가 걸리지 않는다. 지평선 하늘색과 맞춰야 먼 산과 하늘 사이에 경계가 안 보인다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storm|Weather|Fog", meta = (HideAlphaChannel))
	FLinearColor FogColor = FLinearColor(0.45f, 0.5f, 0.55f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storm|Weather|Fog", meta = (ClampMin = "0", ClampMax = "1"))
	float FogMaxOpacity = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storm|Weather|Fog", meta = (ClampMin = "0"))
	float FogStartDistance = 0.f;
};
