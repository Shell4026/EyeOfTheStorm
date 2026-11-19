// Fill out your copyright notice in the Description page of Project Settings.


#include "VWActor.h"
#include "VWSubSystem.h"
#include "VWParameterMapper.h"
#include "VWRenderer.h"

#include "Components/BillboardComponent.h"
#include "Components/LightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/World.h"
#include "Engine/DirectionalLight.h"
#include "Engine/TextureRenderTarget2D.h"
// Sets default values
AVWActor::AVWActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	ShadowCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("ShadowCapture"));
	ShadowCapture->SetupAttachment(RootComponent);
	ShadowCapture->SetUsingAbsoluteLocation(true); // 액터를 따라 움직이지 않음
	ShadowCapture->SetUsingAbsoluteRotation(true);
	ShadowCapture->ProjectionType = ECameraProjectionMode::Orthographic;
	ShadowCapture->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;
	ShadowCapture->bCaptureEveryFrame = false; // 태양이 움직일 때만 직접 CaptureScene() 호출
	ShadowCapture->bCaptureOnMovement = false;
	ShadowCapture->bAutoCalculateOrthoPlanes = false;

#if WITH_EDITORONLY_DATA
	Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (Sprite)
		Sprite->SetupAttachment(RootComponent);
#endif
}

void AVWActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	EnsureShadowTarget();
	bDirty = true;
	bShadowDirty = true;
}

void AVWActor::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	UWorld* const WorldPtr = GetWorld();
	if (UVWSubSystem* SubSystem = WorldPtr ? WorldPtr->GetSubsystem<UVWSubSystem>() : nullptr)
	{
		SubSystem->RegisterWeatherActor(this);
		SubSystem->PushBlueNoise(BlueNoise);

		bDirty = true;
		bHasPushed = false;
		PushToRenderer();
	}
}

void AVWActor::PostUnregisterAllComponents()
{
	UWorld* const WorldPtr = GetWorld();
	if (UVWSubSystem* SubSystem = WorldPtr ? WorldPtr->GetSubsystem<UVWSubSystem>() : nullptr)
		SubSystem->UnRegisterWeatherActor(this);

	Super::PostUnregisterAllComponents();
}

#if WITH_EDITOR
void AVWActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetMemberPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AVWActor, ShadowMapResolution) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AVWActor, ShadowAreaKm) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AVWActor, ShadowDepthBiasCm))
	{
		EnsureShadowTarget();
		bShadowDirty = true;
	}
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AVWActor, BlueNoise))
	{
		if (UVWSubSystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UVWSubSystem>() : nullptr)
			Subsystem->PushBlueNoise(BlueNoise);
	}

	bDirty = true;
	PushToRenderer();
}
#endif

void AVWActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (Artist.TimeSpeed > 0.f)
	{
		Artist.Hour = FMath::Fmod(Artist.Hour + Artist.TimeSpeed * DeltaTime, 24.f);
		CloudTimeHours += Artist.TimeSpeed * DeltaTime;
		bDirty = true;
	}

	if (bDirty)
		PushToRenderer();
}
void AVWActor::BeginPlay()
{
	Super::BeginPlay();
}

void AVWActor::EnsureShadowTarget()
{
	if (ShadowDepthTarget && ShadowDepthTarget->SizeX == ShadowMapResolution)
		return;

	ShadowDepthTarget = NewObject<UTextureRenderTarget2D>(this, NAME_None, EObjectFlags::RF_Transient);
	ShadowDepthTarget->InitCustomFormat(ShadowMapResolution, ShadowMapResolution, EPixelFormat::PF_R32_FLOAT, true);
	ShadowDepthTarget->UpdateResourceImmediate(true);
	
	ShadowCapture->TextureTarget = ShadowDepthTarget;

	bShadowDirty = true;
}

void AVWActor::UpdateSunLight(const FVector& SunDirUE, float SunIlluminance, const FLinearColor& SunTransmittance)
{
	if (!SunLight)
		return;

	SunLight->SetActorRotation(SunDirUE.Rotation());
	if (ULightComponent* Light = SunLight->GetLightComponent())
	{
		const float MaxT = SunTransmittance.GetMax();
		Light->SetIntensity(SunIlluminance * SunLightIntensityScale * MaxT);
		if (MaxT > UE_KINDA_SMALL_NUMBER)
		{
			Light->SetLightColor(SunTransmittance / MaxT);
		}
	}
}

