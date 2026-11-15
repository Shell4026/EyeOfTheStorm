#include "Character/StormTargetDummy.h"

#include "Components/SkeletalMeshComponent.h"

AStormTargetDummy::AStormTargetDummy()
{
	PrimaryActorTick.bCanEverTick = false;
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -96.f), FRotator(0.f, -90.f, 0.f));
	// AI 컨트롤러 없이 서 있기만 한다.
	AutoPossessAI = EAutoPossessAI::Disabled;
}
