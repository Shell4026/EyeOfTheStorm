#pragma once

#include <CoreMinimal.h>
#include <GlobalShader.h>
#include <ShaderParameterStruct.h>
#include <RenderGraphResources.h>
#include <DataDrivenShaderPlatformInfo.h>

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FVWViewUniformParams, )
	SHADER_PARAMETER(FMatrix44f, ClipToCamRel)
	SHADER_PARAMETER(FMatrix44f, PrevCamRelToClip)
	SHADER_PARAMETER(FVector3f, CamPosWorld)
	SHADER_PARAMETER(uint32, FrameIndex)
	SHADER_PARAMETER(FVector3f, CamDeltaUE)
	SHADER_PARAMETER(uint32, HistoryValid)
	SHADER_PARAMETER(FIntPoint, ViewRectMin)
	SHADER_PARAMETER(FIntPoint, ViewSize)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FVWAtmosphereUniformParams, )
	SHADER_PARAMETER(float, GroundRadius)
	SHADER_PARAMETER(float, AtmosphereRadius)
	SHADER_PARAMETER(float, MieCoefficient)
	SHADER_PARAMETER(float, MieG)
	SHADER_PARAMETER(FVector3f, MieColor)
	SHADER_PARAMETER(float, SunIlluminance)
	SHADER_PARAMETER(FVector3f, RayleighColor)
	SHADER_PARAMETER(float, APDistanceFactor)
	SHADER_PARAMETER(FVector3f, SunDir)
	SHADER_PARAMETER(uint32, ModeFlags)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FVWCloudUniformParameters, )
	SHADER_PARAMETER(uint32, MinSteps)
	SHADER_PARAMETER(uint32, MaxSteps)
	SHADER_PARAMETER(uint32, LightViewSteps)
	SHADER_PARAMETER(uint32, ModeFlags)
	SHADER_PARAMETER(float, Time)
	SHADER_PARAMETER(float, Tiling)
	SHADER_PARAMETER(float, Tiling2)
	SHADER_PARAMETER(float, ExtinctionCoefficient)
	SHADER_PARAMETER(float, Coverage)
	SHADER_PARAMETER(float, AnvilBias)
	SHADER_PARAMETER(float, Brightness)
	SHADER_PARAMETER(float, DensityPMin)
	SHADER_PARAMETER(float, MaxMarchDistance)
	SHADER_PARAMETER(FVector4f, BezierAB)
	SHADER_PARAMETER(FVector4f, BezierCD)
	SHADER_PARAMETER(FVector2f, WindVelKmh)
	SHADER_PARAMETER(float, DensityPFactor)
	SHADER_PARAMETER(FVector4f, CloudColor)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

BEGIN_SHADER_PARAMETER_STRUCT(FVWCommonParams, )
	SHADER_PARAMETER_STRUCT_REF(FVWViewUniformParams, VWView)
	SHADER_PARAMETER_STRUCT_REF(FVWAtmosphereUniformParams, VWAtmosphere)
	SHADER_PARAMETER_STRUCT_REF(FVWCloudUniformParameters, VWCloud)
END_SHADER_PARAMETER_STRUCT()

class FVWShaderBase : public FGlobalShader
{
public:
	FVWShaderBase() {}
	FVWShaderBase(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

class FVWHelloCS : public FVWShaderBase
{
public:
	DECLARE_GLOBAL_SHADER(FVWHelloCS);
	SHADER_USE_PARAMETER_STRUCT(FVWHelloCS, FVWShaderBase);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTexture)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, Output)
		SHADER_PARAMETER(FIntPoint, ViewRectMin)
		SHADER_PARAMETER(FIntPoint, ViewSize)
		SHADER_PARAMETER(FVector4f, Tint)
	END_SHADER_PARAMETER_STRUCT()
};

class FVWDebugViewCS : public FVWShaderBase
{
public:
	DECLARE_GLOBAL_SHADER(FVWDebugViewCS);
	SHADER_USE_PARAMETER_STRUCT(FVWDebugViewCS, FVWShaderBase);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_INCLUDE(FVWCommonParams, Common)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTex)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTex)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, Output)
		SHADER_PARAMETER(uint32, DebugMode)
	END_SHADER_PARAMETER_STRUCT()
};

class FVWTransmittanceLUTCS : public FVWShaderBase
{
public:
	DECLARE_GLOBAL_SHADER(FVWTransmittanceLUTCS);
	SHADER_USE_PARAMETER_STRUCT(FVWTransmittanceLUTCS, FVWShaderBase);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_INCLUDE(FVWCommonParams, Common)
		SHADER_PARAMETER(uint32, Steps)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutLUT)
	END_SHADER_PARAMETER_STRUCT()
};

class FVWMultiScatteringLUTCS : public FVWShaderBase
{
public:
	DECLARE_GLOBAL_SHADER(FVWMultiScatteringLUTCS);
	SHADER_USE_PARAMETER_STRUCT(FVWMultiScatteringLUTCS, FVWShaderBase);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_INCLUDE(FVWCommonParams, Common)
		SHADER_PARAMETER(uint32, Steps)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D<float4>, TransmittanceLUT)
		SHADER_PARAMETER_SAMPLER(SamplerState, LinearClampSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutLUT)
	END_SHADER_PARAMETER_STRUCT()
};

