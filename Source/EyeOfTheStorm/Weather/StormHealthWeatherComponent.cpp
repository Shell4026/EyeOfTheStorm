#include "Weather/StormHealthWeatherComponent.h"

#include "Combat/StormHealthComponent.h"
#include "Engine/World.h"
#include "Weather/StormWeatherPreset.h"
#include "Weather/StormWeatherSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogStormHealthWeather, Log, All);

UStormHealthWeatherComponent::UStormHealthWeatherComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStormHealthWeatherComponent::BeginPlay()
{
	Super::BeginPlay();

	UStormHealthComponent* Health = GetOwner()->FindComponentByClass<UStormHealthComponent>();
	if (!Health)
	{
		UE_LOG(LogStormHealthWeather, Warning, TEXT("%s: 체력 컴포넌트가 없어 날씨를 바꾸지 않는다"), *GetOwner()->GetName());
		return;
	}
	HealthComponent = Health;
	Health->OnHealthChanged.AddDynamic(this, &UStormHealthWeatherComponent::HandleHealthChanged);
	StartTrack();
}

void UStormHealthWeatherComponent::StartTrack()
{
	UStormWeatherSubsystem* Weather = GetWorld()->GetSubsystem<UStormWeatherSubsystem>();
	const UStormHealthComponent* Health = HealthComponent.Get();
	if (!Weather || !Health || Keys.Num() == 0)
	{
		return;
	}

	const TArray<float>& Thresholds = Health->GetSortedThresholds();
	TArray<FStormWeatherKey> TrackKeys;
	for (int32 i = 0; i < Keys.Num(); ++i)
	{
		if (!Keys[i].Preset)
		{
			continue;
		}
		float Ratio = Keys[i].HealthRatio;
		if (Ratio < 0.f)
		{
			Ratio = i == 0 ? 1.f : (Thresholds.IsValidIndex(i - 1) ? Thresholds[i - 1] : 0.f);
		}
		FStormWeatherKey& Key = TrackKeys.AddDefaulted_GetRef();
		Key.Position = Ratio;
		Key.Preset = Keys[i].Preset;
	}
	Weather->StartTrack(TrackKeys, Health->GetHealthRatio(), FollowRate);
}

void UStormHealthWeatherComponent::HandleHealthChanged(float NewHealth, float MaxHealth)
{
	if (UStormWeatherSubsystem* Weather = GetWorld()->GetSubsystem<UStormWeatherSubsystem>())
	{
		Weather->SetTrackTarget(MaxHealth > 0.f ? NewHealth / MaxHealth : 0.f);
	}
}
