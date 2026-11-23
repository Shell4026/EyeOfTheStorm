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
	// 플러그인이 커브를 셰이더로 보낼 때 쓰는 샘플 수와 같다. 같은 점에서 섞어야 셰이더가 받는 값이 정확히 섞인다
	constexpr int32 CurveSamples = 32;

	// 커브는 키 개수와 위치가 달라 키끼리 섞을 수 없다. 0에서 1까지 같은 간격으로 두 커브를 계산해 섞은 값을 직선 키로 넣는다
	FRuntimeFloatCurve LerpCurve(const FRuntimeFloatCurve& From, const FRuntimeFloatCurve& To, float Alpha)
	{
		if (Alpha <= 0.f)
		{
			return From;
		}
		if (Alpha >= 1.f)
		{
			return To;
		}

		const FRichCurve* FromCurve = From.GetRichCurveConst();
		const FRichCurve* ToCurve = To.GetRichCurveConst();
		FRuntimeFloatCurve Out;
		FRichCurve* OutCurve = Out.GetRichCurve();
		for (int32 i = 0; i < CurveSamples; ++i)
		{
			const float X = static_cast<float>(i) / (CurveSamples - 1);
			const FKeyHandle Key = OutCurve->AddKey(X, FMath::Lerp(FromCurve->Eval(X), ToCurve->Eval(X), Alpha));
			OutCurve->SetKeyInterpMode(Key, RCIM_Linear);
		}
		return Out;
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

	if (TrackKeys.Num() > 0 && TrackPosition != TrackTarget)
	{
		TrackPosition = TrackFollowRate > 0.f
			? FMath::FInterpConstantTo(TrackPosition, TrackTarget, DeltaTime, TrackFollowRate)
			: TrackTarget;
		ApplyTrack();
	}

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

void UStormWeatherSubsystem::TransitionTo(UStormWeatherPreset* Preset, bool bInstant, bool bKeepHour)
{
	AVWActor* Actor = GetWeatherActor();
	if (!Preset || !Actor)
	{
		UE_LOG(LogStormWeather, Warning, TEXT("날씨 전환 실패: 프리셋 %s, 날씨 액터 %s"),
			Preset ? *Preset->GetName() : TEXT("없음"), Actor ? *Actor->GetName() : TEXT("없음"));
		return;
	}

	TrackKeys.Reset();
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
	bTransitionKeepsHour = bKeepHour;
	UE_LOG(LogStormWeather, Log, TEXT("날씨 전환 시작: %s (%.1f초, 안개 가시거리 %.0fm, 시간 %s)"),
		*Preset->GetName(), Duration, Preset->FogVisibilityMeters, bKeepHour ? TEXT("유지") : TEXT("프리셋"));

	if (Duration <= 0.f)
	{
		ApplyBlend(1.f);
		TargetPreset = nullptr;
	}
}

void UStormWeatherSubsystem::StartTrack(const TArray<FStormWeatherKey>& Keys, float Position, float FollowRate)
{
	TrackKeys.Reset();
	for (const FStormWeatherKey& Key : Keys)
	{
		if (Key.Preset)
		{
			TrackKeys.Add(Key);
		}
	}
	TrackKeys.Sort([](const FStormWeatherKey& A, const FStormWeatherKey& B) { return A.Position > B.Position; });

	if (TrackKeys.Num() == 0 || !GetWeatherActor())
	{
		UE_LOG(LogStormWeather, Warning, TEXT("날씨 트랙 시작 실패: 프리셋이 있는 기준점 %d개, 날씨 액터 %s"),
			TrackKeys.Num(), GetWeatherActor() ? *GetWeatherActor()->GetName() : TEXT("없음"));
		TrackKeys.Reset();
		return;
	}

	TargetPreset = nullptr;
	TrackPosition = Position;
	TrackTarget = Position;
	TrackFollowRate = FollowRate;
	AppliedSegment = INDEX_NONE;
	UE_LOG(LogStormWeather, Log, TEXT("날씨 트랙 시작: 기준점 %d개, 위치 %.2f"), TrackKeys.Num(), Position);
	ApplyTrack();
}

void UStormWeatherSubsystem::SetTrackTarget(float Position)
{
	TrackTarget = Position;
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
		FVWArtistSetting Blended = LerpArtist(FromArtist, TargetPreset->Artist, Alpha);
		if (bTransitionKeepsHour)
		{
			// FromArtist의 시간이 아니라 액터의 지금 시간을 써야 시간 속도로 흐른 만큼이 되돌아가지 않는다
			Blended.Hour = Actor->Artist.Hour;
		}
		Actor->SetArtist(Blended);
	}

	// 안개 없는 프리셋으로 갈 때는 색과 높이 값을 지금 값으로 두고 밀도만 줄인다. 걷히는 동안 안개 색이 기본값 쪽으로 바뀌지 않게 한다 (ApplyBetween과 같은 처리)
	FStormFogState ToFog = GetFogState(TargetPreset);
	if (ToFog.Density <= 0.f)
	{
		ToFog = FromFog;
		ToFog.Density = 0.f;
	}
	// 안개 액터는 TransitionTo에서 필요하면 이미 만들었다
	ApplyFog(LerpFog(FromFog, ToFog, Alpha), false);
}

