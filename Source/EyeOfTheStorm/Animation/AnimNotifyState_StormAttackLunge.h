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
	virtual FString GetNotifyName_Implementation() const override;
public:
	UPROPERTY(EditAnywhere, Category = "Storm", meta = (ClampMin = "0"))
	float Distance = 60.f;
};
