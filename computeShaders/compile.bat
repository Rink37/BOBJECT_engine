C:/VulkanSDK/1.3.296.0/Bin/glslc.exe NormalGenerator.vert -o NormalGeneratorVert.spv
C:/VulkanSDK/1.3.296.0/Bin/glslc.exe NormalGenerator.frag -o NormalGeneratorFrag.spv

C:/VulkanSDK/1.3.296.0/Bin/glslc.exe NormalConvertor.vert -o NormalConvertorVert.spv
C:/VulkanSDK/1.3.296.0/Bin/glslc.exe NormalConvertor.frag -o NormalConvertorFrag.spv

C:/VulkanSDK/1.3.296.0/Bin/glslc.exe FilterDefault.comp -o FilterDefaultVert.spv
C:/VulkanSDK/1.3.296.0/Bin/glslc.exe NormalConvertor.frag -o FilterDefaultFrag.spv

C:/VulkanSDK/1.3.296.0/Bin/glslc.exe Kuwahara.comp -o KuwaharaVert.spv
C:/VulkanSDK/1.3.296.0/Bin/glslc.exe NormalConvertor.frag -o KuwaharaFrag.spv

C:/VulkanSDK/1.3.296.0/Bin/glslc.exe Averager.comp -o AveragerVert.spv
C:/VulkanSDK/1.3.296.0/Bin/glslc.exe NormalConvertor.frag -o AveragerFrag.spv

C:/VulkanSDK/1.3.296.0/Bin/glslc.exe GradRemap.comp -o GradRemapVert.spv
C:/VulkanSDK/1.3.296.0/Bin/glslc.exe NormalConvertor.frag -o GradRemapFrag.spv