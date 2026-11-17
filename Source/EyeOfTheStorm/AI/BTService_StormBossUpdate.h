#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_StormBossUpdate.generated.h"

UCLASS(meta = (DisplayName = "Storm Boss Update"))
class EYEOFTHESTORM_API UBTService_StormBossUpdate : public UBTService
{
	GENERATED_BODY()
public:
	UBTService_StormBossUpdate();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
protected:
	UPROPERTY(EditAnywhere, Category = "Storm")
	FBlackboardKeySelector TargetKey;
	// 지금 쓸 수 있는 패턴이 있는지 (AStormBossCharacter::HasAvailablePattern)
	UPROPERTY(EditAnywhere, Category = "Storm")
	FBlackboardKeySelector HasPatternKey;
};
