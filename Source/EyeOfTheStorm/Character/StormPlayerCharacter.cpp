#include "Character/StormPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Combat/StormLockOnComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/RootMotionSource.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "TimerManager.h"

AStormPlayerCharacter::AStormPlayerCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -96.f), FRotator(0.f, -90.f, 0.f));

	// 캐릭터는 컨트롤러가 아니라 이동 방향(평상시) 또는 락온 대상(락온 중)을 본다.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, 600.f, 0.f);
	Movement->MaxWalkSpeed = WalkSpeed;
	Movement->BrakingDecelerationWalking = 2000.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 60.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	LockOn = CreateDefaultSubobject<UStormLockOnComponent>(TEXT("LockOn"));
}

void AStormPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	if (MoveAction)
	{
		Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AStormPlayerCharacter::Move);
		Input->BindAction(MoveAction, ETriggerEvent::Completed, this, &AStormPlayerCharacter::MoveCompleted);
	}
	if (LookAction)
	{
		Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AStormPlayerCharacter::Look);
	}
	if (DodgeAction)
	{
		Input->BindAction(DodgeAction, ETriggerEvent::Started, this, &AStormPlayerCharacter::Dodge);
	}
	if (LockOnAction)
	{
		Input->BindAction(LockOnAction, ETriggerEvent::Started, this, &AStormPlayerCharacter::ToggleLockOn);
	}
	if (RunAction)
	{
		Input->BindAction(RunAction, ETriggerEvent::Started, this, &AStormPlayerCharacter::Run);
		Input->BindAction(RunAction, ETriggerEvent::Completed, this, &AStormPlayerCharacter::RunCompleted);
	}
}

void AStormPlayerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	if (const APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

void AStormPlayerCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// 레벨에 배치했을 때 에디터 뷰포트에서도 표정이 보이게.
	ApplyDefaultMorphTargets();
}

bool AStormPlayerCharacter::IsLockedOn() const
{
	return LockOn->IsLockedOn();
}

void AStormPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	LockOn->OnTargetChanged.AddDynamic(this, &AStormPlayerCharacter::HandleLockOnTargetChanged);
	ApplyDefaultMorphTargets();
	UpdateRotationMode();
}

void AStormPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	const float Target = bIsRunning ? RunSpeed : WalkSpeed;
	Movement->MaxWalkSpeed = FMath::FInterpConstantTo(Movement->MaxWalkSpeed, Target, DeltaSeconds, SpeedChangeRate);
}

void AStormPlayerCharacter::HandleDeath(AActor* Killer)
{
	EndDodge();
	LockOn->ClearLockOn();
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		DisableInput(PC);
	}
	Super::HandleDeath(Killer);
}

void AStormPlayerCharacter::Move(const FInputActionValue& Value)
{
	MoveInput = Value.Get<FVector2D>();
	if (bIsDodging || IsDead())
	{
		return;
	}

	const FVector Direction = GetMoveInputWorldDirection();
	if (!Direction.IsNearlyZero())
	{
		AddMovementInput(Direction, FMath::Min(1.f, MoveInput.Size()));
	}
}

void AStormPlayerCharacter::MoveCompleted(const FInputActionValue& Value)
{
	MoveInput = FVector2D::ZeroVector;
}

void AStormPlayerCharacter::Look(const FInputActionValue& Value)
{
	// 락온 중에는 카메라를 락온 컴포넌트가 돌린다.
	if (IsLockedOn())
		return;

	const FVector2D LookAxis = Value.Get<FVector2D>();
	AddControllerYawInput(LookAxis.X);
	AddControllerPitchInput(LookAxis.Y);
}

void AStormPlayerCharacter::Dodge()
{
	if (bIsDodging || IsDead() || GetCharacterMovement()->IsFalling())
	{
		return;
	}

	const FVector InputDirection = GetMoveInputWorldDirection();
	if (InputDirection.IsNearlyZero())
	{
		StartDodge(-GetActorForwardVector().GetSafeNormal2D(), false);
	}
	else
	{
		StartDodge(InputDirection, true);
	}
}

void AStormPlayerCharacter::ToggleLockOn()
{
	if (!IsDead())
	{
		LockOn->ToggleLockOn();
	}
}

void AStormPlayerCharacter::Run()
{
	bIsRunning = true;
}

void AStormPlayerCharacter::RunCompleted()
{
	bIsRunning = false;
}

