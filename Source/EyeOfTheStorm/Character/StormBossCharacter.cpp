#include "Character/StormBossCharacter.h"

#include "AI/StormBossAIController.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "BrainComponent.h"
#include "Combat/StormHealthComponent.h"
#include "Combat/StormMeleeComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogStormBoss, Log, All);

AStormBossCharacter::AStormBossCharacter()
{
	AIControllerClass = AStormBossAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 몸 방향은 AI 컨트롤러의 포커스(대상)를 따라 돈다. 공격 중에는 UpdateRotationMode가 끈다.
	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = false;
	Movement->bUseControllerDesiredRotation = true;
	Movement->RotationRate = FRotator(0.f, 240.f, 0.f);
	Movement->MaxWalkSpeed = 350.f;

	Melee = CreateDefaultSubobject<UStormMeleeComponent>(TEXT("Melee"));

	HealthComponent->SetThresholds({ 0.66f, 0.33f });
	AttackLungeStopDistance = 150.f;

	Patterns.SetNum(3);
	Patterns[0].Name = TEXT("Combo2");
	Patterns[0].MinDistance = 0.f;
	Patterns[0].MaxDistance = 300.f;
	Patterns[0].Cooldown = 2.5f;
	Patterns[0].Weight = 2.f;
	Patterns[1].Name = TEXT("Dash");
	Patterns[1].MinDistance = 600.f;
	Patterns[1].MaxDistance = 1500.f;
	Patterns[1].Cooldown = 6.f;
	Patterns[2].Name = TEXT("Slam");
	Patterns[2].MinDistance = 0.f;
	Patterns[2].MaxDistance = 400.f;
	Patterns[2].Cooldown = 8.f;

	Phases.SetNum(3);
	Phases[1].PlayRate = 1.15f;
	Phases[1].CooldownScale = 0.8f;
	Phases[2].PlayRate = 1.3f;
	Phases[2].CooldownScale = 0.6f;
}

bool AStormBossCharacter::StartRandomPattern()
{
	const UWorld* World = GetWorld();
	if (!World || IsDead() || IsAttacking() || World->GetTimeSeconds() < NextAttackTime)
	{
		return false;
	}

	const float Now = World->GetTimeSeconds();
	const float Distance = GetDistanceToTarget();
	float TotalWeight = 0.f;
	for (int32 i = 0; i < Patterns.Num(); ++i)
	{
		if (IsPatternAvailable(i, Distance, Now))
		{
			TotalWeight += Patterns[i].Weight;
		}
	}
	if (TotalWeight <= 0.f)
	{
		return false;
	}

	float Pick = FMath::FRandRange(0.f, TotalWeight);
	for (int32 i = 0; i < Patterns.Num(); ++i)
	{
		if (!IsPatternAvailable(i, Distance, Now))
		{
			continue;
		}
		Pick -= Patterns[i].Weight;
		if (Pick <= 0.f)
		{
			return StartPattern(i);
		}
	}
	return false;
}

bool AStormBossCharacter::StartPattern(int32 Index)
{
	if (IsDead() || IsAttacking() || !Patterns.IsValidIndex(Index))
	{
		return false;
	}
	UAnimMontage* Montage = PickPatternMontage(Index);
	if (!Montage)
	{
		return false;
	}

	if (const AActor* Target = CombatTarget.Get())
	{
		const FVector Facing = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		if (!Facing.IsNearlyZero())
		{
			SetActorRotation(Facing.Rotation());
		}
	}

	ActivePatternIndex = Index;
	ActivePatternMontage = Montage;
	Melee->CancelHitWindow();
	Melee->CancelTelegraph();
	EndAttackLunge();
	GetCharacterMovement()->StopMovementImmediately();
	UpdateRotationMode();

	const FStormBossPhase& PhaseSettings = GetPhaseSettings();
	if (PlayAnimMontage(Montage, PhaseSettings.PlayRate) <= 0.f)
	{
		EndPattern(true);
		return false;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		FOnMontageBlendingOutStarted BlendingOut;
		BlendingOut.BindUObject(this, &AStormBossCharacter::HandlePatternMontageBlendingOut);
		AnimInstance->Montage_SetBlendingOutDelegate(BlendingOut, Montage);
	}

	if (PatternReadyTimes.Num() != Patterns.Num())
	{
		PatternReadyTimes.SetNumZeroed(Patterns.Num());
	}
	PatternReadyTimes[Index] = GetWorld()->GetTimeSeconds() + Patterns[Index].Cooldown * PhaseSettings.CooldownScale;

	UE_LOG(LogStormBoss, Log, TEXT("%s: 패턴 %s - %s (페이즈 %d, 거리 %.0f)"), *GetName(), *Patterns[Index].Name.ToString(), *Montage->GetName(), Phase, GetDistanceToTarget());
	return true;
}

