#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "StormCharacterBase.generated.h"

class UStormHealthComponent;
class UAnimMontage;
class UMaterialInterface;

UCLASS(Abstract)
class EYEOFTHESTORM_API AStormCharacterBase : public ACharacter
{
	GENERATED_BODY()
public:
	AStormCharacterBase();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	virtual FVector GetLockOnPoint() const;
	// 공격 전진이 멈출 기준 대상. 없으면 nullptr
	virtual AActor* GetAttackTarget() const { return nullptr; }
	UFUNCTION(BlueprintPure, Category = "Storm")
	virtual bool IsAttacking() const { return false; }

	// 공격 몽타주의 Storm Attack Lunge 노티파이가 부른다. Duration(초) 동안 바라보는 방향으로 Distance(cm)만큼 나간다.
	// 공격 중이 아니면 무시한다. bStopAtTarget이면 GetAttackTarget() 앞에서 멈추고,
	// bPassThroughPawns이면 끝날 때까지 캡슐이 다른 폰을 통과한다.
	void BeginAttackLunge(float Distance, float Duration, bool bStopAtTarget = true, bool bPassThroughPawns = false);
	// 전진 힘을 없애고 폰 충돌을 되돌린다. 노티파이 끝, 패턴 중단, 사망 때 부른다.
	void EndAttackLunge();
	void AddInvincibility();
	void RemoveInvincibility();

	UFUNCTION(BlueprintPure, Category = "Storm")
	UStormHealthComponent* GetHealthComponent() const { return HealthComponent; }
	UFUNCTION(BlueprintPure, Category = "Storm")
	bool IsDead() const;
	UFUNCTION(BlueprintPure, Category = "Storm")
	bool IsInvincible() const { return InvincibleCount > 0; }
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	virtual void HandleDeath(AActor* Killer);
private:
	// 사망 몽타주가 진행되는 만큼 메시를 투명하게 하고, 끝나면 숨긴다. Tick에서 부른다
	void UpdateDeathFade(float DeltaSeconds);
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Storm")
	TObjectPtr<UStormHealthComponent> HealthComponent;

	// 끝 포즈로 멈춘다. 몽타주 에셋의 자동 블렌드 아웃 설정과 상관없다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Storm|Animation")
	TObjectPtr<UAnimMontage> DeathMontage;
	// 사망 몽타주가 진행되는 동안 재질 파라미터 DeathFadeParameter를 1에서 0으로 내리고, 끝나면 액터를 숨긴다.
	// 재질이 이 파라미터로 Opacity Mask를 받아야 사라진다 (MF_DeathDissolve)
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Animation")
	bool bFadeOutOnDeath = false;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Animation", meta = (EditCondition = "bFadeOutOnDeath"))
	FName DeathFadeParameter = TEXT("DeathFade");
	// 죽는 순간 메시의 이 슬롯 재질을 바꿔 끼운다. 전투 중에는 원래 재질(Opaque)을 쓰고 사라질 때만 Masked 재질을 쓰기 위해서다
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Animation", meta = (EditCondition = "bFadeOutOnDeath"))
	TMap<FName, TObjectPtr<UMaterialInterface>> DeathMaterials;

	UPROPERTY(EditDefaultsOnly, Category = "Storm|LockOn", meta = (ClampMin = "-1", ClampMax = "1"))
	float LockOnPointHeightRatio = 0.4f;

	// 공격 전진 중 GetAttackTarget()의 중심에서 이 거리 앞에서 멈춘다.
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Attack", meta = (ClampMin = "0"))
	float AttackLungeStopDistance = 110.f;
private:
	int32 InvincibleCount = 0;
	// 공격 전진 힘(루트 모션 소스) ID. 0이면 없음
	uint16 AttackLungeID = 0;
	// 관통 전진 중이면 끝날 때 폰 충돌을 이 값으로 되돌린다
	bool bAttackLungePassingPawns = false;
	TEnumAsByte<ECollisionResponse> PawnResponseBeforeLunge = ECR_Block;
	bool bDeathFading = false;
	float DeathFadeElapsed = 0.f;
	float DeathFadeDuration = 0.f;
};