void AVWActor::UpdateShadowCapture(const FVector& SunDirUE)
{
	UWorld* const World = GetWorld();
	UVWSubSystem* const Subsystem = World ? World->GetSubsystem<UVWSubSystem>() : nullptr;
	if (!Subsystem || !ShadowCapture)
		return;

	EnsureShadowTarget();

	constexpr double CaptureDistanceCm = 1'000'000.0; // 10km
	const float HalfWidth = ShadowAreaKm * 100'000.f * 0.5f;
	const float FarPlane = CaptureDistanceCm * 2.0;
	const FVector Center = GetActorLocation();
	
	ShadowCapture->bUseCustomProjectionMatrix = true;
	ShadowCapture->CustomProjectionMatrix = FReversedZOrthoMatrix(HalfWidth, HalfWidth, 1.f / FarPlane, 0.f);
	ShadowCapture->SetWorldLocationAndRotation(Center - SunDirUE * CaptureDistanceCm, SunDirUE.Rotation());
	ShadowCapture->OrthoWidth = ShadowAreaKm * 100'000.f;

	if (SunDirUE.Z < 0.0)
		ShadowCapture->CaptureScene();

	FVWShadowCaptureInfo Info;
	Info.OriginUE = FVector3f(ShadowCapture->GetComponentLocation());
	Info.Forward = FVector3f(ShadowCapture->GetForwardVector());
	Info.Right = FVector3f(ShadowCapture->GetRightVector());
	Info.Up = FVector3f(ShadowCapture->GetUpVector());
	Info.OrthoWidth = ShadowCapture->OrthoWidth;
	Info.DepthBias = ShadowDepthBiasCm;

	Subsystem->PushShadowCapture(ShadowDepthTarget, Info);
}

void AVWActor::PushToRenderer()
{
	UWorld* const WorldPtr = GetWorld();
	UVWSubSystem* const SubSystem = WorldPtr ? WorldPtr->GetSubsystem<UVWSubSystem>() : nullptr;
	if (SubSystem == nullptr)
		return;

	FVWRenderSettings Settings = VWParameterMapper::ConvertRenderSetting(Artist, Quality);
	Settings.Cloud.TimeHours = CloudTimeHours;

	const bool bStaticLUTDirty = !bHasPushed
		|| LastAtmosphereThickness != Artist.AtmosphereThickness
		|| LastRayleigh != Artist.RayleighScatteringStrength
		|| LastMie != Artist.MieScatteringStrength
		|| LastMieColor != Artist.MieColor
		|| LastTransmittanceSteps != Quality.TransmittanceSteps
		|| LastMultiScatteringSteps != Quality.MultiScatteringSteps;
	LastAtmosphereThickness = Artist.AtmosphereThickness;
	LastRayleigh = Artist.RayleighScatteringStrength;
	LastMie = Artist.MieScatteringStrength;
	LastMieColor = Artist.MieColor;
	LastTransmittanceSteps = Quality.TransmittanceSteps;
	LastMultiScatteringSteps = Quality.MultiScatteringSteps;

	SubSystem->PushSettings(Settings, bStaticLUTDirty);
	SubSystem->PushHelloTint(HelloTint);
	
	const FVector SunDirUE = VWSpace::WeatherToUEDir(Settings.Lighting.SunDirW);
	const FLinearColor SunTransmittance = VWParameterMapper::ComputeSunTransmittance(Settings.Atmosphere, Settings.Lighting.SunDirW);
	UpdateSunLight(SunDirUE, Settings.Lighting.SunIlluminance, SunTransmittance);

	if (bShadowDirty || !SunDirUE.Equals(LastSunDirUE, 1e-3))
	{
		UpdateShadowCapture(SunDirUE);
		LastSunDirUE = SunDirUE;
		bShadowDirty = false;
	}

	bDirty = false;
	bHasPushed = true;
}