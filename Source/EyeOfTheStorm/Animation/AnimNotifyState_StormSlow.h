#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_StormSlow.generated.h"

UCLASS(meta = (DisplayName = "Storm Slow"))
class EYEOFTHESTORM_API UAnimNotifyState_StormSlow : public UAnimNotifyState
{
	GENERATED_BODY()
public:
	UAnimNotifyState_StormSlow();

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
private:
	void ScalePlayRate(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float Scale) const;
public:
	UPROPERTY(EditAnywhere, Category = "Storm", meta = (ClampMin = "0.05"))
	float SpeedScale = 1.f;
};
