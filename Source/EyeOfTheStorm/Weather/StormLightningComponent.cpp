#include "Weather/StormLightningComponent.h"

#include "Character/StormCharacterBase.h"
#include "Combat/StormHealthComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Weather/StormLightningStrike.h"

DEFINE_LOG_CATEGORY_STATIC(LogStormLightning, Log, All);

UStormLightningComponent::UStormLightningComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStormLightningComponent::BeginPlay()
{
	Super::BeginPlay();

	ArenaCenter = GetOwner()->GetActorLocation();
	UStormHealthComponent* Health = GetOwner()->FindComponentByClass<UStormHealthComponent>();
	if (!Health)
	{
		UE_LOG(LogStormLightning, Warning, TEXT("%s: 체력 컴포넌트가 없어 낙뢰를 시작하지 않는다"), *GetOwner()->GetName());
		return;
	}
	HealthComponent = Health;
	Health->OnHealthChanged.AddDynamic(this, &UStormLightningComponent::HandleHealthChanged);
	Health->OnDeath.AddDynamic(this, &UStormLightningComponent::HandleOwnerDeath);
	HandleHealthChanged(Health->GetHealth(), Health->GetMaxHealth());
}

void UStormLightningComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopStorm();
	Super::EndPlay(EndPlayReason);
}

void UStormLightningComponent::StartStorm()
{
	if (bActive)
	{
		return;
	}
	bActive = true;
	UE_LOG(LogStormLightning, Log, TEXT("%s: 낙뢰 시작"), *GetOwner()->GetName());

	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.SetTimer(StrikeTimer, this, &UStormLightningComponent::StrikeWave, StrikeInterval, true, FirstStrikeDelay);
	ScheduleAmbientFlash();
}

void UStormLightningComponent::StopStorm()
{
	bActive = false;
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StrikeTimer);
		World->GetTimerManager().ClearTimer(AmbientFlashTimer);
	}
}

void UStormLightningComponent::StrikeWave()
{
	AStormCharacterBase* Player = Cast<AStormCharacterBase>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!Player || Player->IsDead())
	{
		return;
	}

	TArray<FVector> Points;
	FVector Point;
	// 플레이어가 공중(회피 중 낭떠러지 등)이라 발밑에 바닥이 없으면 건너뛴다
	if (FindGroundPoint(Player->GetActorLocation(), Point))
	{
		Points.Add(Point);
	}
	for (int32 i = 0; i < RandomStrikeCount; ++i)
	{
		if (FindRandomGroundPoint(Points, Point))
		{
			Points.Add(Point);
		}
	}
	for (const FVector& GroundPoint : Points)
	{
		SpawnStrike(GroundPoint, true, Player, StrikeFlashIntensity, StrikeFlashRadius, 300.f);
	}
}

void UStormLightningComponent::AmbientFlash()
{
	if (!bActive)
	{
		return;
	}
	SpawnStrike(ArenaCenter, false, nullptr, AmbientFlashIntensity, AmbientFlashRadius, AmbientFlashHeight);
	ScheduleAmbientFlash();
}

void UStormLightningComponent::ScheduleAmbientFlash()
{
	if (AmbientFlashIntensity <= 0.f)
	{
		return;
	}
	const float Delay = FMath::FRandRange(AmbientFlashInterval.X, FMath::Max(AmbientFlashInterval.X, AmbientFlashInterval.Y));
	GetWorld()->GetTimerManager().SetTimer(AmbientFlashTimer, this, &UStormLightningComponent::AmbientFlash, Delay, false);
}

void UStormLightningComponent::SpawnStrike(const FVector& GroundPoint, bool bWarning, AActor* Target, float FlashIntensity, float FlashRadius, float FlashHeight)
{
	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStormLightningStrike* Strike = GetWorld()->SpawnActor<AStormLightningStrike>(GroundPoint, FRotator::ZeroRotator, Params);
	if (!Strike)
	{
		return;
	}

	FStormStrikeSettings Settings;
	Settings.WarningTime = bWarning ? WarningTime : 0.f;
	Settings.Radius = StrikeRadius;
	Settings.Damage = bWarning ? Damage : 0.f;
	Settings.TelegraphMaterial = TelegraphMaterial;
	Settings.Target = Target;
	Settings.Causer = GetOwner();
	Settings.FlashIntensity = FlashIntensity;
	Settings.FlashRadius = FlashRadius;
	Settings.FlashColor = FlashColor;
	Settings.FlashHeight = FlashHeight;
	Strike->Start(Settings);
}

void UStormLightningComponent::HandleHealthChanged(float NewHealth, float MaxHealth)
{
	const float Ratio = MaxHealth > 0.f ? NewHealth / MaxHealth : 0.f;
	if (!bActive && NewHealth > 0.f && Ratio <= GetActivateHealthRatio())
	{
		StartStorm();
	}
}

void UStormLightningComponent::HandleOwnerDeath(AActor* Killer)
{
	StopStorm();
}

bool UStormLightningComponent::FindGroundPoint(const FVector& Near, FVector& OutPoint) const
{
	const FVector Start(Near.X, Near.Y, ArenaCenter.Z + ArenaHeightRange);
	const FVector End(Near.X, Near.Y, ArenaCenter.Z - ArenaHeightRange);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(StormLightning), false, GetOwner());
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		return false;
	}
	OutPoint = Hit.ImpactPoint;
	return true;
}

bool UStormLightningComponent::FindRandomGroundPoint(const TArray<FVector>& Avoid, FVector& OutPoint) const
{
	constexpr int32 MaxTries = 12;
	for (int32 Try = 0; Try < MaxTries; ++Try)
	{
		// 원 안에 고르게 뽑으려면 반지름에 제곱근을 씌운다
		const float Radius = ArenaRadius * FMath::Sqrt(FMath::FRand());
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const FVector Candidate = ArenaCenter + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Radius;

		const bool bTooClose = Avoid.ContainsByPredicate([this, &Candidate](const FVector& Other)
		{
			return FVector::Dist2D(Other, Candidate) < MinStrikeSpacing;
		});
		if (!bTooClose && FindGroundPoint(Candidate, OutPoint))
		{
			return true;
		}
	}
	return false;
}

float UStormLightningComponent::GetActivateHealthRatio() const
{
	if (ActivateHealthRatio >= 0.f)
	{
		return ActivateHealthRatio;
	}
	const UStormHealthComponent* Health = HealthComponent.Get();
	if (!Health || Health->GetSortedThresholds().Num() == 0)
	{
		return 0.f;
	}
	return Health->GetSortedThresholds().Last();
}
