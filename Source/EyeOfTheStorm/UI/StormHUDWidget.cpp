#include "UI/StormHUDWidget.h"

#include "Character/StormBossCharacter.h"
#include "Combat/StormHealthComponent.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UI/StormHealthBarWidget.h"

void UStormHUDWidget::SetPlayer(AStormCharacterBase* Player)
{
	if (AStormCharacterBase* Old = CurrentPlayer.Get())
	{
		Old->GetHealthComponent()->OnDeath.RemoveDynamic(this, &UStormHUDWidget::HandlePlayerDeath);
	}
	CurrentPlayer = Player;
	if (Player)
	{
		Player->GetHealthComponent()->OnDeath.AddUniqueDynamic(this, &UStormHUDWidget::HandlePlayerDeath);
	}
	PlayerHealthBar->SetHealthComponent(Player ? Player->GetHealthComponent() : nullptr);
}

void UStormHUDWidget::SetBoss(AStormBossCharacter* Boss)
{
	if (CurrentBoss.Get() == Boss)
	{
		return;
	}
	if (AStormBossCharacter* Old = CurrentBoss.Get())
	{
		Old->GetHealthComponent()->OnDeath.RemoveDynamic(this, &UStormHUDWidget::HandleBossDeath);
	}
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BossHideTimer);
	}

	CurrentBoss = Boss;
	if (!Boss || Boss->IsDead())
	{
		HideBoss();
		return;
	}

	Boss->GetHealthComponent()->OnDeath.AddUniqueDynamic(this, &UStormHUDWidget::HandleBossDeath);
	BossHealthBar->SetHealthComponent(Boss->GetHealthComponent());
	if (BossNameText)
	{
		BossNameText->SetText(Boss->GetBossName());
	}
	SetBossVisible(true);
}

void UStormHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetBossVisible(false);
	if (ResultText)
	{
		ResultText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UStormHUDWidget::NativeDestruct()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BossHideTimer);
	}
	if (AStormBossCharacter* Boss = CurrentBoss.Get())
	{
		Boss->GetHealthComponent()->OnDeath.RemoveDynamic(this, &UStormHUDWidget::HandleBossDeath);
	}
	if (AStormCharacterBase* Player = CurrentPlayer.Get())
	{
		Player->GetHealthComponent()->OnDeath.RemoveDynamic(this, &UStormHUDWidget::HandlePlayerDeath);
	}
	Super::NativeDestruct();
}

void UStormHUDWidget::HandleBossDeath(AActor* Killer)
{
	ShowResult(true);

	// 마지막 일격으로 바가 0까지 줄어드는 걸 보여 준 뒤 숨긴다.
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(BossHideTimer, this, &UStormHUDWidget::HideBoss, FMath::Max(BossHideDelay, 0.01f), false);
	}
}

void UStormHUDWidget::HandlePlayerDeath(AActor* Killer)
{
	ShowResult(false);
}

void UStormHUDWidget::HideBoss()
{
	if (AStormBossCharacter* Boss = CurrentBoss.Get())
	{
		Boss->GetHealthComponent()->OnDeath.RemoveDynamic(this, &UStormHUDWidget::HandleBossDeath);
	}
	CurrentBoss.Reset();
	BossHealthBar->SetHealthComponent(nullptr);
	SetBossVisible(false);
}

void UStormHUDWidget::SetBossVisible(bool bVisible)
{
	const ESlateVisibility SVisibility = bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	if (BossPanel)
	{
		BossPanel->SetVisibility(SVisibility);
		return;
	}
	BossHealthBar->SetVisibility(SVisibility);
	if (BossNameText)
	{
		BossNameText->SetVisibility(SVisibility);
	}
}

void UStormHUDWidget::ShowResult(bool bVictory)
{
	if (bResultShown)
	{
		return;
	}
	bResultShown = true;
	if (ResultText)
	{
		ResultText->SetText(bVictory ? VictoryText : DefeatText);
		ResultText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	OnResultShown(bVictory);
}
