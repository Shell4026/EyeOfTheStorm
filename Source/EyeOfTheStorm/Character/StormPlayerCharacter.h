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

	UFUNCTION(BlueprintPure, Category = "Storm|Player")
	UStormLockOnComponent* GetLockOn() const { return LockOn; }
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

	// bBackward: 몸은 Direction 반대를 보고 뒤로 구른다 (DodgeBackMontage). 아니면 Direction을 보고 앞으로 구른다.
	void StartDodge(const FVector& Direction, bool bBackward);
	void EndDodge();
	// 회피 힘(루트 모션 소스)을 지운다. 몽타주 끝에서 불리고, EndDodge보다 늦다.
	void EndDodgeForce();
	// 기억해 둔 회피 키가 있고 지금 구를 수 있으면 구른다. EndDodge와 EndDodgeForce 끝에서 부른다.
	void TryBufferedDodge();
	void BeginDodgeInvincibility();
	void EndDodgeInvincibility();

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
protected:
	// 컴포넌트

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Storm|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Storm|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Storm|LockOn")
	TObjectPtr<UStormLockOnComponent> LockOn;

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

	// 450cm
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "0"))
	float DodgeDistance = 450.f;
	// 250cm (뒤로 구를 때 거리. 입력 없이 회피, 또는 락온 중 뒷방향 회피)
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "0"))
	float DodgeBackDistance = 250.f;
	// 1.5배 (달리는 중 회피 거리. 앞·뒤 모두)
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "1"))
	float RunDodgeDistanceScale = 1.5f;
	// 1.2배. 달리는 중 회피의 몽타주 재생 속도 배율. 회피 시간은 이 배율로 나눠 짧아지고, 무적 길이(초)는 그대로다.
	// 회피 시간이 DodgeInvincibleEnd보다 짧아지면 무적 끝이 잘린다.
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "1"))
	float RunDodgePlayRateScale = 1.2f;
	// RollEnd (회피 몽타주에서 이동이 끝나는 노티파이 이름. 이 시점에 몽타주를 블렌드 아웃한다)
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge")
	FName DodgeEndNotifyName = TEXT("RollEnd");
	// 0.05초
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "0"))
	float DodgeInvincibleStart = 0.05f;
	// 0.35초
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "0"))
	float DodgeInvincibleEnd = 0.35f;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge")
	TObjectPtr<UCurveFloat> DodgeSpeedCurve;
	// 150cm/s (입력 없이 구를 때 끝 속도. 입력을 누른 채 구르면 걷기/달리기 속도로 끝난다)
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "0"))
	float DodgeExitSpeed = 150.f;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge")
	TObjectPtr<UAnimMontage> DodgeMontage;
	// 뒤로 구르기 몽타주 (입력 없이 회피, 또는 락온 중 뒷방향 입력). 비우면 DodgeMontage
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge")
	TObjectPtr<UAnimMontage> DodgeBackMontage;
	// 45도 (락온 중 입력이 캐릭터 뒤쪽에서 이 각도 안이면 뒤로 구른다)
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "0", ClampMax = "90"))
	float DodgeBackAngle = 45.f;
	// 0.3초 (RollEnd 이 시간 전부터 회복이 끝날 때까지 누른 회피 키를 기억했다가 회복이 끝나면 다시 구른다. 0이면 RollEnd 뒤 회복 중에만 받는다)
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "0"))
	float DodgeBufferTime = 0.3f;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "0.1"))
	float DodgeMontagePlayRate = 1.f;

	// 외형

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storm|Appearance")
	TMap<FName, float> DefaultMorphTargets;
private:
	FVector2D MoveInput = FVector2D::ZeroVector;

	bool bIsDodging = false;
	bool bDodgeInvincible = false;
	bool bIsRunning = false;
	bool bDodgeBuffered = false;
	uint16 DodgeRootMotionID = 0;
	// 지금 재생 중인 회피 몽타주 (EndDodge에서 이것을 멈춘다). 에셋은 위 UPROPERTY가 잡고 있다.
	TObjectPtr<UAnimMontage> ActiveDodgeMontage;

	FTimerHandle DodgeEndTimer;
	FTimerHandle DodgeForceEndTimer;
	FTimerHandle DodgeInvincibleStartTimer;
	FTimerHandle DodgeInvincibleEndTimer;
};