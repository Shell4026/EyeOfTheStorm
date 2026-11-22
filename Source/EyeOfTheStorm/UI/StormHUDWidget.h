#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StormHUDWidget.generated.h"

class AStormBossCharacter;
class AStormCharacterBase;
class UStormHealthBarWidget;
class UTextBlock;

UCLASS(Abstract)
class EYEOFTHESTORM_API UStormHUDWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetPlayer(AStormCharacterBase* Player);
	// 보스가 플레이어를 전투 대상으로 잡으면 부른다. 보스 이름과 체력바를 보여 준다.
	void SetBoss(AStormBossCharacter* Boss);
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	// 승리나 패배 문구를 띄운 직후 부른다. 페이드 같은 연출은 위젯 블루프린트에서 한다
	UFUNCTION(BlueprintImplementableEvent, Category = "Storm|UI")
	void OnResultShown(bool bVictory);
private:
	UFUNCTION()
	void HandleBossDeath(AActor* Killer);
	UFUNCTION()
	void HandlePlayerDeath(AActor* Killer);
	void HideBoss();
	void SetBossVisible(bool bVisible);
	// 먼저 난 결과만 띄운다. 보스를 쓰러뜨린 뒤 낙사해도 승리 문구가 남는다
	void ShowResult(bool bVictory);
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UStormHealthBarWidget> PlayerHealthBar;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UStormHealthBarWidget> BossHealthBar;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BossNameText;
	// 있으면 보스 이름과 체력바를 이 위젯째로 숨기고 보인다. 없으면 둘을 각각 숨긴다
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> BossPanel;
	// 승리나 패배 문구. 없으면 문구 없이 게임 모드가 다시 시작만 한다
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ResultText;

	// 보스가 죽고 보스 표시를 숨길 때까지 시간
	UPROPERTY(EditAnywhere, Category = "Storm|UI", meta = (ClampMin = "0"))
	float BossHideDelay = 3.f;

	// 결과

	UPROPERTY(EditAnywhere, Category = "Storm|UI")
	FText VictoryText = NSLOCTEXT("StormHUD", "Victory", "폭풍이 잦아들었다");
	UPROPERTY(EditAnywhere, Category = "Storm|UI")
	FText DefeatText = NSLOCTEXT("StormHUD", "Defeat", "쓰러졌다");
private:
	TWeakObjectPtr<AStormBossCharacter> CurrentBoss;
	TWeakObjectPtr<AStormCharacterBase> CurrentPlayer;
	FTimerHandle BossHideTimer;
	bool bResultShown = false;
};
