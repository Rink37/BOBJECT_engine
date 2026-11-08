#ifndef OS_EDGEFILLDATA
#define OS_EDGEFILLDATA
#include "ShaderDataType.h"

const bool OS_EdgeFillWireframe = false;

const std::string OS_EdgeFillName = "OS_EdgeFill";

#endif

#ifndef OS_EDGEFILLSHADER
#define OS_EDGEFILLSHADER shaderData( &OS_EdgeFillFragData, &OS_EdgeFillVertData, OS_EdgeFillWireframe, OS_EdgeFillName )
#endif
