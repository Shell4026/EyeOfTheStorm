#pragma once
#include "Combat/StormMeleeComponent.h"

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_StormHitWindow.generated.h"

UCLASS(meta = (DisplayName = "Storm Hit Window"))
class EYEOFTHESTORM_API UAnimNotifyState_StormHitWindow : public UAnimNotifyState
{
	GENERATED_BODY()
public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
public:
	UPROPERTY(EditAnywhere, Category = "Storm", meta = (ShowOnlyInnerProperties))
	FStormHitWindowSettings Settings;
};
