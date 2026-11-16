#include "Combat/StormLockOnComponent.h"

#include "Character/StormCharacterBase.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

UStormLockOnComponent::UStormLockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false; // 락온 중에만 돈다.
}

void UStormLockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AStormCharacterBase* Current = Target.Get();
	AController* Controller = GetOwnerController();
	if (!Current || Current->IsDead() || !Controller)
	{
		ClearLockOn();
		return;
	}

	if (FVector::DistSquared(GetOwner()->GetActorLocation(), Current->GetActorLocation()) > FMath::Square(BreakDistance))
	{
		ClearLockOn();
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	GetViewPoint(ViewLocation, ViewRotation);

	TimeWithoutSight = HasLineOfSight(ViewLocation, Current) ? 0.f : TimeWithoutSight + DeltaTime;
	if (TimeWithoutSight > LoseSightTime)
	{
		ClearLockOn();
		return;
	}

	const FRotator CurrentRotation = Controller->GetControlRotation();
	// 카메라 위치가 아니라 캐릭터 위치에서 대상을 보는 방향을 쓴다.
	// 카메라 위치 기준이면 카메라가 돌면서 기준점도 움직여서 흔들린다.
	const FVector From = GetOwner()->GetActorLocation();
	FRotator Desired = (Current->GetLockOnPoint() - From).Rotation();
	Desired.Pitch = CurrentRotation.Pitch;//FMath::Clamp(Desired.Pitch + PitchOffset, MinPitch, MaxPitch);
	Desired.Roll = 0.f;

	
	Controller->SetControlRotation(FMath::RInterpTo(CurrentRotation, Desired, DeltaTime, CameraInterpSpeed));
}

void UStormLockOnComponent::ToggleLockOn()
{
	if (IsLockedOn())
	{
		ClearLockOn();
	}
	else
	{
		TryLockOn();
	}
}

bool UStormLockOnComponent::TryLockOn()
{
	AStormCharacterBase* Best = FindBestTarget();
	SetTarget(Best);
	return Best != nullptr;
}

void UStormLockOnComponent::ClearLockOn()
{
	SetTarget(nullptr);
}

void UStormLockOnComponent::SetTarget(AStormCharacterBase* NewTarget)
{
	if (Target.Get() == NewTarget)
	{
		return;
	}
	Target = NewTarget;
	TimeWithoutSight = 0.f;
	SetComponentTickEnabled(NewTarget != nullptr);
	OnTargetChanged.Broadcast(NewTarget);
}

AStormCharacterBase* UStormLockOnComponent::FindBestTarget() const
{
	FVector ViewLocation;
	FRotator ViewRotation;
	if (!GetViewPoint(ViewLocation, ViewRotation))
	{
		return nullptr;
	}

	const FVector ViewForward = ViewRotation.Vector();
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(MaxSearchAngle));
	const FVector OwnerLocation = GetOwner()->GetActorLocation();

	// 보스전이라 후보가 몇 개 안 된다. 월드의 캐릭터를 전부 훑는다. 나중에 수정 예정
	AStormCharacterBase* Best = nullptr;
	float BestDot = MinDot;
	for (TActorIterator<AStormCharacterBase> It(GetWorld()); It; ++It)
	{
		AStormCharacterBase* Candidate = *It;
		if (Candidate == GetOwner() || Candidate->IsDead())
		{
			continue;
		}
		if (FVector::DistSquared(OwnerLocation, Candidate->GetActorLocation()) > FMath::Square(SearchRadius))
		{
			continue;
		}

		const FVector ToCandidate = (Candidate->GetLockOnPoint() - ViewLocation).GetSafeNormal();
		const float Dot = FVector::DotProduct(ViewForward, ToCandidate);
		if (Dot < BestDot || !HasLineOfSight(ViewLocation, Candidate))
		{
			continue;
		}
		BestDot = Dot;
		Best = Candidate;
	}
	return Best;
}

bool UStormLockOnComponent::HasLineOfSight(const FVector& From, const AStormCharacterBase* Candidate) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(StormLockOnSight), false, GetOwner());
	Params.AddIgnoredActor(Candidate);
	return !GetWorld()->LineTraceTestByChannel(From, Candidate->GetLockOnPoint(), ECollisionChannel::ECC_Visibility, Params);
}

bool UStormLockOnComponent::GetViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	const AController* Controller = GetOwnerController();
	if (!Controller)
	{
		return false;
	}
	// 플레이어 컨트롤러면 카메라 위치, 아니면 폰 눈 위치.
	Controller->GetPlayerViewPoint(OutLocation, OutRotation);
	return true;
}

AController* UStormLockOnComponent::GetOwnerController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn ? OwnerPawn->GetController() : nullptr;
}