void AStormBossCharacter::StopPattern()
{
	if (!IsAttacking())
	{
		return;
	}
	StopAnimMontage(ActivePatternMontage);
	EndPattern(true);
}

void AStormBossCharacter::SetCombatTarget(AActor* NewTarget)
{
	CombatTarget = NewTarget;
}

bool AStormBossCharacter::HasAvailablePattern() const
{
	const UWorld* World = GetWorld();
	if (!World || IsDead() || IsAttacking() || !CombatTarget.IsValid())
	{
		return false;
	}

	const float Now = World->GetTimeSeconds();
	if (Now < NextAttackTime)
	{
		return false;
	}
	const float Distance = GetDistanceToTarget();
	for (int32 i = 0; i < Patterns.Num(); ++i)
	{
		if (IsPatternAvailable(i, Distance, Now))
		{
			return true;
		}
	}
	return false;
}

float AStormBossCharacter::GetDistanceToTarget() const
{
	const AActor* Target = CombatTarget.Get();
	return Target ? FVector::Dist2D(GetActorLocation(), Target->GetActorLocation()) : TNumericLimits<float>::Max();
}

void AStormBossCharacter::BeginPlay()
{
	Super::BeginPlay();

	PatternReadyTimes.SetNumZeroed(Patterns.Num());
	HealthComponent->OnThresholdCrossed.AddDynamic(this, &AStormBossCharacter::HandleThresholdCrossed);
	UpdateRotationMode();
}

void AStormBossCharacter::HandleDeath(AActor* Killer)
{
	StopPattern();
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* Brain = AI->GetBrainComponent())
		{
			Brain->StopLogic(TEXT("Dead"));
		}
		AI->ClearFocus(EAIFocusPriority::Gameplay);
	}
	UpdateRotationMode();
	Super::HandleDeath(Killer);
}

void AStormBossCharacter::EndPattern(bool bInterrupted)
{
	if (!IsAttacking())
	{
		return;
	}

	ActivePatternIndex = INDEX_NONE;
	ActivePatternMontage = nullptr;
	Melee->CancelHitWindow();
	Melee->CancelTelegraph();
	EndAttackLunge();
	NextAttackTime = GetWorld()->GetTimeSeconds() + AttackInterval * GetPhaseSettings().CooldownScale;
	UpdateRotationMode();

	OnAttackEnded.Broadcast(bInterrupted);
}

void AStormBossCharacter::HandlePatternMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	// 다음 패턴으로 넘어가며 밀려난 이전 몽타주는 무시한다.
	if (Montage == ActivePatternMontage)
	{
		EndPattern(bInterrupted);
	}
}

void AStormBossCharacter::HandleThresholdCrossed(float Threshold)
{
	++Phase;
	UE_LOG(LogStormBoss, Log, TEXT("%s: 페이즈 %d (체력 %.0f%% 경계)"), *GetName(), Phase, Threshold * 100.f);
	OnPhaseChanged.Broadcast(Phase);
}

void AStormBossCharacter::UpdateRotationMode()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = false;
	Movement->bUseControllerDesiredRotation = !IsAttacking() && !IsDead();
}

UAnimMontage* AStormBossCharacter::PickPatternMontage(int32 Index) const
{
	TArray<UAnimMontage*, TInlineAllocator<8>> Candidates; // 메모) 혹시 모를 빈칸 때문에 배열로 다시 만듦
	for (UAnimMontage* Montage : Patterns[Index].Montages)
	{
		if (Montage)
		{
			Candidates.Add(Montage);
		}
	}
	return Candidates.Num() > 0 ? Candidates[FMath::RandRange(0, Candidates.Num() - 1)] : nullptr;
}

const FStormBossPhase& AStormBossCharacter::GetPhaseSettings() const
{
	static const FStormBossPhase DefaultPhase;
	if (Phases.Num() == 0)
	{
		return DefaultPhase;
	}
	return Phases[FMath::Clamp(Phase, 0, Phases.Num() - 1)];
}

bool AStormBossCharacter::IsPatternAvailable(int32 Index, float Distance, float Now) const
{
	const FStormBossPattern& Pattern = Patterns[Index];
	if (Pattern.Weight <= 0.f || Distance < Pattern.MinDistance || Distance > Pattern.MaxDistance)
		return false;
	if (!Pattern.Montages.ContainsByPredicate([](const TObjectPtr<UAnimMontage>& Montage) { return Montage != nullptr; }))
		return false;
	return !PatternReadyTimes.IsValidIndex(Index) || Now >= PatternReadyTimes[Index];
}
