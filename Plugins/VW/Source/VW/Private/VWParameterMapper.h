#pragma once
#include "VWArtistSetting.h"
#include "VWRenderSettings.h"
#include "CoreMinimal.h"

namespace VWParameterMapper
{
	FVWRenderSettings ConvertRenderSetting(const FVWArtistSetting& Artist, const FVWQualityOptions& Quality);
	FVWCloudSettings ConvertCloudSetting(const FVWArtistSetting& Artist);
	FVWQualitySettings ConvertQuality(const FVWQualityOptions& Quality);

	FVector3f GetSunDirection(const FVWArtistSetting& Artist);
	FLinearColor ComputeSunTransmittance(const FVWAtmosphereSettings& Atmosphere, const FVector3f& SunDirW, int32 Steps = 64);
}
