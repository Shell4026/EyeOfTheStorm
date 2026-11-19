#include "UI/StormHealthBarWidget.h"

#include "Combat/StormHealthComponent.h"
#include "Components/ProgressBar.h"

void UStormHealthBarWidget::SetHealthComponent(UStormHealthComponent* InHealth)
{
	if (UStormHealthComponent* Old = Health.Get())
	{
		Old->OnHealthChanged.RemoveDynamic(this, &UStormHealthBarWidget::HandleHealthChanged);
	}
	Health = InHealth;
	if (!InHealth)
	{
		return;
	}

	InHealth->OnHealthChanged.AddUniqueDynamic(this, &UStormHealthBarWidget::HandleHealthChanged);
	// 새 대상은 잔상 없이 바로 맞춘다.
	HealthRatio = InHealth->GetHealthRatio();
	TrailRatio = HealthRatio;
	TrailWaitRemaining = 0.f;
	ApplyRatio(HealthRatio);
}

void UStormHealthBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (TrailRatio <= HealthRatio)
	{
		return;
	}
	if (TrailWaitRemaining > 0.f)
	{
		TrailWaitRemaining -= InDeltaTime;
		return;
	}
	TrailRatio = FMath::Max(HealthRatio, TrailRatio - TrailSpeed * InDeltaTime);
	if (TrailBar)
	{
		TrailBar->SetPercent(TrailRatio);
	}
}

void UStormHealthBarWidget::NativeDestruct()
{
	SetHealthComponent(nullptr);
	Super::NativeDestruct();
}

void UStormHealthBarWidget::HandleHealthChanged(float NewHealth, float MaxHealth)
{
	const float NewRatio = MaxHealth > 0.f ? FMath::Clamp(NewHealth / MaxHealth, 0.f, 1.f) : 0.f;
	if (NewRatio < HealthRatio)
	{
		TrailWaitRemaining = TrailDelay;
	}
	else
	{
		// 회복이나 재시작은 잔상 없이 올린다.
		TrailRatio = NewRatio;
	}
	HealthRatio = NewRatio;
	ApplyRatio(NewRatio);
}

void UStormHealthBarWidget::ApplyRatio(float Ratio)
{
	if (HealthBar)
	{
		HealthBar->SetPercent(Ratio);
	}
	if (TrailBar)
	{
		TrailBar->SetPercent(TrailRatio);
	}
}
