cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project (VulkanRenderer LANGUAGES C CXX)

# Add source to this project's executable.
add_executable (VulkanRenderer "VulkanRenderer.cpp" "Bobject_Engine.h" "Bobject_Engine.cpp" "InputManager.h" "InputManager.cpp" "WindowsFileManager.h" "WindowsFileManager.cpp" "CameraController.cpp" "CameraController.h" "UIelements.cpp" "UIelements.h" "Webcam_feeder.cpp" "Webcam_feeder.h" "Pipelines.h" "Textures.h" "Textures.cpp" "Materials.h" "Materials.cpp" "Meshes.h" "Meshes.cpp" "GenerateNormalMap.h" "ExtractNormalMap.cpp" "ContextConverter.cpp" "MapSpaceTranslator.cpp" "SurfaceConstructor.cpp" "SurfaceConstructor.h" "ImageProcessor.h" "ImageProcessor.cpp" "StudioSession.h" "StudioSession.cpp" "Tomography.cpp" "Tomography.h" "include/BakedImages.cpp" "LoadLists.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET VulkanRenderer PROPERTY CXX_STANDARD 20)
endif()

# Set the directory where cmake will search for Find{module}.cmake files
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

find_package(glfw 3.4 REQUIRED)
if (GLFW_FOUND)
	message(STATUS "GLFW has been found, linking now")
	include_directories(VulkanRenderer PUBLIC ${GLFW_INCLUDE_DIR})
	target_link_libraries(VulkanRenderer PUBLIC ${GLFW_LIBRARY})
else()
	message(STATUS "GLFW has not been found, cannot link. Ensure that GLFW is installed and can be found in one of the search folders in Findglfw.cmake")
endif()

find_package(Vulkan REQUIRED)
if (VULKAN_FOUND)
	message(STATUS "Vulkan has been found, linking now")
	message(STATUS ${Vulkan_INCLUDE_DIRS})
	target_link_libraries(VulkanRenderer PUBLIC Vulkan::Vulkan)
else()
	message(STATUS "Vulkan has not been found, cannot link. Ensure that the VulkanSDK is installed and can be found in one of the search folders in FindVulkan.cmake")
endif()


find_package(OpenCV REQUIRED)
if (OPENCV_FOUND)
	if(TARGET OpenCV::OpenCV)
		target_link_libraries(VulkanRenderer PUBLIC OpenCV::OpenCV)
	else()
		message(STATUS "OpenCV found, linking now")
		target_include_directories(VulkanRenderer PUBLIC ${OPENCV_INCLUDE_DIR})# /../) # Sometimes the include directory has opencv2 in it which will break the imports - check the cmake message log, if OPENCV_INCLUDE_DIR ends in /opencv2 then uncomment the /../)
		target_link_libraries(VulkanRenderer PUBLIC ${OPENCV_LIBRARY})#/../opencv_world4100d.lib) # Similarly if the OPENCV_LIBRARY doesn't end with the lib then uncomment the last bit of this line'
	endif()
else()
	message(STATUS "OpenCV has not been found, cannot link. Ensure that OpenCV is installed and can be found in one of the search folders in FindOpenCV.cmake")
endif()

find_package(glm REQUIRED)
if (GLM_FOUND)
	message(STATUS "GLM found, linking now")
	target_include_directories(VulkanRenderer PUBLIC ${GLM_INCLUDE_DIRS})
else()
	message(STATUS "GLM has not been found, cannot link. Ensure that GLM is installed and can be found in one of the search folders in Findglm.cmake")
endif ()

# Ensure that we can find all files used in the main application
target_include_directories(VulkanRenderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_custom_command(TARGET VulkanRenderer POST_BUILD
				   COMMAND ${CMAKE_COMMAND} -E copy
				   ${OPENCV_DLL_DIR}#/bin/opencv_world4100d.dll # Sometimes the path will terminate one directory before /bin - in that case replace the end segment with this
				   $<TARGET_FILE_DIR:VulkanRenderer>)