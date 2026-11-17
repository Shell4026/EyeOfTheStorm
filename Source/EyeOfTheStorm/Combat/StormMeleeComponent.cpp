#include "Combat/StormMeleeComponent.h"

#include "Character/StormCharacterBase.h"
#include "Combat/StormHealthComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/DamageType.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogStormMelee, Log, All);

static TAutoConsoleVariable<int32> CVarStormMeleeDebug(TEXT("Storm.Melee.Debug"), 0, TEXT("1: 근접 판정 구를 그린다 (초록: 판정 중, 빨강: 맞음)"), EConsoleVariableFlags::ECVF_Cheat);

UStormMeleeComponent::UStormMeleeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	// 이번 프레임 애니메이션이 끝난 본 위치로 판정한다.
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;
}

void UStormMeleeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	SweepHitWindow();
}

void UStormMeleeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelHitWindow();
	EndHitStop();
	Super::EndPlay(EndPlayReason);
}

void UStormMeleeComponent::BeginHitWindow(const UObject* Source, USkeletalMeshComponent* Mesh, const FStormHitWindowSettings& Settings)
{
	if (!Source || !Mesh)
	{
		return;
	}

	ActiveSource = Source;
	ActiveMesh = Mesh;
	ActiveSettings = Settings;
	HitActors.Reset();

	// 첫 프레임은 지난 위치가 없으니 지금 위치에서 겹침만 본다.
	GetSamplePoints(PrevPoints);
	SweepHitWindow();
	SetComponentTickEnabled(true);
}

void UStormMeleeComponent::EndHitWindow(const UObject* Source)
{
	if (ActiveSource.Get() == Source)
	{
		CancelHitWindow();
	}
}

void UStormMeleeComponent::CancelHitWindow()
{
	ActiveSource.Reset();
	ActiveMesh.Reset();
	PrevPoints.Reset();
	HitActors.Reset();
	SetComponentTickEnabled(false);
}

void UStormMeleeComponent::ApplyRadialHit(const FVector& Center, float Radius, float Damage, float HitStopDuration)
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner || Radius <= 0.f)
	{
		return;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(StormMeleeRadial), false, Owner);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(Radius), Params);

	// 한 액터의 여러 컴포넌트가 겹쳐도 한 번만 맞는다.
	TArray<AActor*, TInlineAllocator<4>> Targets;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (Target && Target != Owner)
		{
			Targets.AddUnique(Target);
		}
	}

	for (AActor* Target : Targets)
	{
		const FVector TargetLocation = Target->GetActorLocation();
		FHitResult Hit(Target, Cast<UPrimitiveComponent>(Target->GetRootComponent()), TargetLocation, (TargetLocation - Center).GetSafeNormal());
		Hit.TraceStart = Center;
		Hit.TraceEnd = TargetLocation;
		ApplyHit(Target, Hit, Damage, HitStopDuration);
	}

	if (CVarStormMeleeDebug.GetValueOnGameThread() != 0)
	{
		DrawDebugSphere(World, Center, Radius, 16, Targets.Num() > 0 ? FColor::Red : FColor::Orange, false, 1.f);
	}
}

void UStormMeleeComponent::SweepHitWindow()
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner || !ActiveSource.IsValid() || !ActiveMesh.IsValid())
	{
		return;
	}

	TArray<FVector> Points;
	GetSamplePoints(Points);
	if (PrevPoints.Num() != Points.Num())
	{
		PrevPoints = Points;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(StormMelee), false, Owner);
	const FCollisionObjectQueryParams ObjectParams(ECC_Pawn);
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(ActiveSettings.Radius);
	const bool bDebug = CVarStormMeleeDebug.GetValueOnGameThread() != 0;

	TArray<FHitResult> Hits;
	for (int32 i = 0; i < Points.Num(); ++i)
	{
		Hits.Reset();
		World->SweepMultiByObjectType(Hits, PrevPoints[i], Points[i], FQuat::Identity, ObjectParams, Sphere, Params);

		bool bHitThisPoint = false;
		for (const FHitResult& Hit : Hits)
		{
			AActor* Target = Hit.GetActor();
			if (!Target || Target == Owner || HitActors.Contains(Target))
			{
				continue;
			}
			HitActors.Add(Target);
			bHitThisPoint = true;
			ApplyHit(Target, Hit, ActiveSettings.Damage, ActiveSettings.HitStopDuration);

			if (!ActiveSource.IsValid())
			{
				return;
			}
		}

		if (bDebug)
		{
			DrawDebugSphere(World, Points[i], ActiveSettings.Radius, 8, bHitThisPoint ? FColor::Red : FColor::Green, false, bHitThisPoint ? 1.f : 0.f);
		}
	}

	PrevPoints = MoveTemp(Points);
}

