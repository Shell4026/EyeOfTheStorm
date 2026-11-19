#include "Weather/StormWeatherPreset.h"

float UStormWeatherPreset::GetFogDensity() const
{
	// UE 높이 안개의 소산계수는 안개 높이에서 FogDensity / 1000 (1/cm) = FogDensity / 10 (1/m) (SceneCore.cpp FExponentialHeightFogSceneInfo).
	// Koschmieder 가시거리 V = 3.912 / 소산계수 이므로 FogDensity = 39.12 / V(m)
	return FogVisibilityMeters > 0.f ? 39.12f / FogVisibilityMeters : 0.f;
}
