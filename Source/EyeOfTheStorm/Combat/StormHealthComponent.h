#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StormHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FStormHealthChangedSignature, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStormDeathSignature, AActor*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStormHealthThresholdSignature, float, Threshold);

UCLASS(ClassGroup = (Storm), meta = (BlueprintSpawnableComponent))
class EYEOFTHESTORM_API UStormHealthComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UStormHealthComponent();

	// 피해를 적용하고 실제로 깎인 양을 돌려준다. 이미 죽었거나 Amount <= 0이면 0.
	float ApplyDamage(float Amount, AActor* DamageCauser);

	// 최대 체력으로 되돌리고 경계 통과 기록도 지운다 (재시작용).
	UFUNCTION(BlueprintCallable, Category = "Storm|Health")
	void ResetHealth();
	// 다음 ResetHealth(BeginPlay)부터 적용된다. 생성자에서 기본값을 정할 때 쓴다.
	void SetThresholds(const TArray<float>& InThresholds) { Thresholds = InThresholds; }

	UFUNCTION(BlueprintPure, Category = "Storm|Health")
	float GetHealth() const { return Health; }
	UFUNCTION(BlueprintPure, Category = "Storm|Health")
	float GetMaxHealth() const { return MaxHealth; }
	UFUNCTION(BlueprintPure, Category = "Storm|Health")
	float GetHealthRatio() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }
	UFUNCTION(BlueprintPure, Category = "Storm|Health")
	bool IsDead() const { return Health <= 0.f; }
protected:
	virtual void BeginPlay() override;
public:
	UPROPERTY(BlueprintAssignable, Category = "Storm|Health")
	FStormHealthChangedSignature OnHealthChanged;
	UPROPERTY(BlueprintAssignable, Category = "Storm|Health")
	FStormDeathSignature OnDeath;
	// 체력 비율이 경계값 아래로 처음 내려갈 때 한 번. 한 방에 두 경계를 넘으면 높은 쪽부터 두 번 부른다.
	UPROPERTY(BlueprintAssignable, Category = "Storm|Health")
	FStormHealthThresholdSignature OnThresholdCrossed;
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storm|Health", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;
	// 체력 비율 경계 (0~1). 보스는 0.66, 0.33. 순서 상관없음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storm|Health", meta = (ClampMin = "0", ClampMax = "1"))
	TArray<float> Thresholds;
private:
	UPROPERTY(VisibleInstanceOnly, Category = "Storm|Health")
	float Health = 0.f;
	// Thresholds를 내림차순으로 정렬한 것
	TArray<float> SortedThresholds;
	// 다음에 넘을 경계의 인덱스
	int32 NextThresholdIndex = 0;
};
