#include "Character/StormTargetDummy.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

AStormTargetDummy::AStormTargetDummy()
{
	PrimaryActorTick.bCanEverTick = false;
	// 메시 오프셋 -96과 맞춘다 (ACharacter 기본 캡슐은 반높이 88).
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -96.f), FRotator(0.f, -90.f, 0.f));
	// AI 컨트롤러 없이 서 있기만 한다.
	AutoPossessAI = EAutoPossessAI::Disabled;
}
