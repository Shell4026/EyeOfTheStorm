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
	TEXT("r.VW.DebugView"), 0, TEXT("1 = 광선 방향, 2 = 표면 거리, 3 = 태양 원반"),
	EConsoleVariableFlags::ECVF_RenderThreadSafe
);

namespace
{
	constexpr int32 TransmittanceLUTSize = 256;
	const ETextureCreateFlags UAVTextureFlags = TexCreate_ShaderResource | TexCreate_UAV;
}

FVWRenderer::FVWRenderer() = default;
FVWRenderer::~FVWRenderer() = default;

void FVWRenderer::SetSettings_RenderThread(const FVWRenderSettings& InSettings, bool bInStaticLUTDirty)
{
	check(IsInRenderingThread());
	Settings = InSettings;
	bStaticLUTDirty |= bInStaticLUTDirty;
}

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
	else
	{
		FRDGTextureRef TransmittanceTex = nullptr;
		RenderStaticLUTs(GraphBuilder, ShaderMap, CommonParams, TransmittanceTex);
	}
}

FVWCommonParams FVWRenderer::CreateCommonParams(const FSceneView& View, const FIntRect& ViewRect) const
{
	const FViewMatrices& ViewMat = View.ViewMatrices;

	FVWViewUniformParams ViewUB;
	ViewUB.ClipToCamRel = static_cast<FMatrix44f>(ViewMat.GetInvTranslatedViewProjectionMatrix());
	ViewUB.CamPosWorld = VWSpace::UEToWeatherPos(ViewMat.GetViewOrigin());
	ViewUB.ViewRectMin = ViewRect.Min;
	ViewUB.ViewSize = ViewRect.Size();

	const FVWAtmosphereSettings& A = Settings.Atmosphere;
	FVWAtmosphereUniformParams AtmoUB;
	AtmoUB.GroundRadius = A.GroundRadius;
	AtmoUB.AtmosphereRadius = A.AtmosphereRadius;
	AtmoUB.MieCoefficient = A.MieCoefficient;
	AtmoUB.MieG = A.MieG;
	AtmoUB.MieColor = A.MieColor;
	AtmoUB.SunIlluminance = Settings.Lighting.SunIlluminance;
	AtmoUB.RayleighColor = A.RayleighColor;
	AtmoUB.APDistanceFactor = Settings.Quality.APDistanceFactor;
	AtmoUB.SunDir = Settings.Lighting.SunDirW;
	AtmoUB.ModeFlags = 0;

	FVWCommonParams CommonParams;
	CommonParams.VWView = TUniformBufferRef<FVWViewUniformParams>::CreateUniformBufferImmediate(ViewUB, EUniformBufferUsage::UniformBuffer_SingleFrame);
	CommonParams.VWAtmosphere = TUniformBufferRef<FVWAtmosphereUniformParams>::CreateUniformBufferImmediate(AtmoUB, EUniformBufferUsage::UniformBuffer_SingleFrame);
	return CommonParams;
}

void FVWRenderer::RenderHello(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FViewInputs& Inputs)
{
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

void FVWRenderer::RenderStaticLUTs(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FVWCommonParams& Common, FRDGTextureRef& OutTransmittance)
{
	if (!bStaticLUTDirty && TransmittanceLUT.IsValid())
	{
		OutTransmittance = GraphBuilder.RegisterExternalTexture(TransmittanceLUT);
		return;
	}
	
	RDG_EVENT_SCOPE(GraphBuilder, "VW.StaticLUTs");

	FRDGTextureRef Transmittance = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(
		FIntPoint(TransmittanceLUTSize), EPixelFormat::PF_FloatRGBA, FClearValueBinding::None, UAVTextureFlags), TEXT("VW.TransmittanceLUT"));
	{
		FVWTransmittanceLUTCS::FParameters* Param = GraphBuilder.AllocParameters<FVWTransmittanceLUTCS::FParameters>();
		Param->Common = Common;
		Param->Steps = Settings.Quality.TransmittanceSteps;
		Param->OutLUT = GraphBuilder.CreateUAV(Transmittance);

		TShaderMapRef<FVWTransmittanceLUTCS> CS(ShaderMap);
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("VW.TransmittanceLUT"), CS, Param, FComputeShaderUtils::GetGroupCount(FIntPoint(TransmittanceLUTSize), 16));
	}
	// 그래프 밖으로 꺼내서 다음 프레임에도 쓴다.
	TransmittanceLUT = GraphBuilder.ConvertToExternalTexture(Transmittance);
	bStaticLUTDirty = false;

	OutTransmittance = Transmittance;
}