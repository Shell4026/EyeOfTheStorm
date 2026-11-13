#include "VWShaders.h"

IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FVWViewUniformParams, "VWView");

IMPLEMENT_GLOBAL_SHADER(FVWHelloCS, "/Plugin/VW/Private/Hello.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FVWDebugViewCS, "/Plugin/VW/Private/VWDebugView.usf", "MainCS", SF_Compute);
