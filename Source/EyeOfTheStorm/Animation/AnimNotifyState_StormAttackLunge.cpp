#include "Animation/AnimNotifyState_StormAttackLunge.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/StormPlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotifyState_StormAttackLunge::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AStormPlayerCharacter* Player = MeshComp ? Cast<AStormPlayerCharacter>(MeshComp->GetOwner()) : nullptr;
	const UAnimInstance* AnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (!Player || !AnimInstance)
	{
		return;
	}

	// 이미 다음 타로 넘어가 블렌드 아웃 중인 몽타주의 노티파이는 무시한다.
	const UAnimMontage* Montage = Cast<UAnimMontage>(Animation);
	if (Montage && AnimInstance->Montage_GetIsStopped(Montage))
	{
		return;
	}

	// TotalDuration은 애니메이션 시간이다. 재생 속도(PlayRate x 에셋 RateScale)로 나눠 실제 시간으로 바꾼다.
	const float PlayRate = Montage ? AnimInstance->Montage_GetEffectivePlayRate(Montage) : 1.f;
	const float Duration = TotalDuration / FMath::Max(PlayRate, UE_KINDA_SMALL_NUMBER);
	Player->BeginAttackLunge(Distance, Duration);
}

FString UAnimNotifyState_StormAttackLunge::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Lunge %.0fcm"), Distance);
}
