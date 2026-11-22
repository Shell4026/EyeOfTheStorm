#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StormHealthWeatherComponent.generated.h"

class UStormHealthComponent;
class UStormWeatherPreset;

USTRUCT(BlueprintType)
struct EYEOFTHESTORM_API FStormHealthWeatherKey
{
	GENERATED_BODY()

	// 비우면 이 기준점은 빠진다
	UPROPERTY(EditAnywhere, Category = "Storm|Weather")
	TObjectPtr<UStormWeatherPreset> Preset;
	// 날씨가 Preset으로 완성되는 체력 비율. 음수면 체력 컴포넌트의 경계를 쓴다. i번째 기준점은 높은 쪽에서 i번째 경계에서 완성된다 (첫 기준점은 1)
	UPROPERTY(EditAnywhere, Category = "Storm|Weather", meta = (UIMin = "-1", UIMax = "1"))
	float HealthRatio = -1.f;
};

UCLASS(ClassGroup = (Storm), meta = (BlueprintSpawnableComponent))
class EYEOFTHESTORM_API UStormHealthWeatherComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UStormHealthWeatherComponent();
protected:
	virtual void BeginPlay() override;
private:
	// 기준점마다 체력 비율을 정해 날씨 서브시스템의 트랙을 시작한다. 그다음부터 날씨는 체력 비율을 따라간다
	void StartTrack();
	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float MaxHealth);
	UFUNCTION()
	void HandleDeath(AActor* Killer);
	// 트랙을 멈추고 첫 기준점의 날씨로 되돌린다. 걸리는 시간은 그 프리셋의 TransitionDuration
	void RestoreWeather();
protected:
	// 두 기준점의 체력 비율 사이에서는 두 날씨를 섞는다
	UPROPERTY(EditAnywhere, Category = "Storm|Weather")
	TArray<FStormHealthWeatherKey> Keys;
	// 날씨가 체력 비율을 따라가는 속도 (초당 체력 비율). 0.05면 체력 10%가 한 번에 깎여도 날씨는 2초에 걸쳐 따라간다. 0이면 바로 따라간다
	UPROPERTY(EditAnywhere, Category = "Storm|Weather", meta = (ClampMin = "0"))
	float FollowRate = 0.05f;
	// 소유 액터가 죽고 이만큼 뒤에 첫 기준점의 날씨로 되돌리기 시작한다. 음수면 되돌리지 않는다
	UPROPERTY(EditAnywhere, Category = "Storm|Weather")
	float RestoreDelay = 2.f;
private:
	TWeakObjectPtr<UStormHealthComponent> HealthComponent;
	FTimerHandle RestoreTimer;
};
