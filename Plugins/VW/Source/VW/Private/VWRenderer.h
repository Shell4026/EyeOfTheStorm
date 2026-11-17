#pragma once

#include "VWRenderSettings.h"

#include "CoreMinimal.h"
#include "RendererInterface.h"
#include "RenderGraphResources.h"

class FSceneView;
class FRDGBuilder;
class FGlobalShaderMap;
class FVWCommonParams;

struct FVWShadowCaptureInfo
{
	FTextureRHIRef DepthTexture;
	FVector3f OriginUE = FVector3f::ZeroVector;
	FVector3f Forward = FVector3f(1.f, 0.f, 0.f);
	FVector3f Right = FVector3f(0.f, 1.f, 0.f);
	FVector3f Up = FVector3f(0.f, 0.f, 1.f);
	float OrthoWidth = 2'000'000.f; // 20km
	float DepthBias = 200.f; // 200cm
};

class FVWRenderer
{
public:
	struct FViewInputs
	{
		FRDGTextureRef SceneColor = nullptr;
		FRDGTextureRef SceneDepth = nullptr;
		FIntRect ViewRect;
	};
public:
	FVWRenderer();
	~FVWRenderer();

	void SetHelloTint_RenderThread(const FLinearColor& InColor) { check(IsInRenderingThread()); HelloTint = InColor; }
	void SetSettings_RenderThread(const FVWRenderSettings& InSettings, bool bInStaticLUTDirty);
	void SetShadowCapture_RenderThread(const FVWShadowCaptureInfo& InInfo);

	void Render(FRDGBuilder& GraphBuilder, const FSceneView& View, const FViewInputs& Inputs);
private:
	struct StaticLUT
	{
		FRDGTextureRef Transmittance = nullptr;
		FRDGTextureRef MultiScattering = nullptr;
	};
	FVWCommonParams CreateCommonParams(const FSceneView& View, const FIntRect& ViewRect) const;

	void RenderHello(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FViewInputs& Inputs);
	void RenderDebugView(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FViewInputs& Inputs, const FVWCommonParams& CommonParams, uint32 DebugMode);
	void RenderStaticLUTs(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FVWCommonParams& Common, StaticLUT& Output);
	FRDGTextureRef RenderSkyViewLUT(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FVWCommonParams& Common, FRDGTextureRef Transmittance, FRDGTextureRef MultiScattering);
	void RenderComposite(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FVWCommonParams& Common, const FViewInputs& Inputs, 
		FRDGTextureRef SkyView, FRDGTextureRef AerialPerspective, FRDGTextureRef AerialShadow, FRDGTextureRef QuarterDepth);
	FRDGTextureRef RenderAerialPerspective(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FVWCommonParams& Common, const FViewInputs& Inputs, FRDGTextureRef Transmittance, FRDGTextureRef MultiScattering);
	FRDGTextureRef RenderLowResDepth(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FVWCommonParams& Common, const FViewInputs& Inputs, int32 Divider);
	FRDGTextureRef RenderAerialShadow(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FVWCommonParams& Common, 
		const FViewInputs& Inputs, FRDGTextureRef AerialPerspective, FRDGTextureRef BlueNoiseTexture);

	uint32 GetModeFlags() const;
private:
	FLinearColor HelloTint = FLinearColor{ 1.f, 0.3f, 0.3f };
	FVWRenderSettings Settings;
	FVWShadowCaptureInfo Shadow;

	bool bStaticLUTDirty = false;

	TRefCountPtr<IPooledRenderTarget> TransmittanceLUT;
	TRefCountPtr<IPooledRenderTarget> MultiScatteringLUT;
};
