#include "Weather/StormWeatherSubsystem.h"

#include "Weather/StormWeatherPreset.h"
#include "VWActor.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/World.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogStormWeather, Log, All);

namespace
{
	FVector2D LerpVector2D(const FVector2D& A, const FVector2D& B, float Alpha)
	{
		return FMath::Lerp(A, B, static_cast<double>(Alpha));
	}

	// 350도에서 10도로 갈 때 340도를 돌지 않고 20도만 돈다
	float LerpAngleDegrees(float A, float B, float Alpha)
	{
		const float Delta = FRotator::NormalizeAxis(B - A);
		return FMath::Fmod(A + Delta * Alpha + 360.f, 360.f);
	}
}

void UStormWeatherSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!TargetPreset)
	{
		return;
	}

	Elapsed += DeltaTime;
	const float Linear = Duration > 0.f ? FMath::Clamp(Elapsed / Duration, 0.f, 1.f) : 1.f;
	ApplyBlend(FMath::SmoothStep(0.f, 1.f, Linear));
	if (Linear >= 1.f)
	{
		UE_LOG(LogStormWeather, Log, TEXT("날씨 전환 끝: %s"), *TargetPreset->GetName());
		TargetPreset = nullptr;
	}
}

TStatId UStormWeatherSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UStormWeatherSubsystem, STATGROUP_Tickables);
}

void UStormWeatherSubsystem::TransitionTo(UStormWeatherPreset* Preset, bool bInstant)
{
	AVWActor* Actor = GetWeatherActor();
	if (!Preset || !Actor)
	{
		UE_LOG(LogStormWeather, Warning, TEXT("날씨 전환 실패: 프리셋 %s, 날씨 액터 %s"),
			Preset ? *Preset->GetName() : TEXT("없음"), Actor ? *Actor->GetName() : TEXT("없음"));
		return;
	}

	FromArtist = Actor->Artist;
	// 안개 액터가 없으면 "안개 없음"에서 시작한다. 색은 목표 색으로 둬서 옅을 때부터 맞는 색이 나오게 한다
	const AExponentialHeightFog* Fog = GetFogActor(Preset->GetFogDensity() > 0.f);
	FromFog = Fog ? GetFogState(Fog) : GetFogState(Preset);
	if (!Fog)
	{
		FromFog.Density = 0.f;
	}

	TargetPreset = Preset;
	Elapsed = 0.f;
	Duration = bInstant ? 0.f : Preset->TransitionDuration;
	UE_LOG(LogStormWeather, Log, TEXT("날씨 전환 시작: %s (%.1f초, 안개 가시거리 %.0fm)"),
		*Preset->GetName(), Duration, Preset->FogVisibilityMeters);

	if (Duration <= 0.f)
	{
		ApplyBlend(1.f);
		TargetPreset = nullptr;
	}
}

bool UStormWeatherSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// 에디터 월드의 액터 값을 바꾸면 레벨에 저장되므로 게임과 PIE에서만 돈다
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UStormWeatherSubsystem::ApplyBlend(float Alpha)
{
	if (!TargetPreset)
	{
		return;
	}

	if (AVWActor* Actor = GetWeatherActor())
	{
		Actor->SetArtist(LerpArtist(FromArtist, TargetPreset->Artist, Alpha));
	}

	AExponentialHeightFog* Fog = GetFogActor(false);
	UExponentialHeightFogComponent* FogComponent = Fog ? Fog->GetComponent() : nullptr;
	if (FogComponent)
	{
		// 밀도는 소산계수에 비례하므로 선형으로 섞으면 안개가 일정한 속도로 짙어진다 (가시거리를 섞으면 끝에서 갑자기 짙어진다)
		const FStormFogState State = LerpFog(FromFog, GetFogState(TargetPreset), Alpha);
		FogComponent->SetFogDensity(State.Density);
		FogComponent->SetFogHeightFalloff(State.HeightFalloff);
		FogComponent->SetFogInscatteringColor(State.Color);
		FogComponent->SetFogMaxOpacity(State.MaxOpacity);
		FogComponent->SetStartDistance(State.StartDistance);
	}
}

