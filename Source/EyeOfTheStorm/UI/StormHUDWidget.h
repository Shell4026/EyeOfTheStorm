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
private:
	UFUNCTION()
	void HandleBossDeath(AActor* Killer);
	void HideBoss();
	void SetBossVisible(bool bVisible);
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

	// 보스가 죽고 보스 표시를 숨길 때까지 시간
	UPROPERTY(EditAnywhere, Category = "Storm|UI", meta = (ClampMin = "0"))
	float BossHideDelay = 3.f;
private:
	TWeakObjectPtr<AStormBossCharacter> CurrentBoss;
	FTimerHandle BossHideTimer;
};
