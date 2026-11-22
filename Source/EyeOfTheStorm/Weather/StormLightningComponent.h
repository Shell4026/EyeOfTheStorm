#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StormLightningComponent.generated.h"

class UMaterialInterface;
class UStormHealthComponent;

UCLASS(ClassGroup = (Storm), meta = (BlueprintSpawnableComponent))
class EYEOFTHESTORM_API UStormLightningComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UStormLightningComponent();
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	void StartStorm();
	void StopStorm();
	// 플레이어 발밑에 하나, 아레나 무작위 위치에 RandomStrikeCount개를 떨어뜨린다
	void StrikeWave();
	// 낙뢰 없이 아레나 위 하늘만 번쩍인다. 다음 플래시를 다시 예약한다
	void AmbientFlash();
	void ScheduleAmbientFlash();
	void SpawnStrike(const FVector& GroundPoint, bool bWarning, AActor* Target, float FlashIntensity, float FlashRadius, float FlashHeight);

	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float MaxHealth);
	UFUNCTION()
	void HandleOwnerDeath(AActor* Killer);

	// Near 바로 아래 바닥. 아레나 높이에서 위아래로 ArenaHeightRange 안에 바닥이 없으면 false
	bool FindGroundPoint(const FVector& Near, FVector& OutPoint) const;
	// 아레나 안의 무작위 바닥 점. Avoid의 점들과 MinStrikeSpacing보다 가까우면 다시 뽑는다
	bool FindRandomGroundPoint(const TArray<FVector>& Avoid, FVector& OutPoint) const;
	// ActivateHealthRatio가 음수면 체력 컴포넌트의 가장 낮은 경계
	float GetActivateHealthRatio() const;
protected:
	// 시작

	// 소유 액터의 체력 비율이 이 값 이하가 되면 낙뢰와 번개를 시작한다. 음수면 체력 컴포넌트의 가장 낮은 경계 (보스는 33%)
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning", meta = (UIMin = "-1", UIMax = "1"))
	float ActivateHealthRatio = -1.f;
	// 시작하고 첫 낙뢰까지의 시간. 날씨가 폭풍으로 넘어갈 틈을 준다
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning", meta = (ClampMin = "0"))
	float FirstStrikeDelay = 3.f;

	// 낙뢰

	UPROPERTY(EditAnywhere, Category = "Storm|Lightning", meta = (ClampMin = "0.5"))
	float StrikeInterval = 4.f;
	// 플레이어 발밑 낙뢰 말고 아레나 무작위 위치에 함께 떨어지는 수
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning", meta = (ClampMin = "0"))
	int32 RandomStrikeCount = 2;
	// 예고 장판이 보이고 낙뢰가 떨어지기까지의 시간
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning", meta = (ClampMin = "0"))
	float WarningTime = 1.2f;
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning", meta = (ClampMin = "0"))
	float StrikeRadius = 250.f;
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning", meta = (ClampMin = "0"))
	float Damage = 20.f;
	// 보스 공격 예고 장판과 같은 머티리얼을 쓴다
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning")
	TObjectPtr<UMaterialInterface> TelegraphMaterial;

	// 아레나

	// 무작위 낙뢰는 소유 액터가 시작한 위치에서 이 거리 안에 떨어진다. 보스를 아레나 가운데에 둔다
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning", meta = (ClampMin = "0"))
	float ArenaRadius = 1800.f;
	// 아레나 바닥을 찾을 때 시작 높이에서 위아래로 볼 거리. 낭떠러지 아래 땅에 떨어지지 않게 한다
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning", meta = (ClampMin = "0"))
	float ArenaHeightRange = 500.f;
	// 한 번에 떨어지는 낙뢰끼리 이 거리보다 가까이 떨어지지 않는다
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning", meta = (ClampMin = "0"))
	float MinStrikeSpacing = 500.f;

	// 번개 플래시

	UPROPERTY(EditAnywhere, Category = "Storm|Lightning")
	FLinearColor FlashColor = FLinearColor(0.75f, 0.82f, 1.f);
	// 낙뢰 지점의 조명 밝기 (칸델라)
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning", meta = (ClampMin = "0"))
	float StrikeFlashIntensity = 5000000.f;
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning", meta = (ClampMin = "0"))
	float StrikeFlashRadius = 2500.f;
	// 아레나 위 하늘 플래시 조명 밝기 (칸델라). 0이면 하늘 플래시를 끈다
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning", meta = (ClampMin = "0"))
	float AmbientFlashIntensity = 20000000.f;
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning", meta = (ClampMin = "0"))
	float AmbientFlashRadius = 8000.f;
	// 아레나 바닥에서 하늘 플래시 조명까지의 높이
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning", meta = (ClampMin = "0"))
	float AmbientFlashHeight = 3000.f;
	// 하늘 플래시 간격은 이 범위에서 무작위
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning", meta = (ClampMin = "0.1"))
	FVector2D AmbientFlashInterval = FVector2D(3.f, 7.f);
private:
	TWeakObjectPtr<UStormHealthComponent> HealthComponent;
	FVector ArenaCenter = FVector::ZeroVector;
	bool bActive = false;

	FTimerHandle StrikeTimer;
	FTimerHandle AmbientFlashTimer;
};
