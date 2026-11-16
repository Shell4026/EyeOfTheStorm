#include "Animation/AnimNotifyState_StormHitWindow.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotifyState_StormHitWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	// 에디터 미리보기 액터에는 컴포넌트가 없다.
	UStormMeleeComponent* Melee = Owner ? Owner->FindComponentByClass<UStormMeleeComponent>() : nullptr;
	if (!Melee)
	{
		return;
	}

	// 다음 타로 넘어가 블렌드 아웃 중인 몽타주의 노티파이는 무시한다 (블렌드 아웃 중에도 노티파이는 계속 불린다).
	const UAnimMontage* Montage = Cast<UAnimMontage>(Animation);
	const UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
	if (Montage && AnimInstance && AnimInstance->Montage_GetIsStopped(Montage))
	{
		return;
	}

	Melee->BeginHitWindow(this, MeshComp, Settings);
}

void UAnimNotifyState_StormHitWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (UStormMeleeComponent* Melee = Owner ? Owner->FindComponentByClass<UStormMeleeComponent>() : nullptr)
	{
		Melee->EndHitWindow(this);
	}

	Super::NotifyEnd(MeshComp, Animation, EventReference);
}

FString UAnimNotifyState_StormHitWindow::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("HitWindow %s"), *Settings.EndBone.ToString());
}
