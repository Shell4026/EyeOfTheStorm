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
	FRHISamplerState* GetNoiseSampler()
	{
		return TStaticSamplerState<ESamplerFilter::SF_Trilinear, ESamplerAddressMode::AM_Wrap, ESamplerAddressMode::AM_Wrap, ESamplerAddressMode::AM_Wrap>::GetRHI();
	}
	// 밉맵 생성용
	TArray<uint8> DownsampleNoise(const TArray<uint8>& Src, uint32 Size)
	{
		const uint32 Half = Size >> 1;
		TArray<uint8> Dst;
		Dst.SetNumZeroed(Half * Half * Half);
		for (uint32 Z = 0; Z < Half; ++Z)
		{
			for (uint32 Y = 0; Y < Half; ++Y)
			{
				for (uint32 X = 0; X < Half; ++X)
				{
					uint32 Sum = 0;
					for (uint32 DZ = 0; DZ < 2; ++DZ)
						for (uint32 DY = 0; DY < 2; ++DY)
							for (uint32 DX = 0; DX < 2; ++DX)
								Sum += Src[(X * 2 + DX) + Size * (Y * 2 + DY) + Size * Size * (Z * 2 + DZ)];
					Dst[X + Half * Y + Half * Half * Z] = static_cast<uint8>(Sum / 8);
				}
			}
		}
		return Dst;
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
}

