#pragma once

#include <CoreMinimal.h>
#include <GlobalShader.h>
#include <ShaderParameterStruct.h>
#include <RenderGraphResources.h>
#include <DataDrivenShaderPlatformInfo.h>

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FVWViewUniformParams, )
	SHADER_PARAMETER(FMatrix44f, ClipToCamRel)
	SHADER_PARAMETER(FIntPoint, ViewRectMin)
	SHADER_PARAMETER(FIntPoint, ViewSize)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

BEGIN_SHADER_PARAMETER_STRUCT(FVWCommonParams, )
	SHADER_PARAMETER_STRUCT_REF(FVWViewUniformParams, VWView)
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
