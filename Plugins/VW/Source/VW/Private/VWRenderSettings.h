#pragma once
#include "CoreMinimal.h"

namespace VWSpace
{
	inline FVector3f UEToWeatherDir(const FVector& D) { return FVector3f(FVector(D.X, D.Z, D.Y)); }
	inline FVector3f UEToWeatherPos(const FVector& P) { return FVector3f(FVector(P.X, P.Z, P.Y) * 0.01); }
	inline FVector WeatherToUEDir(const FVector3f& D) { return FVector(D.X, D.Z, D.Y); }
	inline FVector WeatherToUEPos(const FVector3f& P) { return FVector(P.X, P.Z, P.Y) * 100.0; }
}

struct FVWAtmosphereSettings
{
	float GroundRadius = 6'360'000.f; // m
	float AtmosphereRadius = 6'460'000.f; // m
	float MieCoefficient = 1.f;
	float MieG = 0.8f;
	FVector3f MieColor{ 3.996f, 3.996f, 3.996f };
	FVector3f RayleighColor{ 5.802f, 13.558f, 33.1f };
};

struct FVWLightingSettings
{
	FVector3f SunDirW{ 0.f, -1.f, 0.f }; // 빛이 진행하는 방향 (Weather 공간)
	float SunIlluminance = 15.f;
};

struct FVWCloudSettings
{
	uint32 MinSteps = 10;
	uint32 MaxSteps = 64;
	uint32 LightViewSteps = 6;
	uint32 ModeFlags = 1;

	float Tiling = 90'000.f;
	float Tiling2 = 3'000.f;
	float ExtinctionCoefficient = 200.f;
	float Coverage = 0.05f;
	float AnvilBias = 0.f;
	float Brightness = 1.f;
	float DensityPMin = 0.f;
	float DensityPFactor = 1.25f;
	float MaxMarchDistance = 50'000.f;

	static constexpr int CloudCurveSamples = 32;
	FVector4f BrightnessCurve[CloudCurveSamples / 4];

	FVector2f WindVelKmh{ 1.f, 0.f };
	FLinearColor CloudColor = FLinearColor::White;

	float TimeHours = 0.f;
};

struct FVWQualitySettings
{
	uint32 TransmittanceSteps = 64;
	uint32 MultiScatteringSteps = 64;
	uint32 SkyViewSteps = 64;
	uint32 AerialPerspectiveSteps = 5;
	uint32 AerialShadowSteps = 40;
	float APDistanceFactor = 3.2f;

	bool bAerialPerspective = true;
	bool bVolumetricShadow = true;
	bool bCloud = true;

	float HistoryWeight = 0.95f;
	float NeighborhoodGamma = 2.5f;
};

struct FVWRenderSettings
{
	FVWAtmosphereSettings Atmosphere;
	FVWLightingSettings Lighting;
	FVWCloudSettings Cloud;
	FVWQualitySettings Quality;
};