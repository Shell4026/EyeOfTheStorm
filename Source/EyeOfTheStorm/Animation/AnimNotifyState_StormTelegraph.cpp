#include "Animation/AnimNotifyState_StormTelegraph.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimNotify_StormRadialHit.h"
#include "Combat/StormMeleeComponent.h"
#include "Components/SkeletalMeshComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogStormTelegraph, Log, All);

void UAnimNotifyState_StormTelegraph::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

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

	const FAnimNotifyEvent* Event = EventReference.GetNotify();
	const UAnimNotify_StormRadialHit* Hit = FindNextRadialHit(Animation, Event ? Event->GetTriggerTime() : 0.f);
	if (!Hit)
	{
		UE_LOG(LogStormTelegraph, Warning, TEXT("%s: Storm Telegraph 뒤에 Storm Radial Hit이 없다."), *GetNameSafe(Animation));
		return;
	}
	if (!Hit->CenterBone.IsNone())
	{
		// 본 위치는 지금과 때리는 순간이 달라서 미리 알 수 없다. 액터 기준으로만 그린다.
		UE_LOG(LogStormTelegraph, Warning, TEXT("%s: Radial Hit의 CenterBone(%s)은 예고 장판에 반영되지 않는다."), *GetNameSafe(Animation), *Hit->CenterBone.ToString());
	}
	Melee->ShowTelegraph(this, Hit->Offset, Hit->Radius);
}

void UAnimNotifyState_StormTelegraph::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (UStormMeleeComponent* Melee = Owner ? Owner->FindComponentByClass<UStormMeleeComponent>() : nullptr)
	{
		Melee->HideTelegraph(this);
	}
}

FString UAnimNotifyState_StormTelegraph::GetNotifyName_Implementation() const
{
	return TEXT("Telegraph");
}

const UAnimNotify_StormRadialHit* UAnimNotifyState_StormTelegraph::FindNextRadialHit(const UAnimSequenceBase* Animation, float StartTime)
{
	if (!Animation)
	{
		return nullptr;
	}

	const UAnimNotify_StormRadialHit* Found = nullptr;
	float FoundTime = TNumericLimits<float>::Max();
	for (const FAnimNotifyEvent& Event : Animation->Notifies)
	{
		const UAnimNotify_StormRadialHit* Hit = Cast<UAnimNotify_StormRadialHit>(Event.Notify);
		const float Time = Event.GetTriggerTime();
		if (Hit && Time >= StartTime && Time < FoundTime)
		{
			Found = Hit;
			FoundTime = Time;
		}
	}
	return Found;
}
