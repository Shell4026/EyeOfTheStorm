#include "AI/StormBossAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "Character/StormBossCharacter.h"

DEFINE_LOG_CATEGORY_STATIC(LogStormBossAI, Log, All);

void AStormBossAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	const AStormBossCharacter* Boss = Cast<AStormBossCharacter>(InPawn);
	UBehaviorTree* Tree = Boss ? Boss->GetBehaviorTree() : nullptr;
	if (!Tree)
	{
		UE_LOG(LogStormBossAI, Warning, TEXT("%s: %s의 BehaviorTree가 비어 있어 움직이지 않는다."), *GetName(), *GetNameSafe(InPawn));
		return;
	}
	RunBehaviorTree(Tree);
}