void AStormPlayerCharacter::StartDodge(const FVector& Direction, bool bFaceDirection)
{
	bIsDodging = true;
	UpdateRotationMode();
	if (bFaceDirection)
	{
		SetActorRotation(Direction.Rotation());
	}

	// 달리는 중이면 더 멀리, 더 빨리 구른다. 몽타주와 이동이 같이 끝나도록 시간은 재생 속도 배율로 나눈다.
	const bool bRunDodge = bIsRunning && GetVelocity().Size2D() > WalkSpeed;
	const float Distance = bRunDodge ? DodgeDistance * RunDodgeDistanceScale : DodgeDistance;
	const float RateScale = bRunDodge ? RunDodgePlayRateScale : 1.f;
	const float Duration = DodgeDuration / RateScale;
	const float PlayRate = DodgeMontagePlayRate * RateScale;

	UCharacterMovementComponent* Movement = GetCharacterMovement();

	TSharedPtr<FRootMotionSource_ConstantForce> Force = MakeShared<FRootMotionSource_ConstantForce>();
	Force->InstanceName = TEXT("StormDodge");
	Force->AccumulateMode = ERootMotionAccumulateMode::Override;
	Force->Priority = 5;
	Force->Force = Direction * (Distance / Duration);
	Force->Duration = Duration;
	Force->StrengthOverTime = DodgeSpeedCurve;
	Force->Settings.SetFlag(ERootMotionSourceSettingsFlags::IgnoreZAccumulate);
	Force->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::ClampVelocity;
	Force->FinishVelocityParams.ClampVelocity = DodgeExitSpeed;
	DodgeRootMotionID = Movement->ApplyRootMotionSource(Force);

	if (DodgeMontage)
	{
		PlayAnimMontage(DodgeMontage, PlayRate);
	}

	FTimerManager& Timers = GetWorldTimerManager();
	Timers.SetTimer(DodgeEndTimer, this, &AStormPlayerCharacter::EndDodge, Duration, false);

	const float InvStart = FMath::Clamp(DodgeInvincibleStart, 0.f, Duration);
	const float InvEnd = FMath::Clamp(DodgeInvincibleEnd, InvStart, Duration);
	if (InvEnd > InvStart)
	{
		if (InvStart <= 0.f)
		{
			BeginDodgeInvincibility();
		}
		else
		{
			Timers.SetTimer(DodgeInvincibleStartTimer, this, &AStormPlayerCharacter::BeginDodgeInvincibility, InvStart, false);
		}
		Timers.SetTimer(DodgeInvincibleEndTimer, this, &AStormPlayerCharacter::EndDodgeInvincibility, InvEnd, false);
	}
}

void AStormPlayerCharacter::EndDodge()
{
	if (!bIsDodging)
	{
		return;
	}

	FTimerManager& Timers = GetWorldTimerManager();
	Timers.ClearTimer(DodgeEndTimer);
	Timers.ClearTimer(DodgeInvincibleStartTimer);
	Timers.ClearTimer(DodgeInvincibleEndTimer);
	EndDodgeInvincibility();

	if (DodgeRootMotionID != 0)
	{
		GetCharacterMovement()->RemoveRootMotionSourceByID(DodgeRootMotionID);
		DodgeRootMotionID = 0;
	}

	bIsDodging = false;
	UpdateRotationMode();
}

void AStormPlayerCharacter::BeginDodgeInvincibility()
{
	if (!bDodgeInvincible)
	{
		bDodgeInvincible = true;
		AddInvincibility();
	}
}

void AStormPlayerCharacter::EndDodgeInvincibility()
{
	if (bDodgeInvincible)
	{
		bDodgeInvincible = false;
		RemoveInvincibility();
	}
}

void AStormPlayerCharacter::UpdateRotationMode()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (bIsDodging)
	{
		// 회피 중에는 StartDodge에서 정한 방향을 유지한다.
		Movement->bOrientRotationToMovement = false;
		Movement->bUseControllerDesiredRotation = false;
	}
	else if (IsLockedOn())
	{
		Movement->bOrientRotationToMovement = false;
		Movement->bUseControllerDesiredRotation = true;
	}
	else
	{
		Movement->bOrientRotationToMovement = true;
		Movement->bUseControllerDesiredRotation = false;
	}
}

void AStormPlayerCharacter::ApplyDefaultMorphTargets()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}
	for (const TPair<FName, float>& Morph : DefaultMorphTargets)
	{
		MeshComp->SetMorphTarget(Morph.Key, Morph.Value);
	}
}

void AStormPlayerCharacter::HandleLockOnTargetChanged(AStormCharacterBase* NewTarget)
{
	UpdateRotationMode();
}

FVector AStormPlayerCharacter::GetMoveInputWorldDirection() const
{
	if (MoveInput.IsNearlyZero() || !Controller)
	{
		return FVector::ZeroVector;
	}
	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	return (Forward * MoveInput.Y + Right * MoveInput.X).GetSafeNormal2D();
}
