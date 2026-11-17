#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_StormBossAttack.generated.h"

class AStormBossCharacter;

UCLASS(meta = (DisplayName = "Storm Boss Attack"))
class EYEOFTHESTORM_API UBTTask_StormBossAttack : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UBTTask_StormBossAttack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
private:
	void HandleAttackEnded(bool bInterrupted);
	void Unbind();
private:
	// 노드 인스턴스(bCreateNodeInstance)라 AI마다 따로 갖는다.
	TWeakObjectPtr<UBehaviorTreeComponent> OwnerCompPtr;
	TWeakObjectPtr<AStormBossCharacter> BossPtr;
	FDelegateHandle AttackEndedHandle;
};
