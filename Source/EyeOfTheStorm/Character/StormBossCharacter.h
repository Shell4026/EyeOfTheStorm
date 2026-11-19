#pragma once
#include "Character/StormCharacterBase.h"

#include "CoreMinimal.h"
#include "StormBossCharacter.generated.h"

class UBehaviorTree;
class UStormMeleeComponent;
class UStormWeatherPreset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStormBossPhaseChangedSignature, int32, NewPhase);
DECLARE_MULTICAST_DELEGATE_OneParam(FStormBossAttackEndedSignature, bool);

USTRUCT(BlueprintType)
struct EYEOFTHESTORM_API FStormBossPattern
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Storm|Boss")
	FName Name;
	// 패턴을 쓸 때마다 이 중 하나를 무작위로 재생한다. 쿨다운, 거리는 패턴 단위로 공유한다
	UPROPERTY(EditAnywhere, Category = "Storm|Boss")
	TArray<TObjectPtr<UAnimMontage>> Montages;
	// 대상과의 수평 거리가 MinDistance ~ MaxDistance일 때만 고른다.
	UPROPERTY(EditAnywhere, Category = "Storm|Boss", meta = (ClampMin = "0"))
	float MinDistance = 0.f;
	UPROPERTY(EditAnywhere, Category = "Storm|Boss", meta = (ClampMin = "0"))
	float MaxDistance = 300.f;
	// 페이즈의 CooldownScale이 곱해진다.
	UPROPERTY(EditAnywhere, Category = "Storm|Boss", meta = (ClampMin = "0"))
	float Cooldown = 3.f;
	// 여러 패턴을 쓸 수 있을 때 고를 비중. 0이면 고르지 않는다
	UPROPERTY(EditAnywhere, Category = "Storm|Boss", meta = (ClampMin = "0"))
	float Weight = 1.f;
};

USTRUCT(BlueprintType)
struct EYEOFTHESTORM_API FStormBossPhase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Storm|Boss", meta = (ClampMin = "0.1"))
	float PlayRate = 1.f;
	UPROPERTY(EditAnywhere, Category = "Storm|Boss", meta = (ClampMin = "0"))
	float CooldownScale = 1.f;
	// 이 페이즈가 되면 바꿀 날씨. 첫 페이즈는 시작할 때 바로 바꾸고, 그다음부터는 프리셋의 전환 시간 동안 바꾼다. 비우면 날씨를 그대로 둔다
	UPROPERTY(EditAnywhere, Category = "Storm|Boss")
	TObjectPtr<UStormWeatherPreset> Weather;
};

UCLASS()
class EYEOFTHESTORM_API AStormBossCharacter : public AStormCharacterBase
{
	GENERATED_BODY()
public:
	AStormBossCharacter();

	virtual AActor* GetAttackTarget() const override { return CombatTarget.Get(); }
	virtual bool IsAttacking() const override { return ActivePatternIndex != INDEX_NONE; }

	// 지금 쓸 수 있는 패턴 중 하나를 Weight 비중대로 골라 시작한다. 고를 게 없으면 false.
	bool StartRandomPattern();
	bool StartPattern(int32 Index);
	void StopPattern();
	void SetCombatTarget(AActor* NewTarget);
	// 공격 간격이 지났고, 거리와 쿨다운이 맞는 패턴이 하나라도 있으면 true.
	bool HasAvailablePattern() const;

	UBehaviorTree* GetBehaviorTree() const { return BehaviorTree; }
	const FText& GetBossName() const { return BossName; }
	UFUNCTION(BlueprintPure, Category = "Storm|Boss")
	UStormMeleeComponent* GetMelee() const { return Melee; }
	UFUNCTION(BlueprintPure, Category = "Storm|Boss")
	int32 GetPhase() const { return Phase; }
	// 대상이 없으면 아주 큰 값
	float GetDistanceToTarget() const;
protected:
	virtual void BeginPlay() override;
	virtual void HandleDeath(AActor* Killer) override;
private:
	void EndPattern(bool bInterrupted);
	void HandlePatternMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);
	UFUNCTION()
	void HandleThresholdCrossed(float Threshold);
	// 공격 중에는 시작할 때 정한 방향을 유지하고, 아니면 AI 컨트롤러의 포커스(대상)를 본다.
	void UpdateRotationMode();
	// 패턴의 Montages 중 비어 있지 않은 것 하나를 무작위로 고른다. 없으면 nullptr
	UAnimMontage* PickPatternMontage(int32 Index) const;
	void ApplyPhaseWeather(bool bInstant);

	const FStormBossPhase& GetPhaseSettings() const;
	bool IsPatternAvailable(int32 Index, float Distance, float Now) const;
public:
	UPROPERTY(BlueprintAssignable, Category = "Storm|Boss")
	FStormBossPhaseChangedSignature OnPhaseChanged;
	// 패턴이 끝나거나 끊기면 부른다. 인자는 끊겼는지. BT 태스크가 기다린다
	FStormBossAttackEndedSignature OnAttackEnded;
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Storm|Boss")
	TObjectPtr<UStormMeleeComponent> Melee;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Boss")
	TObjectPtr<UBehaviorTree> BehaviorTree;

	// 패턴

	UPROPERTY(EditDefaultsOnly, Category = "Storm|Boss")
	TArray<FStormBossPattern> Patterns;
	// 패턴이 끝나고 다음 패턴까지 최소 간격. 페이즈의 CooldownScale이 곱해진다
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Boss", meta = (ClampMin = "0"))
	float AttackInterval = 1.f;

	// 페이즈

	UPROPERTY(EditDefaultsOnly, Category = "Storm|Boss")
	TArray<FStormBossPhase> Phases;

	// UI

	// HUD 보스 체력바 위에 띄우는 이름
	UPROPERTY(EditDefaultsOnly, Category = "Storm|UI")
	FText BossName;
private:
	TWeakObjectPtr<AActor> CombatTarget;
	int32 Phase = 0;

	// 재생 중인 패턴의 인덱스 (Patterns). INDEX_NONE이면 공격 중이 아님
	int32 ActivePatternIndex = INDEX_NONE;
	// 이전 패턴의 늦은 블렌드 아웃 알림을 거른다. 에셋은 Patterns가 잡고 있다
	TObjectPtr<UAnimMontage> ActivePatternMontage;
	// 패턴별로 다시 쓸 수 있게 되는 월드 시간
	TArray<float> PatternReadyTimes;
	float NextAttackTime = 0.f;
};
