#pragma once

#include <CoreMinimal.h>
#include <GlobalShader.h>
#include <ShaderParameterStruct.h>
#include <RenderGraphResources.h>
#include <DataDrivenShaderPlatformInfo.h>

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
