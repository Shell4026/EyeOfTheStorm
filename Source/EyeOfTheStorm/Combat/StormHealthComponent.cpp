#include "Combat/StormHealthComponent.h"

UStormHealthComponent::UStormHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

float UStormHealthComponent::ApplyDamage(float Amount, AActor* DamageCauser)
{
	if (IsDead() || Amount <= 0.f)
	{
		return 0.f;
	}

	const float OldHealth = Health;
	Health = FMath::Max(0.f, Health - Amount);
	const float Applied = OldHealth - Health;

	OnHealthChanged.Broadcast(Health, MaxHealth);

	const float Ratio = GetHealthRatio();
	while (SortedThresholds.IsValidIndex(NextThresholdIndex) && Ratio <= SortedThresholds[NextThresholdIndex])
	{
		OnThresholdCrossed.Broadcast(SortedThresholds[NextThresholdIndex]);
		++NextThresholdIndex;
	}

	if (IsDead())
	{
		OnDeath.Broadcast(DamageCauser);
	}
	return Applied;
}

void UStormHealthComponent::ResetHealth()
{
	Health = MaxHealth;

	SortedThresholds = Thresholds;
	SortedThresholds.Sort([](float A, float B) { return A > B; });
	NextThresholdIndex = 0;

	OnHealthChanged.Broadcast(Health, MaxHealth);
}

void UStormHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	ResetHealth();
}
