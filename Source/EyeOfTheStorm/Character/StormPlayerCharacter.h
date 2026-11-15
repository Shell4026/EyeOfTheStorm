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

	void StartDodge(const FVector& Direction, bool bFaceDirection);
	void EndDodge();
	void BeginDodgeInvincibility();
	void EndDodgeInvincibility();

	// 회피 중 / 락온 / 평상시에 맞게 회전 방식을 고른다.
	void UpdateRotationMode();

	void ApplyDefaultMorphTargets();

	UFUNCTION()
	void HandleLockOnTargetChanged(AStormCharacterBase* NewTarget);

	// 입력(카메라 기준)을 월드 수평 방향으로. 입력이 없으면 영벡터.
	FVector GetMoveInputWorldDirection() const;
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
	// 1.5배 (달리는 중 회피 거리)
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "1"))
	float RunDodgeDistanceScale = 1.5f;
	// 1.2배. 달리는 중 회피의 몽타주 재생 속도 배율. 회피 시간은 이 배율로 나눠 짧아지고, 무적 길이(초)는 그대로다.
	// 회피 시간이 DodgeInvincibleEnd보다 짧아지면 무적 끝이 잘린다 (기본값 기준 1.4배까지 안전).
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "1"))
	float RunDodgePlayRateScale = 1.2f;
	// 0.5초
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "0.05"))
	float DodgeDuration = 0.5f;
	// 0.05초
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "0"))
	float DodgeInvincibleStart = 0.05f;
	// 0.35초
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "0"))
	float DodgeInvincibleEnd = 0.35f;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge")
	TObjectPtr<UCurveFloat> DodgeSpeedCurve;
	// 150cm/s (입력 없이 뒤로 빠질 때 끝 속도. 방향 회피는 걷기/달리기 속도로 끝난다)
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge", meta = (ClampMin = "0"))
	float DodgeExitSpeed = 150.f;
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Dodge")
	TObjectPtr<UAnimMontage> DodgeMontage;
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
	uint16 DodgeRootMotionID = 0;

	FTimerHandle DodgeEndTimer;
	FTimerHandle DodgeInvincibleStartTimer;
	FTimerHandle DodgeInvincibleEndTimer;
};