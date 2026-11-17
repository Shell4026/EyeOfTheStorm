#include "Animation/AnimNotifyState_StormSlow.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

UAnimNotifyState_StormSlow::UAnimNotifyState_StormSlow()
{
}

void UAnimNotifyState_StormSlow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	ScalePlayRate(MeshComp, Animation, SpeedScale);
}

void UAnimNotifyState_StormSlow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	ScalePlayRate(MeshComp, Animation, 1.f / SpeedScale);
}

FString UAnimNotifyState_StormSlow::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Slow x%.2f"), SpeedScale);
}

void UAnimNotifyState_StormSlow::ScalePlayRate(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float Scale) const
{
	UAnimInstance* AnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	const UAnimMontage* Montage = Cast<UAnimMontage>(Animation);
	if (!AnimInstance || !Montage || !AnimInstance->Montage_IsPlaying(Montage))
		return;
	AnimInstance->Montage_SetPlayRate(Montage, AnimInstance->Montage_GetPlayRate(Montage) * Scale);
}
