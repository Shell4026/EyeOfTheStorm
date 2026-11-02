// Fill out your copyright notice in the Description page of Project Settings.


#include "VWViewExtension.h"
#include "FXRenderingUtils.h"

FVWViewExtension::FVWViewExtension(const FAutoRegister& AutoReg, UWorld* InWorld) :
	FWorldSceneViewExtension(AutoReg, InWorld)
{
}

FVWViewExtension::~FVWViewExtension()
{
}

void FVWViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs)
{
	if (GFrameCounterRenderThread % 120 != 0)
		return;

	const FIntRect ViewRect = UE::FXRenderingUtils::GetRawViewRectUnsafe(InView);

	const int IsRenderThread = IsInRenderingThread() ? 1 : 0;
	UE_LOG(LogTemp, Log, TEXT("[VW] PrePostProcessPass 렌더 스레드 = %d, 뷰 = %u, 크기 = %dx%d, SceneCapture = %d"),
		IsRenderThread, InView.GetViewKey(), ViewRect.Width(), ViewRect.Height(), InView.bIsSceneCapture ? 1 : 0);
}
