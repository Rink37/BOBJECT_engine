#!/usr/bin/sh
glslc NormalGenerator.vert -o NormalGeneratorVert.spv
glslc NormalGenerator.frag -o NormalGeneratorFrag.spv

glslc NormalConvertor.vert -o NormalConvertorVert.spv
glslc NormalConvertor.frag -o NormalConvertorFrag.spv

glslc FilterDefault.comp -o FilterDefaultVert.spv
#glslc FilterDefault.frag -o FilterDefaultFrag.spv

glslc Kuwahara.comp -o KuwaharaVert.spv
#glslc FilterDefault.frag -o KuwaharaFrag.spv
