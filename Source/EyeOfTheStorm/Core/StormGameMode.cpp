#include "Core/StormGameMode.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogStormGameMode, Log, All);

void AStormGameMode::HandlePlayerDeath(APawn* Player)
{
	if (GetWorldTimerManager().IsTimerActive(RestartTimer))
	{
		return;
	}
	UE_LOG(LogStormGameMode, Log, TEXT("%s 사망, %.1f초 뒤 다시 시작"), *GetNameSafe(Player), RestartDelay);
	GetWorldTimerManager().SetTimer(RestartTimer, this, &AStormGameMode::RestartLevel, FMath::Max(RestartDelay, 0.01f), false);
}

void AStormGameMode::RestartLevel()
{
	// PIE에서는 패키지 이름에 UEDPIE_0_ 접두사가 붙으므로 떼고 연다. 레벨을 새로 열어야 보스 체력, 날씨, 낙뢰가 모두 처음으로 돌아간다
	const FString LevelName = UWorld::RemovePIEPrefix(GetWorld()->GetPackage()->GetName());
	UGameplayStatics::OpenLevel(this, FName(*LevelName));
}
