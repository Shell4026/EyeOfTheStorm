#include "VWParameterMapper.h"

FVWRenderSettings VWParameterMapper::ConvertRenderSetting(const FVWArtistSetting& Artist, const FVWQualityOptions& Quality)
{
	FVWRenderSettings Setting;

	FVWAtmosphereSettings& Atmo = Setting.Atmosphere;
	Atmo.AtmosphereRadius = Atmo.GroundRadius + 100'000.f * Artist.AtmosphereThickness;
	Atmo.RayleighColor *= FMath::Clamp(Artist.RayleighScatteringStrength, 0.f, 4.f);
	Atmo.MieCoefficient = Artist.MieScatteringStrength;
	Atmo.MieG = FMath::Clamp(Artist.MieAnisotropy, 0.f, 0.99f);
	Atmo.MieColor = FVector3f(3.996f) * FVector3f(Artist.MieColor.R, Artist.MieColor.G, Artist.MieColor.B);

	Setting.Lighting.SunDirW = GetSunDirection(Artist);
	Setting.Lighting.SunIlluminance = 15.f * Artist.SunIntensity;

	Setting.Cloud = ConvertCloudSetting(Artist);
	Setting.Quality = ConvertQuality(Quality);

	Setting.Cloud.MinSteps = FMath::Max(Quality.CloudMinSteps, 1);
	Setting.Cloud.MaxSteps = FMath::Max(Quality.CloudMaxSteps, Quality.CloudMinSteps);
	Setting.Cloud.LightViewSteps = FMath::Max(Quality.CloudLightViewSteps, 1);
	Setting.Cloud.MaxMarchDistance = Quality.CloudRayMarchMaxDistance;
	Setting.Cloud.ModeFlags = Quality.bLightViewDistanceLimit ? 1u : 0u;
	return Setting;
}

FVWCloudSettings VWParameterMapper::ConvertCloudSetting(const FVWArtistSetting& Artist)
{
	FVWCloudSettings Result;
	const float Amount = FMath::Clamp(Artist.CloudAmount, 0.f, 1.f);
	const float SizeKM = FMath::Clamp(Artist.CloudSizeKM, 10.f, 100.f);
	const float Detail = FMath::Clamp(Artist.CloudDetail, 0.f, 1.f);
	const float Density = FMath::Clamp(Artist.CloudDensity, 0.f, 1.f);

	Result.Coverage = FMath::Lerp(0.f, 0.2f, Amount);
	Result.Tiling = SizeKM * 1'000.f;
	Result.Tiling2 = FMath::Lerp(Result.Tiling / 10.f, Result.Tiling / 40.f, Detail);
	Result.ExtinctionCoefficient = FMath::Lerp(10.f, 500.f, Density * Density);
	Result.AnvilBias = FMath::Lerp(0.2f, 0.8f, (Artist.CloudVerticalAmount + 1.f) / 2.f); // -1~1 -> 0.2~0.8
	Result.Brightness = FMath::Clamp(Artist.CloudBrightness, 0.f, 4.f);

	const FRichCurve* Curve = Artist.CloudBrightnessCurve.GetRichCurveConst();
	for (int i = 0; i < FVWCloudSettings::CloudCurveSamples; ++i)
	{
		const float X = i / static_cast<float>(FVWCloudSettings::CloudCurveSamples - 1);
		Result.BrightnessCurve[i / 4][i % 4] = Curve->Eval(X);
	}

	Result.DensityPMin = FMath::Lerp(0.f, 0.2f, Artist.CloudSoftness);
	Result.DensityPFactor = FMath::Lerp(0.f, 1.5f, Artist.CloudSoftness);
	Result.CloudColor = Artist.CloudColor;

	const float DirectionRadians = FMath::DegreesToRadians(Artist.WindDirectionDegrees);
	Result.WindVelKmh = FVector2f(FMath::Cos(DirectionRadians), FMath::Sin(DirectionRadians)) * Artist.WindSpeedKmh;
	return Result;
}

FVWQualitySettings VWParameterMapper::ConvertQuality(const FVWQualityOptions& Quality)
{
	FVWQualitySettings Result;
	Result.TransmittanceSteps = FMath::Max(Quality.TransmittanceSteps, 1);
	Result.MultiScatteringSteps = FMath::Max(Quality.MultiScatteringSteps, 1);
	Result.SkyViewSteps = FMath::Max(Quality.SkyViewSteps, 1);
	Result.AerialPerspectiveSteps = FMath::Max(Quality.AerialPerspectiveSteps, 1);
	Result.AerialShadowSteps = FMath::Max(Quality.AerialShadowSteps, 1);
	Result.APDistanceFactor = Quality.APDistanceFactor;
	Result.bAerialPerspective = Quality.bAerialPerspective;
	Result.bVolumetricShadow = Quality.bVolumetricShadow;
	Result.bCloud = Quality.bCloud;
	Result.HistoryWeight = Quality.HistoryWeight;
	Result.NeighborhoodGamma = Quality.NeighborhoodGamma;
	return Result;
}

