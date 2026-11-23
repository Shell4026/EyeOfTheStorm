#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "VWArtistSetting.generated.h"

USTRUCT(BlueprintType)
struct FVWArtistSetting
{
	GENERATED_BODY()

	FVWArtistSetting()
	{
		FRichCurve* Curve = CloudBrightnessCurve.GetRichCurve();
		Curve->AddKey(0.f, 0.f);
		Curve->AddKey(0.5f, 0.5f);
		Curve->AddKey(1.f, 1.f);
	}

	// 태양
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun", meta = (DisplayName = "밝기", ClampMin = "0", UIMax = "4"))
	float SunIntensity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun", meta = (DisplayName = "자전축 (도)", ClampMin = "0", ClampMax = "45"))
	float PlanetRotationAxis = 23.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun", meta = (DisplayName = "위도 (도)", ClampMin = "-90", ClampMax = "90"))
	float Latitude = 37.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun", meta = (DisplayName = "월", ClampMin = "1", ClampMax = "12"))
	float Month = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun", meta = (DisplayName = "시간", ClampMin = "0", ClampMax = "24"))
	float Hour = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun", meta = (DisplayName = "시간 속도 (시간/초)", ClampMin = "0", UIMax = "2"))
	float TimeSpeed = 0.f;

	// 대기
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere", meta = (DisplayName = "대기 두께", ClampMin = "0.1"))
	float AtmosphereThickness = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere", meta = (DisplayName = "레일리 산란 강도", ClampMin = "0"))
	float RayleighScatteringStrength = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere", meta = (DisplayName = "에어로졸 강도", ClampMin = "0"))
	float MieScatteringStrength = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere", meta = (DisplayName = "에어로졸 방향성", ClampMin = "0", ClampMax = "0.99"))
	float MieAnisotropy = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere", meta = (DisplayName = "에어로졸 색", HideAlphaChannel))
	FLinearColor MieColor = FLinearColor::White;

	// 구름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloud", meta = (DisplayName = "구름 색", HideAlphaChannel))
	FLinearColor CloudColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloud", meta = (DisplayName = "구름 양", ClampMin = "0", ClampMax = "1"))
	float CloudAmount = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloud", meta = (DisplayName = "수직 발달", ClampMin = "-1", ClampMax = "1"))
	float CloudVerticalAmount = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloud", meta = (DisplayName = "구름 크기 (km)", ClampMin = "10", ClampMax = "100"))
	float CloudSizeKM = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloud", meta = (DisplayName = "디테일", ClampMin = "0", ClampMax = "1"))
	float CloudDetail = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloud", meta = (DisplayName = "밀도", ClampMin = "0", ClampMax = "1"))
	float CloudDensity = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloud", meta = (DisplayName = "구름 밝기", ClampMin = "0", ClampMax = "4"))
	float CloudBrightness = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloud", 
		meta = (DisplayName = "높이별 밝기 커브", XAxisName = "높이 비율", YAxisName = "밝기", ViewMinInput = "0", ViewMaxInput = "1", ViewMinOutput = "0", ViewMaxOutput = "1"))
	FRuntimeFloatCurve CloudBrightnessCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloud", meta = (DisplayName = "파우더 효과", ClampMin = "0", ClampMax = "1"))
	float CloudSoftness = 0.5f;

	// 바람
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind", meta = (DisplayName = "풍향 (도)", ClampMin = "0", ClampMax = "360"))
	float WindDirectionDegrees = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind", meta = (DisplayName = "풍속 (km/h)", ClampMin = "0", ClampMax = "500"))
	float WindSpeedKmh = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind", meta = (DisplayName = "환경 시간을 따름"))
	bool bCloudUseSkyTime = true;
};

USTRUCT(BlueprintType)
struct FVWQualityOptions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "LUT", meta = (ClampMin = "1", ClampMax = "64"))
	int32 TransmittanceSteps = 64;

	UPROPERTY(EditAnywhere, Category = "LUT", meta = (ClampMin = "1", ClampMax = "64"))
	int32 MultiScatteringSteps = 64;

	UPROPERTY(EditAnywhere, Category = "LUT", meta = (ClampMin = "1", ClampMax = "64"))
	int32 SkyViewSteps = 64;

	UPROPERTY(EditAnywhere, Category = "LUT", meta = (ClampMin = "1", ClampMax = "16"))
	int32 AerialPerspectiveSteps = 5;

	UPROPERTY(EditAnywhere, Category = "LUT", meta = (ClampMin = "0.1", ClampMax = "6.4"))
	float APDistanceFactor = 3.2f;

	UPROPERTY(EditAnywhere, Category = "Toggle")
	bool bAerialPerspective = true;

	UPROPERTY(EditAnywhere, Category = "Toggle")
	bool bVolumetricShadow = true;

	UPROPERTY(EditAnywhere, Category = "Toggle")
	bool bCloud = true;

	UPROPERTY(EditAnywhere, Category = "Volumetric Shadow", meta = (ClampMin = "1", ClampMax = "128"))
	int32 AerialShadowSteps = 40;

	UPROPERTY(EditAnywhere, Category = "Cloud", meta = (ClampMin = "1", ClampMax = "128"))
	int32 CloudMinSteps = 10;

	UPROPERTY(EditAnywhere, Category = "Cloud", meta = (ClampMin = "1", ClampMax = "128"))
	int32 CloudMaxSteps = 64;

	UPROPERTY(EditAnywhere, Category = "Cloud", meta = (ClampMin = "1", ClampMax = "128"))
	int32 CloudLightViewSteps = 6;

	UPROPERTY(EditAnywhere, Category = "Cloud")
	bool bLightViewDistanceLimit = true;

	UPROPERTY(EditAnywhere, Category = "Cloud", meta = (ClampMin = "0", ClampMax = "0.99"))
	float HistoryWeight = 0.95f;

	UPROPERTY(EditAnywhere, Category = "Cloud", meta = (ClampMin = "0", ClampMax = "5"))
	float NeighborhoodGamma = 2.5f;
	// 구름 레이마칭을 끊을 최대 거리 (m단위)
	UPROPERTY(EditAnywhere, Category = "Cloud", meta = (ClampMin = "0"))
	float CloudRayMarchMaxDistance = 50000.f;
};
