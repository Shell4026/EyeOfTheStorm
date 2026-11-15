#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "StormCharacterBase.generated.h"

class UStormHealthComponent;
class UAnimMontage;

UCLASS(Abstract)
class EYEOFTHESTORM_API AStormCharacterBase : public ACharacter
{
	GENERATED_BODY()
public:
	AStormCharacterBase();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	
	virtual FVector GetLockOnPoint() const;

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

	UFUNCTION()
	virtual void HandleDeath(AActor* Killer);
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Storm")
	TObjectPtr<UStormHealthComponent> HealthComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Storm|Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Storm|LockOn", meta = (ClampMin = "-1", ClampMax = "1"))
	float LockOnPointHeightRatio = 0.4f;
private:
	int32 InvincibleCount = 0;
};
