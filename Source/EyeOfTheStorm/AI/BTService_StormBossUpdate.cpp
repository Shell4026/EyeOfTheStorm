#include "AI/BTService_StormBossUpdate.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Character/StormBossCharacter.h"
#include "Kismet/GameplayStatics.h"

UBTService_StormBossUpdate::UBTService_StormBossUpdate()
{
	NodeName = TEXT("Storm Boss Update");
	Interval = 0.1f;
	RandomDeviation = 0.f;

	TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_StormBossUpdate, TargetKey), AActor::StaticClass());
	HasPatternKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_StormBossUpdate, HasPatternKey));
}

void UBTService_StormBossUpdate::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* Blackboard = GetBlackboardAsset())
	{
		TargetKey.ResolveSelectedKey(*Blackboard);
		HasPatternKey.ResolveSelectedKey(*Blackboard);
	}
}

void UBTService_StormBossUpdate::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AI = OwnerComp.GetAIOwner();
	AStormBossCharacter* Boss = AI ? Cast<AStormBossCharacter>(AI->GetPawn()) : nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Boss || !Blackboard)
	{
		return;
	}

	AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(TargetKey.SelectedKeyName));
	if (!Target)
	{
		Target = UGameplayStatics::GetPlayerPawn(Boss, 0);
	}
	if (const AStormCharacterBase* StormTarget = Cast<AStormCharacterBase>(Target); StormTarget && StormTarget->IsDead())
	{
		Target = nullptr;
	}

	Blackboard->SetValueAsObject(TargetKey.SelectedKeyName, Target);
	if (Boss->GetAttackTarget() != Target)
	{
		Boss->SetCombatTarget(Target);
		if (Target)
		{
			AI->SetFocus(Target);
		}
		else
		{
			AI->ClearFocus(EAIFocusPriority::Gameplay);
		}
	}

	Blackboard->SetValueAsBool(HasPatternKey.SelectedKeyName, Boss->HasAvailablePattern());
}
