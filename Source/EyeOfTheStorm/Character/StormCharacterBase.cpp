#include "Character/StormCharacterBase.h"

#include "Combat/StormHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"

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

	const float Damage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	return HealthComponent->ApplyDamage(Damage, DamageCauser);
}

FVector AStormCharacterBase::GetLockOnPoint() const
{
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	return Capsule->GetComponentLocation() + FVector(0.f, 0.f, Capsule->GetScaledCapsuleHalfHeight() * LockOnPointHeightRatio);
}

void AStormCharacterBase::BeginAttackLunge(float Distance, float Duration, bool bStopAtTarget, bool bPassThroughPawns)
{
	EndAttackLunge();
	if (!IsAttacking() || Distance <= 0.f || Duration <= 0.f)
	{
		return;
	}

	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	if (const AActor* Target = bStopAtTarget ? GetAttackTarget() : nullptr)
	{
		const float Ahead = FVector::DotProduct(Target->GetActorLocation() - GetActorLocation(), Forward);
		Distance = FMath::Clamp(Ahead - AttackLungeStopDistance, 0.f, Distance);
		if (Distance <= 0.f)
		{
			return;
		}
	}

	if (bPassThroughPawns)
	{
		UCapsuleComponent* Capsule = GetCapsuleComponent();
		PawnResponseBeforeLunge = Capsule->GetCollisionResponseToChannel(ECC_Pawn);
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		bAttackLungePassingPawns = true;
	}

	// 이동 컴포넌트 시간으로 돌아서 히트스톱(CustomTimeDilation) 동안 같이 멈춘다.
	TSharedPtr<FRootMotionSource_ConstantForce> Force = MakeShared<FRootMotionSource_ConstantForce>();
	Force->InstanceName = TEXT("StormAttackLunge");
	Force->AccumulateMode = ERootMotionAccumulateMode::Override;
	Force->Priority = 4;
	Force->Force = Forward * (Distance / Duration);
	Force->Duration = Duration;
	Force->Settings.SetFlag(ERootMotionSourceSettingsFlags::IgnoreZAccumulate);
	// 끝나면 바로 선다 (공격 중에는 이동하지 않으니 미끄러지지 않게).
	Force->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::ClampVelocity;
	Force->FinishVelocityParams.ClampVelocity = 0.f;

	AttackLungeID = GetCharacterMovement()->ApplyRootMotionSource(Force);
}

void AStormCharacterBase::EndAttackLunge()
{
	if (bAttackLungePassingPawns)
	{
		// 폰과 겹친 채로 되돌리면 다음 이동에서 CharacterMovement가 밀어낸다.
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, PawnResponseBeforeLunge);
		bAttackLungePassingPawns = false;
	}

	if (AttackLungeID != 0)
	{
		// 시간이 다 돼 이미 사라진 소스면 아무 일도 없다.
		GetCharacterMovement()->RemoveRootMotionSourceByID(AttackLungeID);
		AttackLungeID = 0;
	}
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

	// 다른 캐릭터가 플레이어와 카메라 사이에 들어와도 스프링암이 줄어들지 않게 한다.
	// 생성자에서 바꾸면 BP에 저장된 충돌 프리셋(Pawn, CharacterMesh)을 로드할 때 덮어써져서 여기서 한다.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	// 예고 장판 같은 바닥 데칼이 캐릭터 몸에 그려지지 않게 한다.
	GetMesh()->SetReceivesDecals(false);
}

void AStormCharacterBase::HandleDeath(AActor* Killer)
{
	EndAttackLunge();
	// 공중에서 죽으면(낙사) 그대로 떨어지게 둔다. 이동을 끄면 공중에 멈추고, 사망 몽타주도 서서 쓰러지는 동작이라 틀지 않는다
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement->IsFalling())
	{
		return;
	}
	Movement->DisableMovement();
	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}
}
