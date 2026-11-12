// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "VWRenderer.h"

class FVWViewExtension : public FWorldSceneViewExtension
{
public:
	FVWViewExtension(const FAutoRegister& AutoReg, UWorld* InWorld);
	~FVWViewExtension();

	virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs) override;

	void SetActive_RenderThread(bool bInActive) { check(IsInRenderingThread()); bActive = bInActive; }

	FVWRenderer& GetRenderer_RenderThread() { check(IsInRenderingThread()); return Renderer; }
private:
	FVWRenderer Renderer;
	bool bActive = false;
};
