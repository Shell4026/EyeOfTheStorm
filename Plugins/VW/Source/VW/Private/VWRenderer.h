#pragma once

#include "VWRenderSettings.h"

#include "CoreMinimal.h"
#include "RendererInterface.h"
#include "RenderGraphResources.h"

class FSceneView;
class FRDGBuilder;
class FGlobalShaderMap;
class FVWCommonParams;

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

	void Render(FRDGBuilder& GraphBuilder, const FSceneView& View, const FViewInputs& Inputs);
private:
	FVWCommonParams CreateCommonParams(const FSceneView& View, const FIntRect& ViewRect) const;
	void RenderHello(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FViewInputs& Inputs);
	void RenderDebugView(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FViewInputs& Inputs, const FVWCommonParams& CommonParams, uint32 DebugMode);
	void RenderStaticLUTs(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FVWCommonParams& Common, FRDGTextureRef& OutTransmittance);
private:
	FLinearColor HelloTint = FLinearColor{ 1.f, 0.3f, 0.3f };
	FVWRenderSettings Settings;
	bool bStaticLUTDirty = false;

	TRefCountPtr<IPooledRenderTarget> TransmittanceLUT;
};
