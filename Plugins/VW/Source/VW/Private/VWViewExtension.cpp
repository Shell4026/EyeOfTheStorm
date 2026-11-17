// Fill out your copyright notice in the Description page of Project Settings.


#include "VWViewExtension.h"
#include <FXRenderingUtils.h>
#include <PostProcess/PostProcessInputs.h>	

FVWViewExtension::FVWViewExtension(const FAutoRegister& AutoReg, UWorld* InWorld) :
	FWorldSceneViewExtension(AutoReg, InWorld)
{
}

FVWViewExtension::~FVWViewExtension()
{
}

void FVWViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs)
{
	if (!bActive)
		return;
	if (InView.bIsSceneCapture || InView.bIsReflectionCapture || InView.bIsPlanarReflection)
		return;

	Inputs.Validate();
	const FSceneTextureUniformParameters* SceneTextures = Inputs.SceneTextures->GetContents();

	FVWRenderer::FViewInputs ViewInputs;
	ViewInputs.SceneColor = SceneTextures->SceneColorTexture;
	ViewInputs.SceneDepth = SceneTextures->SceneDepthTexture;
	ViewInputs.ViewRect = UE::FXRenderingUtils::GetRawViewRectUnsafe(InView);

	Renderer.Render(GraphBuilder, InView, ViewInputs);
}
