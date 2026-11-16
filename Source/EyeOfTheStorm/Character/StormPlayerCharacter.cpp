#include "Character/StormPlayerCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Camera/CameraComponent.h"
#include "Combat/StormLockOnComponent.h"
#include "Combat/StormMeleeComponent.h"

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
	// 붐이 캐릭터 회전을 물려받지 않게 한다. 카메라 위치는 붐 기준 상대값으로 저장되는데, 붐이 틱한 뒤
	// 타이머(버퍼된 회피)가 캐릭터를 돌리면 카메라 갱신 전까지 한 프레임 카메라가 같이 돌아 화면이 튄다.
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	LockOn = CreateDefaultSubobject<UStormLockOnComponent>(TEXT("LockOn"));
	Melee = CreateDefaultSubobject<UStormMeleeComponent>(TEXT("Melee"));
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
	if (AttackAction)
	{
		Input->BindAction(AttackAction, ETriggerEvent::Started, this, &AStormPlayerCharacter::Attack);
	}
	if (ZoomAction)
	{
		Input->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AStormPlayerCharacter::Zoom);
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

void AStormPlayerCharacter::OpenComboWindow()
{
	if (!bIsAttacking)
	{
		return;
	}
	bComboWindowOpen = true;
	bAttackCancelable = true;

	// 공격 중에 누른 회피가 다음 타보다 먼저다.
	if (bDodgeBuffered)
	{
		bAttackBuffered = false;
		TryBufferedDodge();
		return;
	}
	if (bAttackBuffered)
	{
		StartAttack(ComboIndex + 1);
	}
}

void AStormPlayerCharacter::CloseComboWindow()
{
	bComboWindowOpen = false;
}

void AStormPlayerCharacter::BeginAttackLunge(float Distance, float Duration)
{
	EndAttackLunge();
	if (!bIsAttacking || Distance <= 0.f || Duration <= 0.f)
	{
		return;
	}

	// 방향은 StartAttack에서 정한 몸 방향 그대로다.
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();

	// 락온 중이면 대상 앞에서 멈춘다 (붙어 있으면 제자리에서 찬다).
	if (const AStormCharacterBase* Target = LockOn->GetTarget())
	{
		const float Ahead = FVector::DotProduct(Target->GetActorLocation() - GetActorLocation(), Forward);
		Distance = FMath::Clamp(Ahead - AttackLungeStopDistance, 0.f, Distance);
		if (Distance <= 0.f)
		{
			return;
		}
	}

	// 회피와 같은 방식이다. 이동 컴포넌트 시간으로 돌아서 히트스톱(CustomTimeDilation) 동안 같이 멈춘다.
	TSharedPtr<FRootMotionSource_ConstantForce> Force = MakeShared<FRootMotionSource_ConstantForce>();
	Force->InstanceName = TEXT("StormAttackLunge");
	Force->AccumulateMode = ERootMotionAccumulateMode::Override;
	Force->Priority = 4;
	Force->Force = Forward * (Distance / Duration);
	Force->Duration = Duration;
	Force->Settings.SetFlag(ERootMotionSourceSettingsFlags::IgnoreZAccumulate);
	// 끝나면 바로 선다 (공격 중에는 이동 입력을 안 받으니 미끄러지지 않게).
	Force->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::ClampVelocity;
	Force->FinishVelocityParams.ClampVelocity = 0.f;
	AttackLungeID = GetCharacterMovement()->ApplyRootMotionSource(Force);
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

	TargetZoomDistance = CameraBoom->TargetArmLength;

	UpdateRotationMode();
}

void AStormPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	const float Target = bIsRunning ? RunSpeed : WalkSpeed;
	Movement->MaxWalkSpeed = FMath::FInterpConstantTo(Movement->MaxWalkSpeed, Target, DeltaSeconds, SpeedChangeRate);

	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetZoomDistance, DeltaSeconds, ZoomInterpSpeed);
}

void AStormPlayerCharacter::HandleDeath(AActor* Killer)
{
	bDodgeBuffered = false;
	bAttackBuffered = false;
	EndAttack();
	EndDodge();
	EndDodgeForce();
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
	if (bIsDodging || bIsAttacking || IsDead())
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
	const FVector2D LookAxis = Value.Get<FVector2D>();
	if (IsLockedOn())
	{
		AddControllerPitchInput(LookAxis.Y);
		return;
	}
	AddControllerYawInput(LookAxis.X);
	AddControllerPitchInput(LookAxis.Y);
}

