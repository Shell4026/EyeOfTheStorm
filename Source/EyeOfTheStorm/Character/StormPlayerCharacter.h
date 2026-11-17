#pragma once
#include "Character/StormCharacterBase.h"

#include "CoreMinimal.h"
#include "StormPlayerCharacter.generated.h"

class UCameraComponent;
class UCurveFloat;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
class UStormLockOnComponent;
class UStormMeleeComponent;
struct FInputActionValue;

UCLASS()
class EYEOFTHESTORM_API AStormPlayerCharacter : public AStormCharacterBase
{
	GENERATED_BODY()
public:
	AStormPlayerCharacter();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual AActor* GetAttackTarget() const override;
	virtual bool IsAttacking() const override { return bIsAttacking; }
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// 공격 몽타주의 Storm Combo Window 노티파이가 부른다. 열리는 순간 기억해 둔 회피·다음 타를 실행하고,
	// 이때부터 공격이 끝날 때까지 회피로 끊을 수 있다.
	void OpenComboWindow();
	void CloseComboWindow();

	UFUNCTION(BlueprintPure, Category = "Storm|Player")
	UStormLockOnComponent* GetLockOn() const { return LockOn; }
	UFUNCTION(BlueprintPure, Category = "Storm|Player")
	UStormMeleeComponent* GetMelee() const { return Melee; }
	UFUNCTION(BlueprintPure, Category = "Storm|Player")
	bool IsDodging() const { return bIsDodging; }
	UFUNCTION(BlueprintPure, Category = "Storm|Player")
	bool IsLockedOn() const;
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void HandleDeath(AActor* Killer) override;
private:
	void Move(const FInputActionValue& Value);
	void MoveCompleted(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Dodge();
	void ToggleLockOn();
	void Run();
	void RunCompleted();
	void Zoom(const FInputActionValue& Value);

	// bBackward: 몸은 Direction 반대를 보고 뒤로 구른다 (DodgeBackMontage). 아니면 Direction을 보고 앞으로 구른다.
	void StartDodge(const FVector& Direction, bool bBackward);
	void EndDodge();
	// 회피 힘(루트 모션 소스)을 지운다. 몽타주 끝에서 불리고, EndDodge보다 늦다.
	void EndDodgeForce();
	// 기억해 둔 회피 키가 있고 지금 구를 수 있으면 구른다. EndDodge와 EndDodgeForce 끝에서 부른다.
	void TryBufferedDodge();
	void BeginDodgeInvincibility();
	void EndDodgeInvincibility();

	void Attack();
	// ComboMontages[Index]를 재생한다. 락온 중이면 대상을, 아니면 입력 방향을 보고 찬다.
	void StartAttack(int32 Index);
	// 콤보를 처음으로 되돌린다. 몽타주 블렌드 아웃 시작, 회피, 사망에서 부른다.
	void EndAttack();
	void HandleAttackMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

	// 회피 중 / 락온 / 평상시에 맞게 회전 방식을 고른다.
	void UpdateRotationMode();

	void ApplyDefaultMorphTargets();

	UFUNCTION()
	void HandleLockOnTargetChanged(AStormCharacterBase* NewTarget);

	// 입력(카메라 기준)을 월드 수평 방향으로. 입력이 없으면 영벡터.
	FVector GetMoveInputWorldDirection() const;
	// 회피 이동 시간 (재생 속도 1 기준). 몽타주의 DodgeEndNotifyName 노티파이 시간 없으면 1.0f
	float GetDodgeMoveTime(const UAnimMontage* Montage) const;
	bool CanDodge() const;
	bool CanAttack() const;
protected:
	// 컴포넌트

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Storm|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Storm|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Storm|LockOn")
	TObjectPtr<UStormLockOnComponent> LockOn;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Storm|Attack")
	TObjectPtr<UStormMeleeComponent> Melee;

	// 입력

	UPROPERTY(EditDefaultsOnly, Category = "Storm|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Input")
	TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Input")
	TObjectPtr<UInputAction> LookAction;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Input")
	TObjectPtr<UInputAction> DodgeAction;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Input")
	TObjectPtr<UInputAction> LockOnAction;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Input")
	TObjectPtr<UInputAction> RunAction;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Input")
	TObjectPtr<UInputAction> AttackAction;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Input")
	TObjectPtr<UInputAction> ZoomAction;

	// 이동

	// 200cm/s
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Move", meta = (ClampMin = "0"))
	float WalkSpeed = 200.f;
	// 400cm/s
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Move", meta = (ClampMin = "0"))
	float RunSpeed = 400.f;
	// 400cm/s^2
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Move", meta = (ClampMin = "1"))
	float SpeedChangeRate = 400.f;

	// 회피

	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "0"))
	float DodgeDistance = 450.f;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "0"))
	float DodgeBackDistance = 250.f;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "1"))
	float RunDodgeDistanceScale = 1.5f;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "1"))
	float RunDodgePlayRateScale = 1.2f;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge")
	FName DodgeEndNotifyName = TEXT("RollEnd");
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "0"))
	float DodgeInvincibleStart = 0.05f;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "0"))
	float DodgeInvincibleEnd = 0.35f;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge")
	TObjectPtr<UCurveFloat> DodgeSpeedCurve;
	// 입력 없이 구를 때 끝 속도.
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "0"))
	float DodgeExitSpeed = 150.f;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge")
	TObjectPtr<UAnimMontage> DodgeMontage;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge")
	TObjectPtr<UAnimMontage> DodgeBackMontage;
	// 락온 중 입력이 캐릭터 뒤쪽에서 이 각도 안이면 뒤로 구른다.
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "0", ClampMax = "90"))
	float DodgeBackAngle = 45.f;
	// RollEnd 이 시간 전부터 회복이 끝날 때까지 누른 회피 키를 기억했다가 회복이 끝나면 다시 구른다. 0이면 RollEnd 뒤 회복 중에만 받는다.
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "0"))
	float DodgeBufferTime = 0.3f;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "0.1"))
	float DodgeMontagePlayRate = 1.f;

	// 공격

	// 1타, 2타, 3타 순서. 각 몽타주에 Storm Hit Window(판정)와 Storm Combo Window(다음 타 입력)를 둔다.
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Attack")
	TArray<TObjectPtr<UAnimMontage>> ComboMontages;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Attack", meta = (ClampMin = "0.1"))
	float AttackMontagePlayRate = 1.f;

	// 카메라
	
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Camera", meta = (ClampMin = "0"))
	float MinZoomDistance = 150.f;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Camera", meta = (ClampMin = "0"))
	float MaxZoomDistance = 800.f;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Camera", meta = (ClampMin = "0"))
	float ZoomStep = 50.f;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Camera", meta = (ClampMin = "0"))
	float ZoomInterpSpeed = 10.f;
	
	// 외형

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storm|Appearance")
	TMap<FName, float> DefaultMorphTargets;
