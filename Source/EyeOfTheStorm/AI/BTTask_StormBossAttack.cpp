#include "AI/BTTask_StormBossAttack.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Character/StormBossCharacter.h"

UBTTask_StormBossAttack::UBTTask_StormBossAttack()
{
	NodeName = TEXT("Storm Boss Attack");
	bCreateNodeInstance = true;
	bNotifyTaskFinished = true;
}

EBTNodeResult::Type UBTTask_StormBossAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* AI = OwnerComp.GetAIOwner();
	AStormBossCharacter* Boss = AI ? Cast<AStormBossCharacter>(AI->GetPawn()) : nullptr;
	if (!Boss || !Boss->StartRandomPattern())
	{
		return EBTNodeResult::Failed;
	}

	// 시작이 성공한 뒤에 묶는다. 실패할 때 부르는 OnAttackEnded가 이 실행 중에 FinishLatentTask를 부르지 않게.
	OwnerCompPtr = &OwnerComp;
	BossPtr = Boss;
	AttackEndedHandle = Boss->OnAttackEnded.AddUObject(this, &UBTTask_StormBossAttack::HandleAttackEnded);
	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_StormBossAttack::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 먼저 풀어야 StopPattern이 부르는 OnAttackEnded가 중단 중에 FinishLatentTask를 부르지 않는다.
	AStormBossCharacter* Boss = BossPtr.Get();
	Unbind();
	if (Boss)
	{
		Boss->StopPattern();
	}
	return EBTNodeResult::Aborted;
}

void UBTTask_StormBossAttack::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	Unbind();
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

void UBTTask_StormBossAttack::HandleAttackEnded(bool bInterrupted)
{
	UBehaviorTreeComponent* OwnerComp = OwnerCompPtr.Get();
	Unbind();
	if (OwnerComp)
	{
		FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
	}
}

void UBTTask_StormBossAttack::Unbind()
{
	if (AStormBossCharacter* Boss = BossPtr.Get())
	{
		Boss->OnAttackEnded.Remove(AttackEndedHandle);
	}
	AttackEndedHandle.Reset();
	BossPtr.Reset();
	OwnerCompPtr.Reset();
}
