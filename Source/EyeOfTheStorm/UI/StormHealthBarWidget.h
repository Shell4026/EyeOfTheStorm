#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StormHealthBarWidget.generated.h"

class UProgressBar;
class UStormHealthComponent;

UCLASS(Abstract)
class EYEOFTHESTORM_API UStormHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	// 이 체력 컴포넌트를 따라간다. nullptr이면 연결만 끊는다.
	void SetHealthComponent(UStormHealthComponent* InHealth);
protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;
private:
	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float MaxHealth);
	void ApplyRatio(float Ratio);
protected:
	// 위젯

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;
	// 있으면 HealthBar 뒤에서 잠시 멈췄다가 따라 줄어드는 잔상 바. HealthBar보다 아래 레이어에 둔다
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> TrailBar;

	// 잔상

	// 마지막으로 맞은 뒤 잔상 바가 줄어들기 시작할 때까지 시간. 연속으로 맞으면 다시 센다
	UPROPERTY(EditAnywhere, Category = "Storm|UI", meta = (ClampMin = "0"))
	float TrailDelay = 0.5f;
	// 잔상 바가 줄어드는 속도 (전체 체력에 대한 비율, 초당)
	UPROPERTY(EditAnywhere, Category = "Storm|UI", meta = (ClampMin = "0.01"))
	float TrailSpeed = 0.6f;
private:
	TWeakObjectPtr<UStormHealthComponent> Health;
	float HealthRatio = 1.f;
	float TrailRatio = 1.f;
	float TrailWaitRemaining = 0.f;
};
