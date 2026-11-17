#include "Animation/AnimNotify_StormRadialHit.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Combat/StormMeleeComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_StormRadialHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	// 에디터 미리보기 액터에는 컴포넌트가 없다.
	UStormMeleeComponent* Melee = Owner ? Owner->FindComponentByClass<UStormMeleeComponent>() : nullptr;
	if (!Melee)
	{
		return;
	}

	// 다음 패턴으로 넘어가 블렌드 아웃 중인 몽타주의 노티파이는 무시한다.
	const UAnimMontage* Montage = Cast<UAnimMontage>(Animation);
	const UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
	if (Montage && AnimInstance && AnimInstance->Montage_GetIsStopped(Montage))
	{
		return;
	}

	const FVector Base = CenterBone.IsNone() ? Owner->GetActorLocation() : MeshComp->GetSocketLocation(CenterBone);
	const FVector Center = Base + Owner->GetActorRotation().RotateVector(Offset);
	Melee->ApplyRadialHit(Center, Radius, Damage, HitStopDuration);
}

FString UAnimNotify_StormRadialHit::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("RadialHit %.0fcm"), Radius);
}
