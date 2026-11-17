#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "StormBossAIController.generated.h"

UCLASS()
class EYEOFTHESTORM_API AStormBossAIController : public AAIController
{
	GENERATED_BODY()
protected:
	virtual void OnPossess(APawn* InPawn) override;
};
