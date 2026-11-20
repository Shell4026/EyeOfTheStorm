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
	ResetState();
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

void UStormHealthComponent::OnRegister()
{
	Super::OnRegister();
	// 컴포넌트끼리 BeginPlay 순서는 정해져 있지 않아서, 체력과 경계를 읽는 컴포넌트가 먼저 시작해도 맞는 값을 보게 미리 채운다
	ResetState();
}

void UStormHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	ResetHealth();
}

void UStormHealthComponent::ResetState()
{
	Health = MaxHealth;
	SortedThresholds = Thresholds;
	SortedThresholds.Sort([](float A, float B) { return A > B; });
	NextThresholdIndex = 0;
}
