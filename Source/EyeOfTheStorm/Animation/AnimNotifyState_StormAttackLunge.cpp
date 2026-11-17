#include "Animation/AnimNotifyState_StormAttackLunge.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/StormCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotifyState_StormAttackLunge::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AStormCharacterBase* Character = MeshComp ? Cast<AStormCharacterBase>(MeshComp->GetOwner()) : nullptr;
	const UAnimInstance* AnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (!Character || !AnimInstance)
		return;

	const UAnimMontage* Montage = Cast<UAnimMontage>(Animation);
	if (Montage && AnimInstance->Montage_GetIsStopped(Montage))
		return;

	// TotalDuration은 애니메이션 시간이다. 재생 속도(PlayRate * 에셋 RateScale)로 나눠 실제 시간으로 바꾼다.
	const float PlayRate = Montage ? AnimInstance->Montage_GetEffectivePlayRate(Montage) : 1.f;
	const float Duration = TotalDuration / FMath::Max(PlayRate, UE_KINDA_SMALL_NUMBER);
	Character->BeginAttackLunge(Distance, Duration, bStopAtTarget, bPassThroughPawns);
}

void UAnimNotifyState_StormAttackLunge::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	AStormCharacterBase* Character = MeshComp ? Cast<AStormCharacterBase>(MeshComp->GetOwner()) : nullptr;
	const UAnimInstance* AnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (!Character || !AnimInstance)
	{
		return;
	}

	const UAnimMontage* Montage = Cast<UAnimMontage>(Animation);
	if (Montage && AnimInstance->Montage_GetIsStopped(Montage))
		return;

	Character->EndAttackLunge();
}

FString UAnimNotifyState_StormAttackLunge::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Lunge %.0fcm%s%s"), Distance, bStopAtTarget ? TEXT("") : TEXT(" Full"), bPassThroughPawns ? TEXT(" Pass") : TEXT(""));
}