void AStormPlayerCharacter::Dodge()
{
	// 콤보 구간이 열리기 전(차는 동작의 앞부분)에 누른 회피는 기억했다가 구간이 열리면 구른다.
	if (bIsAttacking && !bAttackCancelable)
	{
		bDodgeBuffered = true;
		return;
	}

	// 구르기 끝(RollEnd) 직전부터 회복(힘이 남은 꼬리 구간)이 끝날 때까지 누른 키는 기억했다가 회복이 끝나면 구른다.
	// 구르기 초반에 누른 키는 버린다.
	if (bIsDodging || DodgeRootMotionID != 0)
	{
		const float Remaining = GetWorldTimerManager().GetTimerRemaining(DodgeEndTimer);
		if (!bIsDodging || (Remaining >= 0.f && Remaining <= DodgeBufferTime))
		{
			bDodgeBuffered = true;
		}
		return;
	}
	if (!CanDodge())
	{
		return;
	}

	const FVector InputDirection = GetMoveInputWorldDirection();
	if (InputDirection.IsNearlyZero())
	{
		StartDodge(-GetActorForwardVector().GetSafeNormal2D(), true);
		return;
	}

	// 락온 중에는 대상을 보고 있으니, 뒤쪽 입력은 돌아서지 않고 뒤로 구른다.
	const float BackDot = FVector::DotProduct(InputDirection, GetActorForwardVector().GetSafeNormal2D());
	const bool bBackward = IsLockedOn() && BackDot <= -FMath::Cos(FMath::DegreesToRadians(DodgeBackAngle));
	StartDodge(InputDirection, bBackward);
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

void AStormPlayerCharacter::Zoom(const FInputActionValue& Value)
{
	TargetZoomDistance = FMath::Clamp(TargetZoomDistance - Value.Get<float>() * ZoomStep, MinZoomDistance, MaxZoomDistance);
}

void AStormPlayerCharacter::StartDodge(const FVector& Direction, bool bBackward)
{
	// 공격 뒷부분을 회피로 끊는다. 공격 몽타주는 아래 회피 몽타주가 밀어낸다.
	EndAttack();

	bIsDodging = true;
	UpdateRotationMode();
	SetActorRotation(bBackward ? (-Direction).Rotation() : Direction.Rotation());

	UAnimMontage* Montage = (bBackward && DodgeBackMontage) ? DodgeBackMontage.Get() : DodgeMontage.Get();

	// 달리는 중이면 더 멀리, 더 빨리 구른다. 몽타주와 이동이 같이 끝나도록 시간은 재생 속도 배율로 나눈다.
	const bool bRunDodge = bIsRunning && GetVelocity().Size2D() > WalkSpeed;
	const float BaseDistance = bBackward ? DodgeBackDistance : DodgeDistance;
	const float Distance = bRunDodge ? BaseDistance * RunDodgeDistanceScale : BaseDistance;
	
	// 조작은 몽타주의 RollEnd에서 돌려주고(EndDodge), 힘은 몽타주 끝까지 준다 (일어나는 동안 발이 미끄러지지 않게).
	// 재생 속도가 빠르면 둘 다 그만큼 일찍 도달한다.
	const float RateScale = bRunDodge ? RunDodgePlayRateScale : 1.f;
	const float PlayRate = DodgeMontagePlayRate * RateScale;
	const float Duration = GetDodgeMoveTime(Montage) / PlayRate;
	const float ForceDuration = Montage ? FMath::Max(Montage->GetPlayLength() / PlayRate, Duration) : Duration;

	// 이전 회피의 힘이 아직 남아 있으면 지운다 (RollEnd 뒤 다시 구른 경우).
	EndDodgeForce();

	UCharacterMovementComponent* Movement = GetCharacterMovement();

	TSharedPtr<FRootMotionSource_ConstantForce> Force = MakeShared<FRootMotionSource_ConstantForce>();
	Force->InstanceName = TEXT("StormDodge");
	Force->AccumulateMode = ERootMotionAccumulateMode::Override;
	Force->Priority = 5;
	Force->Force = Direction * (Distance / ForceDuration);
	Force->Duration = ForceDuration;
	Force->StrengthOverTime = DodgeSpeedCurve;
	Force->Settings.SetFlag(ERootMotionSourceSettingsFlags::IgnoreZAccumulate);
	Force->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::ClampVelocity;

	// 입력을 누른 채 구르면 걷기/달리기 속도로, 입력 없이 구르면 DodgeExitSpeed로 끝난다.
	const float ExitSpeed = MoveInput.IsNearlyZero() ? DodgeExitSpeed : (bRunDodge ? RunSpeed : WalkSpeed);
	Force->FinishVelocityParams.ClampVelocity = ExitSpeed;
	DodgeRootMotionID = Movement->ApplyRootMotionSource(Force);

	ActiveDodgeMontage = Montage;
	if (Montage)
	{
		PlayAnimMontage(Montage, PlayRate);
	}

	FTimerManager& Timers = GetWorldTimerManager();
	Timers.SetTimer(DodgeEndTimer, this, &AStormPlayerCharacter::EndDodge, Duration, false);
	Timers.SetTimer(DodgeForceEndTimer, this, &AStormPlayerCharacter::EndDodgeForce, ForceDuration, false);

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

	// 남은 일어나기 구간은 블렌드 아웃 동안 섞여 사라지고, 슬롯 아래 블렌드 스페이스(속도 기준 대기/걷기/달리기)가 드러난다.
	// 블렌드 시간과 방식(Standard/Inertialization)은 몽타주 에셋의 Blend Out 설정을 따른다.
	if (ActiveDodgeMontage)
	{
		StopAnimMontage(ActiveDodgeMontage);
		ActiveDodgeMontage = nullptr;
	}

	bIsDodging = false;
	UpdateRotationMode();

	// 꼬리 구간이 없으면(몽타주 없음) 여기서 바로 버퍼를 처리한다. 있으면 EndDodgeForce에서.
	TryBufferedDodge();
}

void AStormPlayerCharacter::EndDodgeForce()
{
	GetWorldTimerManager().ClearTimer(DodgeForceEndTimer);
	if (DodgeRootMotionID == 0)
	{
		return;
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->RemoveRootMotionSourceByID(DodgeRootMotionID);
	DodgeRootMotionID = 0;

	// 끝 속도(걷기/달리기)는 시작할 때 정해지므로, 그사이 손을 뗐으면 한 번 더 자른다.
	if (MoveInput.IsNearlyZero())
	{
		Movement->Velocity = Movement->Velocity.GetClampedToMaxSize2D(DodgeExitSpeed);
	}
	UpdateRotationMode();

	TryBufferedDodge();
}

void AStormPlayerCharacter::TryBufferedDodge()
{
	if (!bDodgeBuffered || !CanDodge())
	{
		return;
	}
	bDodgeBuffered = false;
	// 방향은 누른 순간이 아니라 지금 입력으로 정한다.
	Dodge();
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

void AStormPlayerCharacter::Attack()
{
	if (bIsAttacking)
	{
		// 콤보 구간이 지나간 뒤(회복 동작)에 누른 키와 마지막 타 중에 누른 키는 버린다.
		const bool bWindowPassed = bAttackCancelable && !bComboWindowOpen;
		if (bWindowPassed || !ComboMontages.IsValidIndex(ComboIndex + 1))
		{
			return;
		}
		// 구간이 열리기 전에 누르면 기억했다가 열릴 때, 열려 있으면 바로 다음 타.
		bAttackBuffered = true;
		if (bComboWindowOpen)
		{
			StartAttack(ComboIndex + 1);
		}
		return;
	}
	if (CanAttack())
	{
		StartAttack(0);
	}
}

void AStormPlayerCharacter::StartAttack(int32 Index)
{
	UAnimMontage* Montage = ComboMontages.IsValidIndex(Index) ? ComboMontages[Index].Get() : nullptr;
	if (!Montage)
	{
		EndAttack();
		return;
	}

	bIsAttacking = true;
	bComboWindowOpen = false;
	bAttackCancelable = false;
	bAttackBuffered = false;
	bDodgeBuffered = false;
	ComboIndex = Index;
	Melee->CancelHitWindow();
	EndAttackLunge();

	// 구르기 회복(힘이 남은 꼬리 구간) 중이면 끊고 찬다. 위에서 버퍼를 비웠으니 다시 구르지 않는다.
	EndDodgeForce();
	UpdateRotationMode();

	// 락온 중이면 대상을, 아니면 입력 방향을 본다. 입력이 없으면 지금 방향 그대로.
	FVector Facing = GetMoveInputWorldDirection();
	if (const AStormCharacterBase* Target = LockOn->GetTarget())
	{
		Facing = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	}
	if (!Facing.IsNearlyZero())
	{
		SetActorRotation(Facing.Rotation());
	}

	// 다음 타를 먼저 기억해 둬야 이전 타의 블렌드 아웃 알림을 걸러낼 수 있다.
	ActiveAttackMontage = Montage;
	if (PlayAnimMontage(Montage, AttackMontagePlayRate) <= 0.f)
	{
		EndAttack();
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		FOnMontageBlendingOutStarted BlendingOut;
		BlendingOut.BindUObject(this, &AStormPlayerCharacter::HandleAttackMontageBlendingOut);
		AnimInstance->Montage_SetBlendingOutDelegate(BlendingOut, Montage);
	}
}

void AStormPlayerCharacter::EndAttack()
{
	if (!bIsAttacking)
	{
		return;
	}

	bIsAttacking = false;
	bComboWindowOpen = false;
	bAttackCancelable = false;
	bAttackBuffered = false;
	ComboIndex = 0;
	ActiveAttackMontage = nullptr;
	Melee->CancelHitWindow();
	EndAttackLunge();
	UpdateRotationMode();

	// 콤보 구간 노티파이가 없는 몽타주라면 회피 버퍼는 여기서 처리한다.
	TryBufferedDodge();
}

void AStormPlayerCharacter::HandleAttackMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	// 다음 타·회피로 넘어가며 밀려난 이전 몽타주는 무시한다.
	if (Montage == ActiveAttackMontage)
	{
		EndAttack();
	}
}

void AStormPlayerCharacter::EndAttackLunge()
{
	if (AttackLungeID == 0)
	{
		return;
	}
	// 시간이 다 돼 이미 사라진 소스면 아무 일도 없다.
	GetCharacterMovement()->RemoveRootMotionSourceByID(AttackLungeID);
	AttackLungeID = 0;
}

void AStormPlayerCharacter::UpdateRotationMode()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (bIsDodging || DodgeRootMotionID != 0 || bIsAttacking)
	{
		// 회피 중과 힘이 남은 동안에는 StartDodge에서 정한 방향을 유지한다 (백스텝이 이동 방향으로 돌아서지 않게).
		// 공격 중에는 StartAttack에서 정한 방향을 유지한다.
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

float AStormPlayerCharacter::GetDodgeMoveTime(const UAnimMontage* Montage) const
{
	if (Montage)
	{
		for (const FAnimNotifyEvent& Event : Montage->Notifies)
		{
			// 스켈레톤 노티파이는 이벤트 이름, Montage Notify 같은 노티파이 객체는 GetNotifyName()에 이름이 있다.
			const bool bMatch = Event.Notify ? 
				FName(Event.Notify->GetNotifyName()) == DodgeEndNotifyName : 
				Event.NotifyName == DodgeEndNotifyName;
			if (bMatch && Event.GetTriggerTime() > 0.f)
			{
				return Event.GetTriggerTime();
			}
		}
	}
	return 1.f;
}

bool AStormPlayerCharacter::CanDodge() const
{
	return !bIsDodging && DodgeRootMotionID == 0 && !IsDead() && !GetCharacterMovement()->IsFalling()
		&& (!bIsAttacking || bAttackCancelable);
}

bool AStormPlayerCharacter::CanAttack() const
{
	// 구르는 중(RollEnd 전)에는 못 차고, 회복 중에는 찰 수 있다 (StartAttack이 회복을 끊는다).
	return !bIsDodging && !IsDead() && !GetCharacterMovement()->IsFalling() && ComboMontages.Num() > 0;
}