void UStormWeatherSubsystem::ApplyTrack()
{
	if (TrackKeys.Num() == 0)
	{
		return;
	}

	// 위치가 첫 기준점보다 크면 첫 구간의 처음, 마지막 기준점 이하면 마지막 기준점 하나
	const int32 Last = TrackKeys.Num() - 1;
	int32 Segment = Last;
	float Alpha = 0.f;
	for (int32 i = 0; i < Last; ++i)
	{
		if (TrackPosition > TrackKeys[i + 1].Position)
		{
			Segment = i;
			const float Span = TrackKeys[i].Position - TrackKeys[i + 1].Position;
			const float Linear = Span > 0.f ? FMath::Clamp((TrackKeys[i].Position - TrackPosition) / Span, 0.f, 1.f) : 0.f;
			// 기준점에서 변화 속도가 툭 바뀌지 않게 구간 양 끝을 부드럽게 한다
			Alpha = FMath::SmoothStep(0.f, 1.f, Linear);
			break;
		}
	}

	if (Segment == AppliedSegment && Alpha == AppliedAlpha)
	{
		return;
	}
	AppliedSegment = Segment;
	AppliedAlpha = Alpha;
	ApplyBetween(TrackKeys[Segment].Preset, TrackKeys[FMath::Min(Segment + 1, Last)].Preset, Alpha);
}

void UStormWeatherSubsystem::ApplyBetween(const UStormWeatherPreset* From, const UStormWeatherPreset* To, float Alpha)
{
	if (AVWActor* Actor = GetWeatherActor())
	{
		// LerpArtist는 첫 인자의 장소 값을 남기므로, 두 프리셋을 섞은 뒤 레벨 값 위에 Alpha 1로 한 번 더 얹는다
		const FVWArtistSetting Mixed = LerpArtist(From->Artist, To->Artist, Alpha);
		Actor->SetArtist(LerpArtist(Actor->Artist, Mixed, 1.f));
	}

	// 안개 없는 프리셋의 색과 높이 값은 의미가 없으니 상대 프리셋 값을 써서, 안개가 옅어지거나 짙어지는 동안 색이 바뀌지 않게 한다
	FStormFogState FromState = GetFogState(From);
	FStormFogState ToState = GetFogState(To);
	if (FromState.Density <= 0.f)
	{
		FromState = ToState;
		FromState.Density = 0.f;
	}
	else if (ToState.Density <= 0.f)
	{
		ToState = FromState;
		ToState.Density = 0.f;
	}
	const FStormFogState State = LerpFog(FromState, ToState, Alpha);
	ApplyFog(State, State.Density > 0.f);
}

void UStormWeatherSubsystem::ApplyFog(const FStormFogState& State, bool bCreateIfMissing)
{
	AExponentialHeightFog* Fog = GetFogActor(bCreateIfMissing);
	UExponentialHeightFogComponent* FogComponent = Fog ? Fog->GetComponent() : nullptr;
	if (!FogComponent)
	{
		return;
	}

	// 밀도는 소산계수에 비례하므로 선형으로 섞으면 안개가 일정한 속도로 짙어진다 (가시거리를 섞으면 끝에서 갑자기 짙어진다)
	FogComponent->SetFogDensity(State.Density);
	FogComponent->SetFogHeightFalloff(State.HeightFalloff);
	FogComponent->SetFogInscatteringColor(State.Color);
	FogComponent->SetFogMaxOpacity(State.MaxOpacity);
	FogComponent->SetStartDistance(State.StartDistance);
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
	Out.CloudBrightnessCurve = LerpCurve(From.CloudBrightnessCurve, To.CloudBrightnessCurve, Alpha);
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