void FVWRenderer::InitCloudNoise_RenderThread(FRHICommandListImmediate& RHICmdList, TArray<uint8> PerlinWorley, TArray<uint8> Worley0, TArray<uint8> Worley1, TArray<uint8> Worley2)
{
	check(IsInRenderingThread());
	constexpr uint32 BaseSize = 128;
	constexpr uint32 NumMips = 8;

	const int32 TexelCount = BaseSize * BaseSize * BaseSize;
	if (PerlinWorley.Num() != TexelCount || Worley0.Num() != TexelCount || Worley1.Num() != TexelCount || Worley2.Num() != TexelCount)
	{
		UE_LOG(LogTemp, Error, TEXT("[VW] 노이즈 데이터 크기가 잘못되었습니다. (기대값 %d)"), TexelCount);
		return;
	}

	const FRHITextureCreateDesc Desc = FRHITextureCreateDesc::Create3D(TEXT("VW.CloudNoise"), BaseSize, BaseSize, BaseSize, EPixelFormat::PF_R8G8B8A8)
		.SetNumMips(NumMips)
		.SetFlags(ETextureCreateFlags::ShaderResource)
		.SetInitialState(ERHIAccess::SRVMask);
	CloudNoise = RHICmdList.CreateTexture(Desc);

	uint32 Size = BaseSize;
	TArray<uint8> Texels;
	for (uint32 Mip = 0; Mip < NumMips; ++Mip)
	{
		const uint32 Count = Size * Size * Size;
		Texels.SetNumUninitialized(Count * 4);
		for (uint32 i = 0; i < Count; ++i)
		{
			Texels[i * 4 + 0] = PerlinWorley[i];
			Texels[i * 4 + 1] = Worley0[i];
			Texels[i * 4 + 2] = Worley1[i];
			Texels[i * 4 + 3] = Worley2[i];
		}

		const FUpdateTextureRegion3D Region(0, 0, 0, 0, 0, 0, Size, Size, Size);
		RHICmdList.UpdateTexture3D(CloudNoise, Mip, Region, Size * 4, Size * Size * 4, Texels.GetData());

		if (Mip + 1 < NumMips)
		{
			PerlinWorley = DownsampleNoise(PerlinWorley, Size);
			Worley0 = DownsampleNoise(Worley0, Size);
			Worley1 = DownsampleNoise(Worley1, Size);
			Worley2 = DownsampleNoise(Worley2, Size);
			Size >>= 1;
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[VW] 구름 노이즈 3D 텍스처 생성 완료"));
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

	FViewState& State = ViewStates.FindOrAdd(View.GetViewKey());
	const FVWCommonParams CommonParams = CreateCommonParams(View, Inputs.ViewRect, State);
	const int32 DebugMode = CVarVWDebugView.GetValueOnRenderThread();
	if (DebugMode != 0)
	{
		RenderDebugView(GraphBuilder, ShaderMap, Inputs, CommonParams, DebugMode);
	}
	else
	{
		const uint32 ModeFlags = GetModeFlags();
		FRDGTextureRef BlueNoiseTex = BlueNoise.IsValid()
			? RegisterExternalTexture(GraphBuilder, BlueNoise, TEXT("VW.BlueNoise"))
			: GSystemTextures.GetBlackDummy(GraphBuilder);

		StaticLUT StaticLUTs{};
		RenderStaticLUTs(GraphBuilder, ShaderMap, CommonParams, StaticLUTs);
		FRDGTextureRef SkyViewTex = RenderSkyViewLUT(GraphBuilder, ShaderMap, CommonParams, StaticLUTs.Transmittance, StaticLUTs.MultiScattering);
		FRDGTextureRef AerialPerspectiveTex = (ModeFlags & 1)
			? RenderAerialPerspective(GraphBuilder, ShaderMap, CommonParams, Inputs, StaticLUTs.Transmittance, StaticLUTs.MultiScattering)
			: GSystemTextures.GetVolumetricBlackDummy(GraphBuilder);
		FRDGTextureRef QuarterDepth = RenderLowResDepth(GraphBuilder, ShaderMap, CommonParams, Inputs, 4);
		FRDGTextureRef HalfDepth = RenderLowResDepth(GraphBuilder, ShaderMap, CommonParams, Inputs, 2);
		FRDGTextureRef AerialShadowTex = (ModeFlags & 2)
			? RenderAerialShadow(GraphBuilder, ShaderMap, CommonParams, Inputs, AerialPerspectiveTex, BlueNoiseTex)
			: GSystemTextures.GetBlackDummy(GraphBuilder);
		
		FRDGTextureRef CloudTex = GSystemTextures.GetBlackAlphaOneDummy(GraphBuilder);
		FRDGTextureRef CloudDepthTex = GSystemTextures.GetBlackDummy(GraphBuilder);
		if (ModeFlags & 4)
			RenderCloud(GraphBuilder, ShaderMap, CommonParams, Inputs, State, StaticLUTs.Transmittance, BlueNoiseTex, CloudTex, CloudDepthTex);

		RenderComposite(GraphBuilder, ShaderMap, CommonParams, Inputs, SkyViewTex, AerialPerspectiveTex, AerialShadowTex, QuarterDepth, CloudTex, CloudDepthTex, HalfDepth);

		const FViewMatrices& VM = View.ViewMatrices;
		const FVector CamInTranslated = VM.GetViewOrigin() + VM.GetPreViewTranslation();
		State.PrevCamRelToClip = static_cast<FMatrix44f>(FTranslationMatrix(CamInTranslated) * VM.GetTranslatedViewProjectionMatrix());
		State.PrevViewOrigin = VM.GetViewOrigin();
		State.PrevViewLocation = View.ViewLocation;
		State.PrevViewRotation = View.ViewRotation;
		State.PrevViewSize = ViewSize;
		State.bHasPrevFrame = true;
		State.FrameIndex++;
		State.LastRenderedFrame = GFrameCounterRenderThread;
	}

	// 오래 안 쓰인 뷰 제거
	for (auto It = ViewStates.CreateIterator(); It; ++It)
	{
		if (GFrameCounterRenderThread - It.Value().LastRenderedFrame > 120)
		{
			It.RemoveCurrent();
		}
	}
}

FVWCommonParams FVWRenderer::CreateCommonParams(const FSceneView& View, const FIntRect& ViewRect, const FViewState& State) const
{
	const FViewMatrices& ViewMat = View.ViewMatrices;

	const FMatrix ClipToCamRelMat = ViewMat.GetInvTranslatedViewProjectionMatrix();

	const FIntPoint ViewSize = ViewRect.Size();
	const bool bSizeChanged = State.PrevViewSize != ViewSize;
	const bool bMoved = !View.ViewLocation.Equals(State.PrevViewLocation, 0.1)
		|| !View.ViewRotation.Equals(State.PrevViewRotation, 0.01f);
	// 0 = 무효, 1 = 움직이는중, 2 = 정지 상태
	uint32 HistoryValid = 2;
	if (!State.bHasPrevFrame || bSizeChanged || View.bCameraCut)
		HistoryValid = 0;
	else if (bMoved)
		HistoryValid = 1;

	FVWViewUniformParams ViewUB;
	ViewUB.ClipToCamRel = static_cast<FMatrix44f>(ClipToCamRelMat);
	ViewUB.PrevCamRelToClip = State.bHasPrevFrame ? State.PrevCamRelToClip : ViewUB.ClipToCamRel;
	ViewUB.CamPosWorld = VWSpace::UEToWeatherPos(ViewMat.GetViewOrigin());
	ViewUB.CamDeltaUE = State.bHasPrevFrame ? FVector3f{ ViewMat.GetViewOrigin() - State.PrevViewOrigin } : FVector3f::ZeroVector;
	ViewUB.FrameIndex = State.FrameIndex;
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

	const FVWCloudSettings& C = Settings.Cloud;
	FVWCloudUniformParameters CloudUB;
	CloudUB.MinSteps = C.MinSteps;
	CloudUB.MaxSteps = C.MaxSteps;
	CloudUB.LightViewSteps = C.LightViewSteps;
	CloudUB.ModeFlags = C.ModeFlags;
	CloudUB.Time = C.TimeHours;
	CloudUB.Tiling = C.Tiling;
	CloudUB.Tiling2 = C.Tiling2;
	CloudUB.ExtinctionCoefficient = C.ExtinctionCoefficient;
	CloudUB.Coverage = C.Coverage;
	CloudUB.AnvilBias = C.AnvilBias;
	CloudUB.Brightness = C.Brightness;
	CloudUB.DensityPMin = C.DensityPMin;
	CloudUB.MaxMarchDistance = C.MaxMarchDistance;
	CloudUB.BezierAB = FVector4f(C.BezierA.X, C.BezierA.Y, C.BezierB.X, C.BezierB.Y);
	CloudUB.BezierCD = FVector4f(C.BezierC.X, C.BezierC.Y, C.BezierD.X, C.BezierD.Y);
	CloudUB.WindVelKmh = C.WindVelKmh;
	CloudUB.DensityPFactor = C.DensityPFactor;
	CloudUB.CloudColor = FVector4f(C.CloudColor.R, C.CloudColor.G, C.CloudColor.B, C.CloudColor.A);

	FVWCommonParams CommonParams;
	CommonParams.VWView = TUniformBufferRef<FVWViewUniformParams>::CreateUniformBufferImmediate(ViewUB, EUniformBufferUsage::UniformBuffer_SingleFrame);
	CommonParams.VWAtmosphere = TUniformBufferRef<FVWAtmosphereUniformParams>::CreateUniformBufferImmediate(AtmoUB, EUniformBufferUsage::UniformBuffer_SingleFrame);
	CommonParams.VWCloud = TUniformBufferRef<FVWCloudUniformParameters>::CreateUniformBufferImmediate(CloudUB, UniformBuffer_SingleFrame);
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
	Output.MultiScattering = MultiScattering;
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

void FVWRenderer::RenderComposite(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FVWCommonParams& Common, const FViewInputs& Inputs, 
	FRDGTextureRef SkyView, FRDGTextureRef AerialPerspective, FRDGTextureRef AerialShadow, FRDGTextureRef QuarterDepth,
	FRDGTextureRef CloudTex, FRDGTextureRef CloudDpethTex, FRDGTextureRef HalfDepth)
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
	Param->AerialShadowLUT = AerialShadow;
	Param->QuarterDepth = QuarterDepth;
	Param->HalfDepth = HalfDepth;
	Param->CloudTexture = CloudTex;
	Param->CloudDepthTexture = CloudDpethTex;
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

FRDGTextureRef FVWRenderer::RenderLowResDepth(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FVWCommonParams& Common, const FViewInputs& Inputs, int32 Divider)
{
	const FIntPoint OutSize = FIntPoint::DivideAndRoundUp(Inputs.ViewRect.Size(), Divider);
	FRDGTextureRef LowDepth = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(OutSize, EPixelFormat::PF_R32_FLOAT, FClearValueBinding::None, UAVTextureFlags),
		Divider == 2 ? TEXT("VW.HalfDepth") : TEXT("VW.QuarterDepth"));

	FVWDepthDownsampleCS::FParameters* Param = GraphBuilder.AllocParameters<FVWDepthDownsampleCS::FParameters>();
	Param->Common = Common;
	Param->SceneDepthTexture = Inputs.SceneDepth;
	Param->OutSize = OutSize;
	Param->OutDepth = GraphBuilder.CreateUAV(LowDepth);

	TShaderMapRef<FVWDepthDownsampleCS> CS{ ShaderMap };
	FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("VW.DepthDownsample 1/%d", Divider), CS, Param, FComputeShaderUtils::GetGroupCount(OutSize, 16));
	return LowDepth;
}

FRDGTextureRef FVWRenderer::RenderAerialShadow(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FVWCommonParams& Common, const FViewInputs& Inputs, FRDGTextureRef AerialPerspective, FRDGTextureRef BlueNoiseTexture)
{
	const FIntPoint OutSize = FIntPoint::DivideAndRoundUp(Inputs.ViewRect.Size(), 4); // 1/4로 렌더링
	FRDGTextureRef AerialShadow = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(OutSize, EPixelFormat::PF_FloatRGBA, FClearValueBinding::None, UAVTextureFlags), TEXT("VW.AerialShadow"));

	FVWAerialShadowCS::FParameters* Param = GraphBuilder.AllocParameters<FVWAerialShadowCS::FParameters>();
	Param->Common = Common;
	Param->Steps = Settings.Quality.AerialShadowSteps;
	Param->AerialPerspectiveLUT = AerialPerspective;
	Param->ShadowDepthTexture = RegisterExternalTexture(GraphBuilder, Shadow.DepthTexture, TEXT("VW.ShadowDepth"));
	Param->SceneDepthTexture = Inputs.SceneDepth;
	Param->BlueNoiseTexture = BlueNoiseTexture;
	Param->LinearClampSampler = GetLinearClampSampler();
	Param->ShadowOriginUE = Shadow.OriginUE;
	Param->ShadowOrthoWidth = Shadow.OrthoWidth;
	Param->ShadowForward = Shadow.Forward;
	Param->ShadowDepthBias = Shadow.DepthBias;
	Param->ShadowRight = Shadow.Right;
	Param->bShadowValid = 1;
	Param->ShadowUp = Shadow.Up;
	Param->OutLUT = GraphBuilder.CreateUAV(AerialShadow);

	TShaderMapRef<FVWAerialShadowCS> CS{ ShaderMap };
	FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("VW.AerialShadow"), CS, Param, FComputeShaderUtils::GetGroupCount(OutSize, 16));
	return AerialShadow;
}