void UStormMeleeComponent::ApplyHit(AActor* Target, const FHitResult& Hit, float Damage, float HitStopDuration)
{
	AActor* Owner = GetOwner();
	const FVector Direction = (Hit.TraceEnd - Hit.TraceStart).GetSafeNormal();
	const FPointDamageEvent DamageEvent(Damage, Hit, Direction, UDamageType::StaticClass());
	const float Applied = Target->TakeDamage(Damage, DamageEvent, Owner->GetInstigatorController(), Owner);

	// 무적 or 죽은 상태
	if (Applied <= 0.f)
	{
		return;
	}

	const AStormCharacterBase* StormTarget = Cast<AStormCharacterBase>(Target);
	UE_LOG(LogStormMelee, Log, TEXT("%s -> %s: %.1f (남은 체력 %.1f)"),
		*Owner->GetName(), *Target->GetName(), Applied,
		StormTarget ? StormTarget->GetHealthComponent()->GetHealth() : -1.f);

	StartHitStop(Target, HitStopDuration);
	OnHit.Broadcast(Target, Applied, Hit.ImpactPoint);
}

void UStormMeleeComponent::StartHitStop(AActor* Target, float Duration)
{
	if (Duration <= 0.f)
	{
		return;
	}

	// CustomTimeDilation은 액터 틱과 그 컴포넌트(메시 애니메이션, 무브먼트) 틱의 DeltaTime에 곱해진다.
	// 타이머는 월드 시간으로 돌아서 히트스톱 길이는 실제 시간 그대로다.
	for (AActor* Actor : { GetOwner(), Target })
	{
		if (Actor && !HitStopActors.Contains(Actor))
		{
			HitStopActors.Add(Actor);
		}
		if (Actor)
		{
			Actor->CustomTimeDilation = HitStopTimeDilation;
		}
	}

	// 히트스톱 중에 또 맞으면 그때부터 다시 잰다.
	GetWorld()->GetTimerManager().SetTimer(HitStopTimer, this, &UStormMeleeComponent::EndHitStop, Duration, false);
}

void UStormMeleeComponent::EndHitStop()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitStopTimer);
	}
	for (const TWeakObjectPtr<AActor>& Actor : HitStopActors)
	{
		if (Actor.IsValid())
		{
			Actor->CustomTimeDilation = 1.f;
		}
	}
	HitStopActors.Reset();
}

void UStormMeleeComponent::GetSamplePoints(TArray<FVector>& OutPoints) const
{
	OutPoints.Reset();
	const USkeletalMeshComponent* Mesh = ActiveMesh.Get();
	if (!Mesh)
	{
		return;
	}

	const FVector Start = Mesh->GetSocketLocation(ActiveSettings.StartBone);
	const FVector End = Mesh->GetSocketLocation(ActiveSettings.EndBone);
	// 구 사이가 반지름보다 벌어지지 않게 놓는다.
	const float Length = FVector::Distance(Start, End);
	const int32 Count = FMath::Clamp(FMath::CeilToInt(Length / ActiveSettings.Radius) + 1, 2, MaxSamplePoints);
	for (int32 i = 0; i < Count; ++i)
	{
		OutPoints.Add(FMath::Lerp(Start, End, static_cast<float>(i) / (Count - 1)));
	}
}
