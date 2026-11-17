#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_StormAttackLunge.generated.h"

UCLASS(meta = (DisplayName = "Storm Attack Lunge"))
class EYEOFTHESTORM_API UAnimNotifyState_StormAttackLunge : public UAnimNotifyState
{
	GENERATED_BODY()
public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
public:
	UPROPERTY(EditAnywhere, Category = "Storm", meta = (ClampMin = "0"))
	float Distance = 60.f;
	// 끄면 대상을 지나쳐서라도 Distance를 다 간다.
	UPROPERTY(EditAnywhere, Category = "Storm")
	bool bStopAtTarget = true;
	// 구간 동안 다른 캐릭터와 서로 통과한다 (관통 돌진).
	UPROPERTY(EditAnywhere, Category = "Storm")
	bool bPassThroughPawns = false;
};