void FVWRenderer::RenderCloud(FRDGBuilder& GraphBuilder, FGlobalShaderMap* ShaderMap, const FVWCommonParams& Common, const FViewInputs& Inputs, FViewState& State, FRDGTextureRef Transmittance, FRDGTextureRef BlueNoiseTexture, FRDGTextureRef& OutCloud, FRDGTextureRef& OutCloudDepth)
{
	RDG_EVENT_SCOPE(GraphBuilder, "VW.Cloud");

	const FIntPoint HalfSize = FIntPoint::DivideAndRoundUp(Inputs.ViewRect.Size(), 2);
	auto IsReusable = [&HalfSize](const TRefCountPtr<IPooledRenderTarget>& Target)
		{
			return Target.IsValid() && Target->GetDesc().Extent == HalfSize;
		};

	FRDGTextureRef CloudRaw = nullptr;
	FRDGTextureRef CloudRawDepth = nullptr;
	if (IsReusable(State.CloudRaw) && IsReusable(State.CloudRawDepth))
	{
		CloudRaw = GraphBuilder.RegisterExternalTexture(State.CloudRaw);
		CloudRawDepth = GraphBuilder.RegisterExternalTexture(State.CloudRawDepth);
	}
	else
	{
		// 처음이거나 창 크기가 바뀌었으면 새로 만들고 초기화
		CloudRaw = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(
			HalfSize, EPixelFormat::PF_FloatRGBA, FClearValueBinding::None, UAVTextureFlags), TEXT("VW.CloudRaw"));
		CloudRawDepth = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(
			HalfSize, EPixelFormat::PF_R32_FLOAT, FClearValueBinding::None, UAVTextureFlags), TEXT("VW.CloudRawDepth"));
		AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(CloudRaw), FLinearColor(0.f, 0.f, 0.f, 1.f));
		AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(CloudRawDepth), -1.f);
	}
	{
		FVWCloudCS::FParameters* Param = GraphBuilder.AllocParameters<FVWCloudCS::FParameters>();
		Param->Common = Common;
		Param->NoiseTexture = RegisterExternalTexture(GraphBuilder, CloudNoise, TEXT("VW.CloudNoise"));
		Param->NoiseSampler = GetNoiseSampler();
		Param->BlueNoiseTexture = BlueNoiseTexture;
		Param->SceneDepthTexture = Inputs.SceneDepth;
		Param->TransmittanceLUT = Transmittance;
		Param->LinearClampSampler = GetLinearClampSampler();
		Param->OutSize = HalfSize;
		Param->OutCloud = GraphBuilder.CreateUAV(CloudRaw);
		Param->OutCloudDepth = GraphBuilder.CreateUAV(CloudRawDepth);

		TShaderMapRef<FVWCloudCS> CS(ShaderMap);
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("VW.CloudRaymarch"), CS, Param, FComputeShaderUtils::GetGroupCount(FIntPoint::DivideAndRoundUp(HalfSize, 2), 16));
	}
	// 이 뷰만의 텍스처라 지연 추출로 충분
	GraphBuilder.QueueTextureExtraction(CloudRaw, &State.CloudRaw);
	GraphBuilder.QueueTextureExtraction(CloudRawDepth, &State.CloudRawDepth);

	FRDGTextureRef PrevCloud = IsReusable(State.CloudHistory)
		? GraphBuilder.RegisterExternalTexture(State.CloudHistory)
		: GSystemTextures.GetBlackAlphaOneDummy(GraphBuilder);

	FRDGTextureRef Accum = nullptr;
	if (IsReusable(State.CloudAccum))
	{
		Accum = GraphBuilder.RegisterExternalTexture(State.CloudAccum);
	}
	else
	{
		// 원본은 r8ui. UAV 로드 호환성을 위해 R32_UINT 사용
		Accum = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(
			HalfSize, EPixelFormat::PF_R32_UINT, FClearValueBinding::None, UAVTextureFlags), TEXT("VW.CloudAccum"));
		AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(Accum), 0u);
	}

	FRDGTextureRef CloudHistory = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(
		HalfSize, EPixelFormat::PF_FloatRGBA, FClearValueBinding::None, UAVTextureFlags), TEXT("VW.CloudHistory"));
	{
		FVWCloudTRCS::FParameters* Param = GraphBuilder.AllocParameters<FVWCloudTRCS::FParameters>();
		Param->Common = Common;
		Param->CurCloudTexture = CloudRaw;
		Param->CurCloudDepthTexture = CloudRawDepth;
		Param->PrevCloudTexture = PrevCloud;
		Param->LinearClampSampler = GetLinearClampSampler();
		Param->HistoryWeight = Settings.Quality.HistoryWeight;
		Param->NeighborhoodGamma = Settings.Quality.NeighborhoodGamma;
		Param->AccumTexture = GraphBuilder.CreateUAV(Accum);
		Param->OutCloud = GraphBuilder.CreateUAV(CloudHistory);

		TShaderMapRef<FVWCloudTRCS> CS{ ShaderMap };
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("VW.CloudTR"), CS, Param, FComputeShaderUtils::GetGroupCount(HalfSize, 16));
	}
	GraphBuilder.QueueTextureExtraction(CloudHistory, &State.CloudHistory);
	GraphBuilder.QueueTextureExtraction(Accum, &State.CloudAccum);

	OutCloud = CloudHistory;
	OutCloudDepth = CloudRawDepth;
}

uint32 FVWRenderer::GetModeFlags() const
{
	const FVWQualitySettings& Quality = Settings.Quality;
	uint32 Flags = 0;
	if (Quality.bAerialPerspective)
		Flags |= 1;
	if (Quality.bVolumetricShadow && Shadow.DepthTexture.IsValid())
		Flags |= 2;
	if (Quality.bCloud && CloudNoise.IsValid())
		Flags |= 4;
	return Flags;
}
