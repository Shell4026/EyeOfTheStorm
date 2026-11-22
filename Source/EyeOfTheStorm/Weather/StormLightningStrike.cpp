#include "Weather/StormLightningStrike.h"

#include "Components/DecalComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	// GetFlashAlpha가 0이 되는 시간
	constexpr float FlashDuration = 0.35f;
}

AStormLightningStrike::AStormLightningStrike()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Telegraph = CreateDefaultSubobject<UDecalComponent>(TEXT("Telegraph"));
	Telegraph->SetupAttachment(RootComponent);
	// 데칼은 X축 방향으로 투사하므로 아래를 보게 세운다
	Telegraph->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	Telegraph->SetVisibility(false);

	FlashLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FlashLight"));
	FlashLight->SetupAttachment(RootComponent);
	FlashLight->SetIntensityUnits(ELightUnits::Candelas);
	FlashLight->SetIntensity(0.f);
	// 짧게 켜졌다 꺼지는 조명이라 그림자 비용을 아낀다
	FlashLight->SetCastShadows(false);
	FlashLight->SetVisibility(false);
}

void AStormLightningStrike::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Elapsed += DeltaSeconds;
	if (!bStruck)
	{
		if (Elapsed >= Settings.WarningTime)
		{
			Strike();
		}
		return;
	}

	const float FlashTime = Elapsed - Settings.WarningTime;
	FlashLight->SetIntensity(Settings.FlashIntensity * GetFlashAlpha(FlashTime));
	if (FlashTime >= FlashDuration)
	{
		Destroy();
	}
}

void AStormLightningStrike::Start(const FStormStrikeSettings& InSettings)
{
	Settings = InSettings;
	Elapsed = 0.f;
	bStruck = false;

	FlashLight->SetRelativeLocation(FVector(0.f, 0.f, Settings.FlashHeight));
	FlashLight->SetAttenuationRadius(Settings.FlashRadius);
	FlashLight->SetLightColor(Settings.FlashColor);

	if (Settings.TelegraphMaterial && Settings.WarningTime > 0.f)
	{
		Telegraph->SetDecalMaterial(Settings.TelegraphMaterial);
		// DecalSize는 반 크기다. X가 투사 깊이, Y와 Z가 바닥의 원
		Telegraph->DecalSize = FVector(Settings.TelegraphDepth, Settings.Radius, Settings.Radius);
		Telegraph->SetVisibility(true);
		Telegraph->MarkRenderStateDirty();
	}

	if (Settings.WarningTime <= 0.f)
	{
		Strike();
	}
}

void AStormLightningStrike::Strike()
{
	bStruck = true;
	Telegraph->SetVisibility(false);
	FlashLight->SetVisibility(true);
	FlashLight->SetIntensity(Settings.FlashIntensity);

	AActor* Target = Settings.Target.Get();
	if (!Target || Settings.Damage <= 0.f)
	{
		return;
	}
	const FVector Offset = Target->GetActorLocation() - GetActorLocation();
	if (Offset.Size2D() > Settings.Radius || FMath::Abs(Offset.Z) > Settings.TelegraphDepth)
	{
		return;
	}
	AActor* Causer = Settings.Causer.Get();
	const APawn* CauserPawn = Cast<APawn>(Causer);
	// 회피 무적이면 피한다 (TakeDamage가 막는다)
	UGameplayStatics::ApplyDamage(Target, Settings.Damage, CauserPawn ? CauserPawn->GetController() : nullptr, Causer ? Causer : this, UDamageType::StaticClass());
}

float AStormLightningStrike::GetFlashAlpha(float Time)
{
	if (Time < 0.05f)
	{
		return 1.f;
	}
	if (Time < 0.1f)
	{
		return 0.15f;
	}
	if (Time < FlashDuration)
	{
		return 1.f - (Time - 0.1f) / (FlashDuration - 0.1f);
	}
	return 0.f;
}