FVector3f VWParameterMapper::GetSunDirection(const FVWArtistSetting& Artist)
{
	// 주의: 여기의 FVector는 전부 Weather 벡터다. UE 월드 좌표가 아니다.
	const double PlanetDir = FMath::DegreesToRadians(static_cast<double>(Artist.PlanetRotationAxis));
	const double Latitude = FMath::DegreesToRadians(static_cast<double>(Artist.Latitude));
	const double YearRadians = (Artist.Month - 1.0) * (2.0 * UE_DOUBLE_PI / 12.0);
	const double NoonPhase = FMath::Atan2(-FMath::Sin(YearRadians), FMath::Cos(PlanetDir) * FMath::Cos(YearRadians));
	const double HourRadians = (Artist.Hour - 12.0) * (2.0 * UE_DOUBLE_PI / 24.0) + NoonPhase;

	FVector PlayerPos(FMath::Cos(Latitude), FMath::Sin(Latitude), 0.0);

	const FQuat PlanetRotQ(FVector(0.0, 0.0, 1.0), PlanetDir);
	const FVector PlanetUp = PlanetRotQ.RotateVector(FVector(0.0, 1.0, 0.0)).GetSafeNormal();
	PlayerPos = PlanetRotQ.RotateVector(PlayerPos).GetSafeNormal();

	const FQuat TimeQ(PlanetUp, HourRadians);
	PlayerPos = TimeQ.RotateVector(PlayerPos).GetSafeNormal();

	const FVector SunDir(FMath::Cos(YearRadians), 0.0, FMath::Sin(YearRadians));

	const FVector LocalUp = PlayerPos;
	const FVector LocalEast = FVector::CrossProduct(PlanetUp, LocalUp).GetSafeNormal();
	const FVector LocalNorth = FVector::CrossProduct(LocalUp, LocalEast).GetSafeNormal();

	const FVector LocalSunDir(
		FVector::DotProduct(SunDir, LocalEast),
		FVector::DotProduct(SunDir, LocalUp),
		FVector::DotProduct(SunDir, LocalNorth));

	return FVector3f(-LocalSunDir.GetSafeNormal());
}

FLinearColor VWParameterMapper::ComputeSunTransmittance(const FVWAtmosphereSettings& Atmo, const FVector3f& SunDirW, int32 Steps)
{
	const double Mu = -SunDirW.Y;
	const double Height = 1.0;

	const double SampleDis = Atmo.GroundRadius + Height;
	const double RayProj = SampleDis * Mu;
	const double D2 = SampleDis * SampleDis - RayProj * RayProj;
	const double R2 = static_cast<double>(Atmo.AtmosphereRadius) * Atmo.AtmosphereRadius;
	const double SampleToAtmosphere = FMath::Sqrt(FMath::Max(R2 - D2, 0.0)) - RayProj;
	if (SampleToAtmosphere <= 0.0)
	{
		return FLinearColor::White;
	}

	const double StepSize = SampleToAtmosphere / Steps;
	const FVector Rayleigh = FVector(Atmo.RayleighColor) * 1e-6;
	const FVector Mie = FVector(Atmo.MieColor) * 1e-6 * Atmo.MieCoefficient;
	const FVector MieAbsorption = FVector(4.4e-6 * Atmo.MieCoefficient);
	const FVector Ozone(0.65e-6, 1.881e-6, 0.085e-6);

	FVector SigmaSum = FVector::ZeroVector;
	for (int32 i = 0; i < Steps; ++i)
	{
		const double T = (i + 0.5) * StepSize;
		const double SampleRadius = FMath::Sqrt(D2 + (RayProj + T) * (RayProj + T));
		const double H = SampleRadius - Atmo.GroundRadius;
		if (H < 0.0)
		{
			return FLinearColor::Black;
		}
		const double RayleighDensity = FMath::Exp(-H / 8000.0);
		const double MieDensity = FMath::Exp(-H / 1200.0);
		const double OzoneDensity = FMath::Max(0.0, 1.0 - FMath::Abs(H - 25000.0) / 15000.0);
		SigmaSum += Rayleigh * RayleighDensity + (Mie + MieAbsorption) * MieDensity + Ozone * OzoneDensity;
	}

	const FVector T(FMath::Exp(-SigmaSum.X * StepSize), FMath::Exp(-SigmaSum.Y * StepSize), FMath::Exp(-SigmaSum.Z * StepSize));
	return FLinearColor(T.X, T.Y, T.Z);
}
