#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "StormGameMode.generated.h"

UCLASS()
class EYEOFTHESTORM_API AStormGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	// 플레이어가 죽으면 부른다. RestartDelay 뒤 지금 레벨을 처음부터 다시 연다
	void HandlePlayerDeath(APawn* Player);
private:
	void RestartLevel();
protected:
	// 패배 문구가 떠 있는 시간이기도 하다
	UPROPERTY(EditDefaultsOnly, Category = "Storm|Rules", meta = (ClampMin = "0"))
	float RestartDelay = 4.f;
private:
	FTimerHandle RestartTimer;
};
