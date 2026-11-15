#include "Character/StormCharacterBase.h"

#include "Combat/StormHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"

AStormCharacterBase::AStormCharacterBase()
{
	HealthComponent = CreateDefaultSubobject<UStormHealthComponent>(TEXT("Health"));
}

float AStormCharacterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (IsDead() || IsInvincible() || DamageAmount <= 0.f)
	{
		return 0.f;
	}

	// Super는 OnTakeAnyDamage 같은 델리게이트를 부르고 피해량을 돌려준다.
	const float Damage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	return HealthComponent->ApplyDamage(Damage, DamageCauser);
}

FVector AStormCharacterBase::GetLockOnPoint() const
{
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	return Capsule->GetComponentLocation() + FVector(0.f, 0.f, Capsule->GetScaledCapsuleHalfHeight() * LockOnPointHeightRatio);
}

void AStormCharacterBase::AddInvincibility()
{
	++InvincibleCount;
}

void AStormCharacterBase::RemoveInvincibility()
{
	ensureMsgf(InvincibleCount > 0, TEXT("%s: RemoveInvincibility without AddInvincibility"), *GetName());
	InvincibleCount = FMath::Max(0, InvincibleCount - 1);
}

bool AStormCharacterBase::IsDead() const
{
	return HealthComponent->IsDead();
}

void AStormCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	HealthComponent->OnDeath.AddDynamic(this, &AStormCharacterBase::HandleDeath);
}

void AStormCharacterBase::HandleDeath(AActor* Killer)
{
	GetCharacterMovement()->DisableMovement();
	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}
}
