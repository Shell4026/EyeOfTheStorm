#include "VWRenderer.h"
#include "VWShaders.h"

#include "HAL/IConsoleManager.h"
#include "GlobalShader.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "SceneView.h"
#include "SystemTextures.h"

static TAutoConsoleVariable<int32> CVarVWHello(
	TEXT("r.VW.Hello"), 0, TEXT("1 = 화면을 붉게함"), EConsoleVariableFlags::ECVF_RenderThreadSafe);
static TAutoConsoleVariable<int32> CVarVWDebugView(
	TEXT("r.VW.DebugView"), 0, TEXT("1 = 광선 방향, 2 = 표면 거리, 3 = 태양 원반"), EConsoleVariableFlags::ECVF_RenderThreadSafe);
static TAutoConsoleVariable<int32> CVarVWEnable(
	TEXT("r.VW.Enable"), 1, TEXT("VolumetricWeather 렌더링 사용 여부"), EConsoleVariableFlags::ECVF_RenderThreadSafe);

namespace
{
	constexpr int32 TransmittanceLUTSize = 256;
	constexpr int32 MultiScatteringLUTSize = 128;
	const FIntPoint SkyViewLUTSize(200, 400);
	const int32 AerialPerspectiveLUTSize = 32;
	const ETextureCreateFlags UAVTextureFlags = TexCreate_ShaderResource | TexCreate_UAV;

	FRHISamplerState* GetLinearClampSampler()
	{
		return TStaticSamplerState<ESamplerFilter::SF_Bilinear, ESamplerAddressMode::AM_Clamp, ESamplerAddressMode::AM_Clamp, ESamplerAddressMode::AM_Clamp>::GetRHI();
	}
	FRHISamplerState* GetSkyViewSampler()
	{
		return TStaticSamplerState<ESamplerFilter::SF_Bilinear, ESamplerAddressMode::AM_Wrap, ESamplerAddressMode::AM_Clamp, ESamplerAddressMode::AM_Clamp>::GetRHI();
	}
}

FVWRenderer::FVWRenderer() = default;
FVWRenderer::~FVWRenderer() = default;

void FVWRenderer::SetSettings_RenderThread(const FVWRenderSettings& InSettings, bool bInStaticLUTDirty)
{
	check(IsInRenderingThread());
	Settings = InSettings;
	bStaticLUTDirty |= bInStaticLUTDirty;
}

void FVWRenderer::SetShadowCapture_RenderThread(const FVWShadowCaptureInfo& InInfo)
{
	Shadow = InInfo;
	UE_LOG(LogTemp, Log, TEXT("[VW] 그림자 캡처 수신: %s"), Shadow.DepthTexture.IsValid() ? *Shadow.DepthTexture->GetSizeXYZ().ToString() : TEXT("없음"));
}

void FVWRenderer::Render(FRDGBuilder& GraphBuilder, const FSceneView& View, const FViewInputs& Inputs)
{
	if (CVarVWEnable.GetValueOnRenderThread() == 0)
		return;

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
		const uint32 ModeFlags = GetModeFlags();

		StaticLUT StaticLUTs{};
		RenderStaticLUTs(GraphBuilder, ShaderMap, CommonParams, StaticLUTs);
		FRDGTextureRef SkyViewTex = RenderSkyViewLUT(GraphBuilder, ShaderMap, CommonParams, StaticLUTs.Transmittance, StaticLUTs.MultiScattering);
		FRDGTextureRef AerialPerspectiveTex = (ModeFlags & 1)
			? RenderAerialPerspective(GraphBuilder, ShaderMap, CommonParams, Inputs, StaticLUTs.Transmittance, StaticLUTs.MultiScattering)
			: GSystemTextures.GetVolumetricBlackDummy(GraphBuilder);
		RenderComposite(GraphBuilder, ShaderMap, CommonParams, Inputs, SkyViewTex, AerialPerspectiveTex);
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
	AtmoUB.ModeFlags = GetModeFlags();

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

void FVWRenderer::RenderStaticLUTs(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FVWCommonParams& Common, StaticLUT& Output)
{
	if (!bStaticLUTDirty && TransmittanceLUT.IsValid() && MultiScatteringLUT.IsValid())
	{
		Output.Transmittance = GraphBuilder.RegisterExternalTexture(TransmittanceLUT);
		Output.MultiScattering = GraphBuilder.RegisterExternalTexture(MultiScatteringLUT);
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

	FRDGTextureRef MultiScattering = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(
		FIntPoint(MultiScatteringLUTSize), EPixelFormat::PF_FloatRGBA, FClearValueBinding::None, UAVTextureFlags), TEXT("VW.MultiScatteringLUT"));
	{
		FVWMultiScatteringLUTCS::FParameters* Param = GraphBuilder.AllocParameters<FVWMultiScatteringLUTCS::FParameters>();
		Param->Common = Common;
		Param->Steps = Settings.Quality.MultiScatteringSteps;
		Param->TransmittanceLUT = Transmittance;
		Param->LinearClampSampler = GetLinearClampSampler();
		Param->OutLUT = GraphBuilder.CreateUAV(MultiScattering);

		TShaderMapRef<FVWMultiScatteringLUTCS> CS(ShaderMap);
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("VW.MultiScatteringLUT"), CS, Param, FComputeShaderUtils::GetGroupCount(FIntPoint(MultiScatteringLUTSize), 16));
	}

	// 그래프 밖으로 꺼내서 다음 프레임에도 쓴다.
	TransmittanceLUT = GraphBuilder.ConvertToExternalTexture(Transmittance);
	MultiScatteringLUT = GraphBuilder.ConvertToExternalTexture(MultiScattering);
	
	bStaticLUTDirty = false;

	Output.Transmittance = Transmittance;
	Output.MultiScattering = Transmittance;
}

