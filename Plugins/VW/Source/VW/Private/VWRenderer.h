#pragma once

#include "CoreMinimal.h"
#include <RenderGraphResources.h>

class FSceneView;
class FRDGBuilder;
class FGlobalShaderMap;

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

	void Render(FRDGBuilder& GraphBuilder, const FSceneView& View, const FViewInputs& Inputs);
private:
	void RenderHello(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FViewInputs& Inputs);
private:
	FLinearColor HelloTint = FLinearColor{ 1.f, 0.3f, 0.3f };
};
