// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "VWSubSystem.generated.h"

class FVWViewExtension;

UCLASS()
class UVWSubSystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
private:
	TSharedPtr<FVWViewExtension, ESPMode::ThreadSafe> ViewExtension;
};
