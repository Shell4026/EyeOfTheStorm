#include "VWRenderer.h"
#include "VWShaders.h"

#include <GlobalShader.h>
#include <HAL/IConsoleManager.h>
#include <RenderGraphBuilder.h>
#include <RenderGraphUtils.h>
#include <SceneView.h>

static TAutoConsoleVariable<int32> CVarVWHello(
	TEXT("r.VW.Hello"), 0, TEXT("1 = 화면을 붉게함"),
	EConsoleVariableFlags::ECVF_RenderThreadSafe
);
static TAutoConsoleVariable<int32> CVarVWDebugView(
	TEXT("r.VW.DebugView"), 0, TEXT("1 = 광선 방향"),
	EConsoleVariableFlags::ECVF_RenderThreadSafe
);

FVWRenderer::FVWRenderer() = default;
FVWRenderer::~FVWRenderer() = default;

void FVWRenderer::Render(FRDGBuilder& GraphBuilder, const FSceneView& View, const FViewInputs& Inputs)
{
	const FIntPoint ViewSize = Inputs.ViewRect.Size();
	if (ViewSize.X <= 0 || ViewSize.Y <= 0 || Inputs.SceneColor == nullptr || Inputs.SceneDepth == nullptr)
		return;

	RDG_EVENT_SCOPE(GraphBuilder, "VW");
	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(View.GetFeatureLevel());
	if (CVarVWHello.GetValueOnRenderThread() != 0)
	{
		RenderHello(GraphBuilder, ShaderMap, Inputs);
		return;
	}

	const FVWCommonParams CommonParams = CreateCommonParams(View, Inputs.ViewRect);
	const int32 DebugMode = CVarVWDebugView.GetValueOnRenderThread();
	if (DebugMode != 0)
	{
		RenderDebugView(GraphBuilder, ShaderMap, Inputs, CommonParams, DebugMode);
	}
}

FVWCommonParams FVWRenderer::CreateCommonParams(const FSceneView& View, const FIntRect& ViewRect) const
{
	const FViewMatrices& ViewMat = View.ViewMatrices;

	FVWViewUniformParams ViewUB;
	ViewUB.ClipToCamRel = static_cast<FMatrix44f>(ViewMat.GetInvTranslatedViewProjectionMatrix());
	ViewUB.ViewRectMin = ViewRect.Min;
	ViewUB.ViewSize = ViewRect.Size();

	FVWCommonParams CommonParams;
	CommonParams.VWView = TUniformBufferRef<FVWViewUniformParams>::CreateUniformBufferImmediate(ViewUB, EUniformBufferUsage::UniformBuffer_SingleFrame);
	return CommonParams;
}

void FVWRenderer::RenderHello(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FViewInputs& Inputs)
{
	const ETextureCreateFlags UAVTextureFlags = ETextureCreateFlags::ShaderResource | ETextureCreateFlags::UAV;
	const FIntPoint ViewSize = Inputs.ViewRect.Size();
	const FRDGTextureDesc OutDesc = FRDGTextureDesc::Create2D(Inputs.SceneColor->Desc.Extent, Inputs.SceneColor->Desc.Format, FClearValueBinding::Black, UAVTextureFlags);
	FRDGTextureRef Output = GraphBuilder.CreateTexture(OutDesc, TEXT("VW.HelloOutput"));

	FVWHelloCS::FParameters* const ParamsPtr = GraphBuilder.AllocParameters<FVWHelloCS::FParameters>();
	ParamsPtr->SceneColorTexture = Inputs.SceneColor;
	ParamsPtr->Output = GraphBuilder.CreateUAV(Output);
	ParamsPtr->ViewRectMin = Inputs.ViewRect.Min;
	ParamsPtr->ViewSize = ViewSize;
	ParamsPtr->Tint = FVector4f{ HelloTint.R, HelloTint.G, HelloTint.B, 1.f };

	TShaderMapRef<FVWHelloCS> ComputeShader{ ShaderMap };
	FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("VW.Hello"), ComputeShader, ParamsPtr, FComputeShaderUtils::GetGroupCount(ViewSize, 8));

	AddCopyTexturePass(GraphBuilder, Output, Inputs.SceneColor, Inputs.ViewRect.Min, Inputs.ViewRect.Min, ViewSize);
}

void FVWRenderer::RenderDebugView(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FViewInputs& Inputs, const FVWCommonParams& CommonParams, uint32 DebugMode)
{
	const ETextureCreateFlags UAVTextureFlags = ETextureCreateFlags::ShaderResource | ETextureCreateFlags::UAV;
	const FIntPoint ViewSize = Inputs.ViewRect.Size();
	FRDGTextureRef Output = GraphBuilder.CreateTexture(
		FRDGTextureDesc::Create2D(Inputs.SceneColor->Desc.Extent, Inputs.SceneColor->Desc.Format, FClearValueBinding::Black, UAVTextureFlags), 
		TEXT("VW.DebugView")
	);

	FVWDebugViewCS::FParameters* Params = GraphBuilder.AllocParameters<FVWDebugViewCS::FParameters>();
	Params->Common = CommonParams;
	Params->SceneColorTex = Inputs.SceneColor;
	Params->SceneDepthTex = Inputs.SceneDepth;
	Params->Output = GraphBuilder.CreateUAV(Output);
	Params->DebugMode = DebugMode;

	TShaderMapRef<FVWDebugViewCS> ComputeShader{ ShaderMap };
	FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("VW.DebugView"), ComputeShader, Params, FComputeShaderUtils::GetGroupCount(ViewSize, 8));

	AddCopyTexturePass(GraphBuilder, Output, Inputs.SceneColor, Inputs.ViewRect.Min, Inputs.ViewRect.Min, ViewSize);
}
