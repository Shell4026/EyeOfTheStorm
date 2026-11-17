#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_StormRadialHit.generated.h"

UCLASS(meta = (DisplayName = "Storm Radial Hit"))
class EYEOFTHESTORM_API UAnimNotify_StormRadialHit : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
public:
	// 비우면 액터 위치(캡슐 중심)가 기준이다.
	UPROPERTY(EditAnywhere, Category = "Storm")
	FName CenterBone;
	// 기준 위치에 더한다. 액터가 보는 방향 기준 (X가 앞)
	UPROPERTY(EditAnywhere, Category = "Storm")
	FVector Offset = FVector(150.f, 0.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Storm", meta = (ClampMin = "1"))
	float Radius = 300.f;
	UPROPERTY(EditAnywhere, Category = "Storm", meta = (ClampMin = "0"))
	float Damage = 25.f;
	// 0이면 히트스톱 없음
	UPROPERTY(EditAnywhere, Category = "Storm", meta = (ClampMin = "0"))
	float HitStopDuration = 0.1f;
};
