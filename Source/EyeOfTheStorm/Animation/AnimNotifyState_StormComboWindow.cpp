#include "Animation/AnimNotifyState_StormComboWindow.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/StormPlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotifyState_StormComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AStormPlayerCharacter* Player = MeshComp ? Cast<AStormPlayerCharacter>(MeshComp->GetOwner()) : nullptr;
	if (!Player)
	{
		return;
	}

	// 이미 다음 타로 넘어가 블렌드 아웃 중인 몽타주의 노티파이는 무시한다.
	const UAnimMontage* Montage = Cast<UAnimMontage>(Animation);
	const UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
	if (Montage && AnimInstance && AnimInstance->Montage_GetIsStopped(Montage))
	{
		return;
	}

	Player->OpenComboWindow();
}

void UAnimNotifyState_StormComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (AStormPlayerCharacter* Player = MeshComp ? Cast<AStormPlayerCharacter>(MeshComp->GetOwner()) : nullptr)
	{
		Player->CloseComboWindow();
	}

	Super::NotifyEnd(MeshComp, Animation, EventReference);
}

FString UAnimNotifyState_StormComboWindow::GetNotifyName_Implementation() const
{
	return TEXT("ComboWindow");
}
