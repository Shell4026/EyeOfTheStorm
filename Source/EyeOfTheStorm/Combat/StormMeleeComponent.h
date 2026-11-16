#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StormMeleeComponent.generated.h"

class USkeletalMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FStormMeleeHitSignature, AActor*, Target, float, Damage, FVector, ImpactPoint);

// 판정 구간 하나의 모양과 효과. AnimNotifyState_StormHitWindow가 몽타주에서 넘겨준다.
USTRUCT(BlueprintType)
struct EYEOFTHESTORM_API FStormHitWindowSettings
{
	GENERATED_BODY()

	// Right-knee (판정 캡슐의 한쪽 끝 본 또는 소켓)
	UPROPERTY(EditAnywhere, Category = "Storm|Melee")
	FName StartBone = TEXT("Right-knee");
	// Right-toe (판정 캡슐의 다른 쪽 끝 본 또는 소켓)
	UPROPERTY(EditAnywhere, Category = "Storm|Melee")
	FName EndBone = TEXT("Right-toe");
	// 15cm (판정 구의 반지름)
	UPROPERTY(EditAnywhere, Category = "Storm|Melee", meta = (ClampMin = "1"))
	float Radius = 15.f;
	// 10 (피해량)
	UPROPERTY(EditAnywhere, Category = "Storm|Melee", meta = (ClampMin = "0"))
	float Damage = 10.f;
	// 0.08초 (맞았을 때 공격자와 대상이 거의 멈추는 시간. 0이면 히트스톱 없음)
	UPROPERTY(EditAnywhere, Category = "Storm|Melee", meta = (ClampMin = "0"))
	float HitStopDuration = 0.08f;
};

UCLASS(ClassGroup = (Storm), meta = (BlueprintSpawnableComponent))
class EYEOFTHESTORM_API UStormMeleeComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UStormMeleeComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Source는 구간을 연 노티파이. 같은 Source의 EndHitWindow만 구간을 닫는다 (이전 몽타주의 늦은 노티파이가 새 구간을 닫지 않게).
	void BeginHitWindow(const UObject* Source, USkeletalMeshComponent* Mesh, const FStormHitWindowSettings& Settings);
	void EndHitWindow(const UObject* Source);
	// 누가 열었든 지금 구간을 닫는다 (공격이 회피·사망으로 끊길 때).
	void CancelHitWindow();

	UFUNCTION(BlueprintPure, Category = "Storm|Melee")
	bool IsHitWindowActive() const { return ActiveSource.IsValid(); }
private:
	// 판정 캡슐을 따라 놓은 구들을 지난 프레임 위치에서 지금 위치로 쓸어 맞은 대상을 찾는다.
	void SweepHitWindow();
	void ApplyHit(AActor* Target, const FHitResult& Hit);
	void StartHitStop(AActor* Target, float Duration);
	void EndHitStop();

	// StartBone ~ EndBone 사이에 반지름 간격으로 놓은 구의 중심들
	void GetSamplePoints(TArray<FVector>& OutPoints) const;
public:
	// 대상에게 피해가 실제로 들어갔을 때. 소리·이펙트는 블루프린트에서 여기에 붙인다.
	UPROPERTY(BlueprintAssignable, Category = "Storm|Melee")
	FStormMeleeHitSignature OnHit;
protected:
	// 0.05배 (히트스톱 동안 공격자와 대상의 시간 배율. 0이면 완전히 멈춘다)
	UPROPERTY(EditAnywhere, Category = "Storm|Melee", meta = (ClampMin = "0", ClampMax = "1"))
	float HitStopTimeDilation = 0.05f;
	// 8개 (판정 캡슐을 따라 놓는 구의 최대 개수)
	UPROPERTY(EditAnywhere, Category = "Storm|Melee", meta = (ClampMin = "2", ClampMax = "16"))
	int32 MaxSamplePoints = 8;
private:
	TWeakObjectPtr<const UObject> ActiveSource;
	TWeakObjectPtr<USkeletalMeshComponent> ActiveMesh;
	FStormHitWindowSettings ActiveSettings;
	// 지난 프레임의 구 중심들. 이번 프레임 위치까지 쓸어서 빠른 발차기가 대상을 건너뛰지 않게 한다.
	TArray<FVector> PrevPoints;
	// 이번 구간에서 이미 맞은 대상 (한 번 휘두를 때 같은 대상은 한 번만)
	TArray<TWeakObjectPtr<AActor>> HitActors;
	// 히트스톱으로 시간 배율을 바꿔 둔 액터 (끝나면 1로 되돌린다)
	TArray<TWeakObjectPtr<AActor>> HitStopActors;

	FTimerHandle HitStopTimer;
};