private:
	FVector2D MoveInput = FVector2D::ZeroVector;

	float TargetZoomDistance = 0.f;

	bool bIsDodging = false;
	bool bDodgeInvincible = false;
	bool bIsRunning = false;
	bool bDodgeBuffered = false;
	uint16 DodgeRootMotionID = 0;
	// 지금 재생 중인 회피 몽타주 (EndDodge에서 이것을 멈춘다). 에셋은 위 UPROPERTY가 잡고 있다.
	TObjectPtr<UAnimMontage> ActiveDodgeMontage;

	bool bIsAttacking = false;
	bool bComboWindowOpen = false;
	// 콤보 구간이 한 번 열린 뒤부터 공격이 끝날 때까지 true. 이 동안은 회피로 공격을 끊을 수 있다.
	bool bAttackCancelable = false;
	bool bAttackBuffered = false;
	// 지금 재생 중인 타의 인덱스 (ComboMontages)
	int32 ComboIndex = 0;
	// 지금 재생 중인 공격 몽타주. 이전 타의 블렌드 아웃 알림과 구별한다.
	TObjectPtr<UAnimMontage> ActiveAttackMontage;

	FTimerHandle DodgeEndTimer;
	FTimerHandle DodgeForceEndTimer;
	FTimerHandle DodgeInvincibleStartTimer;
	FTimerHandle DodgeInvincibleEndTimer;
};