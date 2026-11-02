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

	void Render(FRDGBuilder& GraphBuilder, const FSceneView& View, const FViewInputs& Inputs);
private:
	void RenderHello(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FViewInputs& Inputs);
};
