#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StormLockOnComponent.generated.h"

class AStormCharacterBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStormLockOnTargetChangedSignature, AStormCharacterBase*, NewTarget);

UCLASS(ClassGroup = (Storm), meta = (BlueprintSpawnableComponent))
class EYEOFTHESTORM_API UStormLockOnComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UStormLockOnComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Storm|LockOn")
	void ToggleLockOn();

	UFUNCTION(BlueprintCallable, Category = "Storm|LockOn")
	bool TryLockOn();

	UFUNCTION(BlueprintCallable, Category = "Storm|LockOn")
	void ClearLockOn();

	UFUNCTION(BlueprintPure, Category = "Storm|LockOn")
	AStormCharacterBase* GetTarget() const { return Target.Get(); }
	UFUNCTION(BlueprintPure, Category = "Storm|LockOn")
	bool IsLockedOn() const { return Target.IsValid(); }
private:
	void SetTarget(AStormCharacterBase* NewTarget);
	AStormCharacterBase* FindBestTarget() const;
	bool HasLineOfSight(const FVector& From, const AStormCharacterBase* Candidate) const;

	bool GetViewPoint(FVector& OutLocation, FRotator& OutRotation) const;
	AController* GetOwnerController() const;
public:
	UPROPERTY(BlueprintAssignable, Category = "Storm|LockOn")
	FStormLockOnTargetChangedSignature OnTargetChanged;
protected:
	UPROPERTY(EditAnywhere, Category = "Storm|LockOn", meta = (ClampMin = "0"))
	float SearchRadius = 2500.f; // 2500cm
	UPROPERTY(EditAnywhere, Category = "Storm|LockOn", meta = (ClampMin = "0"))
	float BreakDistance = 3500.f; // 3500cm
	UPROPERTY(EditAnywhere, Category = "Storm|LockOn", meta = (ClampMin = "0", ClampMax = "180"))
	float MaxSearchAngle = 60.f; // 60도
	UPROPERTY(EditAnywhere, Category = "Storm|LockOn", meta = (ClampMin = "0"))
	float LoseSightTime = 1.f; // 1초
	UPROPERTY(EditAnywhere, Category = "Storm|LockOn", meta = (ClampMin = "0"))
	float CameraInterpSpeed = 8.f;
	UPROPERTY(EditAnywhere, Category = "Storm|LockOn")
	float PitchOffset = -10.f; // -10도 (대상을 바라 볼 때 더하는 Pitch)
	UPROPERTY(EditAnywhere, Category = "Storm|LockOn")
	float MinPitch = -45.f;
	UPROPERTY(EditAnywhere, Category = "Storm|LockOn")
	float MaxPitch = 20.f;
private:
	TWeakObjectPtr<AStormCharacterBase> Target;
	float TimeWithoutSight = 0.f;
};