class FVWSkyViewLUTCS : public FVWShaderBase
{
public:
	DECLARE_GLOBAL_SHADER(FVWSkyViewLUTCS);
	SHADER_USE_PARAMETER_STRUCT(FVWSkyViewLUTCS, FVWShaderBase);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_INCLUDE(FVWCommonParams, Common)
		SHADER_PARAMETER(uint32, Steps)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, TransmittanceLUT)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, MultiScatteringLUT)
		SHADER_PARAMETER_SAMPLER(SamplerState, LinearClampSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutLUT)
	END_SHADER_PARAMETER_STRUCT()
};

class FVWCompositeCS : public FVWShaderBase
{
public:
	DECLARE_GLOBAL_SHADER(FVWCompositeCS);
	SHADER_USE_PARAMETER_STRUCT(FVWCompositeCS, FVWShaderBase);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_INCLUDE(FVWCommonParams, Common)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SkyViewLUT)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, AerialPerspectiveLUT)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, AerialShadowLUT)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, QuarterDepth)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, CloudTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, CloudDepthTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, HalfDepth)
		SHADER_PARAMETER_SAMPLER(SamplerState, LinearClampSampler)
		SHADER_PARAMETER_SAMPLER(SamplerState, SkyViewSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, Output)
	END_SHADER_PARAMETER_STRUCT()
};

class FVWAerialPerspectiveLUTCS : public FVWShaderBase
{
public:
	DECLARE_GLOBAL_SHADER(FVWAerialPerspectiveLUTCS);
	SHADER_USE_PARAMETER_STRUCT(FVWAerialPerspectiveLUTCS, FVWShaderBase);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_INCLUDE(FVWCommonParams, Common)
		SHADER_PARAMETER(uint32, Steps)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, TransmittanceLUT)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, MultiScatteringLUT)
		SHADER_PARAMETER_SAMPLER(SamplerState, LinearClampSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float4>, OutLUT)
	END_SHADER_PARAMETER_STRUCT()
};
class FVWDepthDownsampleCS : public FVWShaderBase
{
public:
	DECLARE_GLOBAL_SHADER(FVWDepthDownsampleCS);
	SHADER_USE_PARAMETER_STRUCT(FVWDepthDownsampleCS, FVWShaderBase);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_INCLUDE(FVWCommonParams, Common)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTexture)
		SHADER_PARAMETER(FIntPoint, OutSize)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, OutDepth)
	END_SHADER_PARAMETER_STRUCT()
};

class FVWAerialShadowCS : public FVWShaderBase
{
public:
	DECLARE_GLOBAL_SHADER(FVWAerialShadowCS);
	SHADER_USE_PARAMETER_STRUCT(FVWAerialShadowCS, FVWShaderBase);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_INCLUDE(FVWCommonParams, Common)
		SHADER_PARAMETER(uint32, Steps)
		SHADER_PARAMETER_RDG_TEXTURE(Texture3D, AerialPerspectiveLUT)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ShadowDepthTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, BlueNoiseTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, LinearClampSampler)
		SHADER_PARAMETER(FVector3f, ShadowOriginUE)
		SHADER_PARAMETER(float, ShadowOrthoWidth)
		SHADER_PARAMETER(FVector3f, ShadowForward)
		SHADER_PARAMETER(float, ShadowDepthBias)
		SHADER_PARAMETER(FVector3f, ShadowRight)
		SHADER_PARAMETER(uint32, bShadowValid)
		SHADER_PARAMETER(FVector3f, ShadowUp)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutLUT)
	END_SHADER_PARAMETER_STRUCT()
};

class FVWCloudCS : public FVWShaderBase
{
public:
	DECLARE_GLOBAL_SHADER(FVWCloudCS);
	SHADER_USE_PARAMETER_STRUCT(FVWCloudCS, FVWShaderBase);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_INCLUDE(FVWCommonParams, Common)
		SHADER_PARAMETER_RDG_TEXTURE(Texture3D, NoiseTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, NoiseSampler)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, BlueNoiseTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, TransmittanceLUT)
		SHADER_PARAMETER_SAMPLER(SamplerState, LinearClampSampler)
		SHADER_PARAMETER(FIntPoint, OutSize)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutCloud)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, OutCloudDepth)
	END_SHADER_PARAMETER_STRUCT()
};

class FVWCloudTRCS : public FVWShaderBase
{
public:
	DECLARE_GLOBAL_SHADER(FVWCloudTRCS);
	SHADER_USE_PARAMETER_STRUCT(FVWCloudTRCS, FVWShaderBase);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_INCLUDE(FVWCommonParams, Common)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, CurCloudTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, CurCloudDepthTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, PrevCloudTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, LinearClampSampler)
		SHADER_PARAMETER(float, HistoryWeight)
		SHADER_PARAMETER(float, NeighborhoodGamma)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, AccumTexture)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutCloud)
	END_SHADER_PARAMETER_STRUCT()
};