FVWArtistSetting UStormWeatherSubsystem::LerpArtist(const FVWArtistSetting& From, const FVWArtistSetting& To, float Alpha)
{
	FVWArtistSetting Out = From;

	Out.SunIntensity = FMath::Lerp(From.SunIntensity, To.SunIntensity, Alpha);
	Out.Hour = FMath::Lerp(From.Hour, To.Hour, Alpha);

	Out.RayleighScatteringStrength = FMath::Lerp(From.RayleighScatteringStrength, To.RayleighScatteringStrength, Alpha);
	Out.MieScatteringStrength = FMath::Lerp(From.MieScatteringStrength, To.MieScatteringStrength, Alpha);
	Out.MieAnisotropy = FMath::Lerp(From.MieAnisotropy, To.MieAnisotropy, Alpha);
	Out.MieColor = FMath::Lerp(From.MieColor, To.MieColor, Alpha);

	Out.CloudColor = FMath::Lerp(From.CloudColor, To.CloudColor, Alpha);
	Out.CloudAmount = FMath::Lerp(From.CloudAmount, To.CloudAmount, Alpha);
	Out.CloudVerticalAmount = FMath::Lerp(From.CloudVerticalAmount, To.CloudVerticalAmount, Alpha);
	Out.CloudSizeKM = FMath::Lerp(From.CloudSizeKM, To.CloudSizeKM, Alpha);
	Out.CloudDetail = FMath::Lerp(From.CloudDetail, To.CloudDetail, Alpha);
	Out.CloudDensity = FMath::Lerp(From.CloudDensity, To.CloudDensity, Alpha);
	Out.CloudBrightness = FMath::Lerp(From.CloudBrightness, To.CloudBrightness, Alpha);
	Out.CloudBrightnessBezier.A = LerpVector2D(From.CloudBrightnessBezier.A, To.CloudBrightnessBezier.A, Alpha);
	Out.CloudBrightnessBezier.B = LerpVector2D(From.CloudBrightnessBezier.B, To.CloudBrightnessBezier.B, Alpha);
	Out.CloudBrightnessBezier.C = LerpVector2D(From.CloudBrightnessBezier.C, To.CloudBrightnessBezier.C, Alpha);
	Out.CloudBrightnessBezier.D = LerpVector2D(From.CloudBrightnessBezier.D, To.CloudBrightnessBezier.D, Alpha);
	Out.CloudSoftness = FMath::Lerp(From.CloudSoftness, To.CloudSoftness, Alpha);

	Out.WindDirectionDegrees = LerpAngleDegrees(From.WindDirectionDegrees, To.WindDirectionDegrees, Alpha);
	Out.WindSpeedKmh = FMath::Lerp(From.WindSpeedKmh, To.WindSpeedKmh, Alpha);

	return Out;
}

FStormFogState UStormWeatherSubsystem::LerpFog(const FStormFogState& From, const FStormFogState& To, float Alpha)
{
	FStormFogState Out;
	Out.Density = FMath::Lerp(From.Density, To.Density, Alpha);
	Out.HeightFalloff = FMath::Lerp(From.HeightFalloff, To.HeightFalloff, Alpha);
	Out.Color = FMath::Lerp(From.Color, To.Color, Alpha);
	Out.MaxOpacity = FMath::Lerp(From.MaxOpacity, To.MaxOpacity, Alpha);
	Out.StartDistance = FMath::Lerp(From.StartDistance, To.StartDistance, Alpha);
	return Out;
}

AVWActor* UStormWeatherSubsystem::GetWeatherActor()
{
	if (!WeatherActor.IsValid())
	{
		for (TActorIterator<AVWActor> It(GetWorld()); It; ++It)
		{
			WeatherActor = *It;
			break;
		}
	}
	return WeatherActor.Get();
}

AExponentialHeightFog* UStormWeatherSubsystem::GetFogActor(bool bCreateIfMissing)
{
	if (!FogActor.IsValid())
	{
		for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
		{
			FogActor = *It;
			break;
		}
	}
	if (!FogActor.IsValid() && bCreateIfMissing)
	{
		// 안개 밀도는 액터 높이 기준이라 날씨 액터 높이에 둔다. 아레나 바닥 높이에 맞추려면 레벨에 직접 배치한다
		const AVWActor* Weather = GetWeatherActor();
		const FVector Location = Weather ? Weather->GetActorLocation() : FVector::ZeroVector;
		AExponentialHeightFog* Spawned = GetWorld()->SpawnActor<AExponentialHeightFog>(Location, FRotator::ZeroRotator);
		if (Spawned)
		{
			Spawned->GetComponent()->SetFogDensity(0.f);
			FogActor = Spawned;
			UE_LOG(LogStormWeather, Log, TEXT("레벨에 높이 안개가 없어 Z=%.0f에 만들었습니다"), Location.Z);
		}
	}
	return FogActor.Get();
}

FStormFogState UStormWeatherSubsystem::GetFogState(const AExponentialHeightFog* Fog)
{
	FStormFogState State;
	const UExponentialHeightFogComponent* Component = Fog ? Fog->GetComponent() : nullptr;
	if (Component)
	{
		State.Density = Component->FogDensity;
		State.HeightFalloff = Component->FogHeightFalloff;
		State.Color = Component->FogInscatteringLuminance;
		State.MaxOpacity = Component->FogMaxOpacity;
		State.StartDistance = Component->StartDistance;
	}
	return State;
}

FStormFogState UStormWeatherSubsystem::GetFogState(const UStormWeatherPreset* Preset)
{
	FStormFogState State;
	State.Density = Preset->GetFogDensity();
	State.HeightFalloff = Preset->FogHeightFalloff;
	State.Color = Preset->FogColor;
	State.MaxOpacity = Preset->FogMaxOpacity;
	State.StartDistance = Preset->FogStartDistance;
	return State;
}
