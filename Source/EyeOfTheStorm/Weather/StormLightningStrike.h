#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StormLightningStrike.generated.h"

class UDecalComponent;
class UMaterialInterface;
class UPointLightComponent;

// 낙뢰 한 번의 설정. UStormLightningComponent가 채워서 넘긴다
struct FStormStrikeSettings
{
	// 0이면 예고 없이 바로 번쩍인다 (하늘만 번쩍이는 플래시)
	float WarningTime = 0.f;
	float Radius = 0.f;
	float Damage = 0.f;
	// 비우면 예고 장판을 그리지 않는다
	UMaterialInterface* TelegraphMaterial = nullptr;
	// 장판 중심에서 위아래로 이 거리 안의 바닥에 그린다
	float TelegraphDepth = 300.f;
	// 피해를 받을 대상. 비우면 플래시만
	TWeakObjectPtr<AActor> Target;
	// 피해를 준 쪽 (보스). 사망 이벤트의 Killer가 된다
	TWeakObjectPtr<AActor> Causer;
	float FlashIntensity = 0.f;
	float FlashRadius = 0.f;
	FLinearColor FlashColor = FLinearColor::White;
	// 바닥에서 플래시 조명까지의 높이
	float FlashHeight = 300.f;
};

UCLASS(NotBlueprintable)
class EYEOFTHESTORM_API AStormLightningStrike : public AActor
{
	GENERATED_BODY()
public:
	AStormLightningStrike();

	virtual void Tick(float DeltaSeconds) override;

	// 액터 위치(바닥)에 WarningTime 동안 예고 장판을 그린 뒤 번쩍이며 Radius 안의 Target에 피해를 준다. 플래시가 끝나면 스스로 없어진다
	void Start(const FStormStrikeSettings& InSettings);
private:
	void Strike();
	// 번쩍인 뒤 지난 시간에 따른 조명 밝기 비율. 두 번 번쩍이고 사라진다
	static float GetFlashAlpha(float Time);
private:
	UPROPERTY(VisibleAnywhere, Category = "Storm|Lightning")
	TObjectPtr<UDecalComponent> Telegraph;
	UPROPERTY(VisibleAnywhere, Category = "Storm|Lightning")
	TObjectPtr<UPointLightComponent> FlashLight;

	FStormStrikeSettings Settings;
	float Elapsed = 0.f;
	bool bStruck = false;
};
