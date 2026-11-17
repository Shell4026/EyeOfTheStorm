#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_StormTelegraph.generated.h"

class UAnimNotify_StormRadialHit;

UCLASS(meta = (DisplayName = "Storm Telegraph"))
class EYEOFTHESTORM_API UAnimNotifyState_StormTelegraph : public UAnimNotifyState
{
	GENERATED_BODY()
public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
private:
	// StartTime 이후 가장 먼저 오는 Storm Radial Hit. 없으면 nullptr
	static const UAnimNotify_StormRadialHit* FindNextRadialHit(const UAnimSequenceBase* Animation, float StartTime);
};
