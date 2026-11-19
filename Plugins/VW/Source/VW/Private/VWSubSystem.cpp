// Fill out your copyright notice in the Description page of Project Settings.


#include "VWSubSystem.h"
#include "VWViewExtension.h"

#include "RenderingThread.h"
#include "TextureResource.h"
#include "Engine/Texture.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void UVWSubSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ViewExtension = FSceneViewExtensions::NewExtension<FVWViewExtension>(GetWorld());
	UE_LOG(LogTemp, Log, TEXT("[VW] 서브 시스템 생성: %s, (PIE = %d)"), *GetWorld()->GetName(), GetWorld()->IsPlayInEditor() ? 1 : 0);
}

void UVWSubSystem::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("[VW] 서브 시스템 제거: %s, (PIE = %d)"), *GetWorld()->GetName(), GetWorld()->IsPlayInEditor() ? 1 : 0);

	ENQUEUE_RENDER_COMMAND(VWReleaseViewExtensin)(
		[Extension = MoveTemp(ViewExtension)](FRHICommandListImmediate&) mutable
		{
			Extension.Reset();
		}
	);

	Super::Deinitialize();
}

void UVWSubSystem::RegisterWeatherActor(AActor* Actor)
{
	WeatherActorPtr = Actor;
	if (!bCloudNoiseLoaded)
	{
		LoadCloudNoise();
		bCloudNoiseLoaded = true;
	}

	ENQUEUE_RENDER_COMMAND(VWSetActive)(
		[Extension = ViewExtension](FRHICommandListImmediate&)
		{
			if (Extension)
				Extension->SetActive_RenderThread(true);
		}
	);
}

void UVWSubSystem::UnRegisterWeatherActor(AActor* Actor)
{
	if (WeatherActorPtr.Get() != Actor)
		return;
	WeatherActorPtr.Reset();
	ENQUEUE_RENDER_COMMAND(VWSetActive)(
		[Extension = ViewExtension](FRHICommandListImmediate&)
		{
			if (Extension)
				Extension->SetActive_RenderThread(false);
		}
	);
}

void UVWSubSystem::PushHelloTint(const FLinearColor& InColor)
{
	ENQUEUE_RENDER_COMMAND(VWPushHelloTint)(
		[Extension = ViewExtension, InColor](FRHICommandListImmediate&)
		{
			if (Extension)
				Extension->GetRenderer_RenderThread().SetHelloTint_RenderThread(InColor);
		}
	);
}

void UVWSubSystem::PushSettings(const FVWRenderSettings& Settings, bool bStaticLUTDirty)
{
	ENQUEUE_RENDER_COMMAND(VWPushSettings)(
		[Extension = ViewExtension, Settings, bStaticLUTDirty](FRHICommandListImmediate&)
		{
			if (Extension)
				Extension->GetRenderer_RenderThread().SetSettings_RenderThread(Settings, bStaticLUTDirty);
		}
	);
}

void UVWSubSystem::PushShadowCapture(UTextureRenderTarget2D* DepthTarget, const FVWShadowCaptureInfo& Info)
{
	FTextureRenderTargetResource* Resource = DepthTarget ? DepthTarget->GameThread_GetRenderTargetResource() : nullptr;

	ENQUEUE_RENDER_COMMAND(VWPushShadowCapture)(
		[Extension = ViewExtension, Info, Resource](FRHICommandListImmediate&)
		{
			if (Extension)
			{
				FVWShadowCaptureInfo Copy = Info;
				Copy.DepthTexture = Resource ? Resource->GetRenderTargetTexture() : nullptr;
				Extension->GetRenderer_RenderThread().SetShadowCapture_RenderThread(Copy);
			}
		});
}

void UVWSubSystem::PushBlueNoise(UTexture* Texture)
{
	FTextureResource* Resource = Texture ? Texture->GetResource() : nullptr;

	ENQUEUE_RENDER_COMMAND(VWPushBlueNoise)(
		[Extension = ViewExtension, Resource](FRHICommandListImmediate&)
		{
			if (Extension)
				Extension->GetRenderer_RenderThread().SetBlueNoise_RenderThread(Resource ? Resource->TextureRHI : nullptr);
		}
	);
}

bool UVWSubSystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Type::Editor || WorldType == EWorldType::Type::PIE || WorldType == EWorldType::Type::Game;
}

void UVWSubSystem::LoadCloudNoise()
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("VW"));
	if (!Plugin.IsValid())
		return;

	const FString NoiseDir = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources"), TEXT("Noise"));
	TArray<uint8> PerlinWorley, Worley0, Worley1, Worley2;
	const bool bLoaded =
		FFileHelper::LoadFileToArray(PerlinWorley, *FPaths::Combine(NoiseDir, TEXT("perlinWorley.bin"))) &&
		FFileHelper::LoadFileToArray(Worley0, *FPaths::Combine(NoiseDir, TEXT("worley0.bin"))) &&
		FFileHelper::LoadFileToArray(Worley1, *FPaths::Combine(NoiseDir, TEXT("worley1.bin"))) &&
		FFileHelper::LoadFileToArray(Worley2, *FPaths::Combine(NoiseDir, TEXT("worley2.bin")));
	if (!bLoaded)
	{
		UE_LOG(LogTemp, Error, TEXT("[VW] 노이즈 파일을 찾을 수 없습니다: %s"), *NoiseDir);
		return;
	}

	ENQUEUE_RENDER_COMMAND(VWInitCloudNoise)(
		[Extension = ViewExtension,
		PerlinWorley = MoveTemp(PerlinWorley), Worley0 = MoveTemp(Worley0),
		Worley1 = MoveTemp(Worley1), Worley2 = MoveTemp(Worley2)](FRHICommandListImmediate& RHICmdList) mutable
		{
			if (Extension)
			{
				Extension->GetRenderer_RenderThread().InitCloudNoise_RenderThread(RHICmdList,
					MoveTemp(PerlinWorley), MoveTemp(Worley0), MoveTemp(Worley1), MoveTemp(Worley2));
			}
		}
	);
}