FRDGTextureRef FVWRenderer::RenderSkyViewLUT(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FVWCommonParams& Common, FRDGTextureRef Transmittance, FRDGTextureRef MultiScattering)
{
	FRDGTextureRef SkyView = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(
		SkyViewLUTSize, EPixelFormat::PF_FloatRGBA, FClearValueBinding::None, UAVTextureFlags), TEXT("VW.SkyViewLUT"));

	FVWSkyViewLUTCS::FParameters* Param = GraphBuilder.AllocParameters<FVWSkyViewLUTCS::FParameters>();
	Param->Common = Common;
	Param->Steps = Settings.Quality.SkyViewSteps;
	Param->TransmittanceLUT = Transmittance;
	Param->MultiScatteringLUT = MultiScattering;
	Param->LinearClampSampler = GetLinearClampSampler();
	Param->OutLUT = GraphBuilder.CreateUAV(SkyView);

	TShaderMapRef<FVWSkyViewLUTCS> CS(ShaderMap);
	FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("VW.SkyViewLUT"), CS, Param, FComputeShaderUtils::GetGroupCount(SkyViewLUTSize, 16));
	return SkyView;
}

void FVWRenderer::RenderComposite(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FVWCommonParams& Common, const FViewInputs& Inputs, FRDGTextureRef SkyView, FRDGTextureRef AerialPerspective)
{
	const FIntPoint ViewSize = Inputs.ViewRect.Size();
	FRDGTextureRef Output = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(
		Inputs.SceneColor->Desc.Extent, Inputs.SceneColor->Desc.Format, FClearValueBinding::Black, UAVTextureFlags), TEXT("VW.Composite"));

	FVWCompositeCS::FParameters* Param = GraphBuilder.AllocParameters<FVWCompositeCS::FParameters>();
	Param->Common = Common;
	Param->SceneColorTexture = Inputs.SceneColor;
	Param->SceneDepthTexture = Inputs.SceneDepth;
	Param->SkyViewLUT = SkyView;
	Param->AerialPerspectiveLUT = AerialPerspective;
	Param->SkyViewSampler = GetSkyViewSampler();
	Param->LinearClampSampler = GetLinearClampSampler();
	Param->Output = GraphBuilder.CreateUAV(Output);

	TShaderMapRef<FVWCompositeCS> CS{ ShaderMap };
	FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("VW.Composite"), CS, Param, FComputeShaderUtils::GetGroupCount(ViewSize, 8));

	// SceneColor 로 복사
	AddCopyTexturePass(GraphBuilder, Output, Inputs.SceneColor, Inputs.ViewRect.Min, Inputs.ViewRect.Min, ViewSize);
}

FRDGTextureRef FVWRenderer::RenderAerialPerspective(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FVWCommonParams& Common, const FViewInputs& Inputs, FRDGTextureRef Transmittance, FRDGTextureRef MultiScattering)
{
	FRDGTextureRef Output = GraphBuilder.CreateTexture(
		FRDGTextureDesc::Create3D(FIntVector(AerialPerspectiveLUTSize), EPixelFormat::PF_FloatRGBA, FClearValueBinding::Black, UAVTextureFlags), TEXT("VW.AerialPerspective"));

	FVWAerialPerspectiveLUTCS::FParameters* Param = GraphBuilder.AllocParameters<FVWAerialPerspectiveLUTCS::FParameters>();
	Param->Common = Common;
	Param->Steps = Settings.Quality.AerialPerspectiveSteps;
	Param->TransmittanceLUT = Transmittance;
	Param->MultiScatteringLUT = MultiScattering;
	Param->LinearClampSampler = GetLinearClampSampler();
	Param->OutLUT = GraphBuilder.CreateUAV(Output);

	TShaderMapRef<FVWAerialPerspectiveLUTCS> CS{ ShaderMap };
	FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("VW.AerialPerspective"), CS, Param, FComputeShaderUtils::GetGroupCount(FIntPoint(AerialPerspectiveLUTSize), 8));
	return Output;
}

uint32 FVWRenderer::GetModeFlags() const
{
	const FVWQualitySettings& Quality = Settings.Quality;
	uint32 Flags = 0;
	if (Quality.bAerialPerspective)
	{
		Flags |= 1;
	}
	return Flags;
}
