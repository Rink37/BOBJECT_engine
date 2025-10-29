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

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

find_package(glfw 3.4 REQUIRED)
if (GLFW_FOUND)
	message(STATUS "GLFW has been found, linking now")
	include_directories(VulkanRenderer PUBLIC ${GLFW_INCLUDE_DIR})
	target_link_libraries(VulkanRenderer PUBLIC ${GLFW_LIBRARY})
endif()

find_package(Vulkan REQUIRED)
if (VULKAN_FOUND)
	message(STATUS "Vulkan has been found, linking now")
	target_link_libraries(VulkanRenderer PUBLIC Vulkan::Vulkan)
endif()


find_package(OpenCV REQUIRED)
if(TARGET OpenCV::OpenCV)
	target_link_libraries(VulkanRenderer PUBLIC OpenCV::OpenCV)
else()
	if (OPENCV_FOUND)
		message(STATUS "OpenCV found, linking now")
		target_include_directories(VulkanRenderer PUBLIC ${OPENCV_INCLUDE_DIR}/../)
		target_link_libraries(VulkanRenderer PUBLIC ${OPENCV_LIBRARY}/../opencv_world4100d.lib)
	endif()
endif()

find_package(glm REQUIRED)
if (GLM_FOUND)
	message(STATUS "GLM found, linking now")
	target_include_directories(VulkanRenderer PUBLIC ${GLM_INCLUDE_DIRS})
endif ()

target_include_directories(VulkanRenderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)

#target_include_directories(VulkanRenderer PUBLIC "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.42.34433\\glfw-3.4.bin.WIN64\\include")
#target_include_directories(VulkanRenderer PUBLIC "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.42.34433\\GLM\\g-truc-glm-0af55cc")
#target_include_directories(VulkanRenderer PUBLIC "C:\\VulkanSDK\\1.3.296.0\\Include")
#target_include_directories(VulkanRenderer PUBLIC "C:\\Users\\robda\\Documents\\VulkanRenderer\\include")

#target_link_libraries(VulkanRenderer PUBLIC "C:\\VulkanSDK\\1.3.296.0\\Lib\\vulkan-1.lib")
#target_link_libraries(VulkanRenderer PUBLIC "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.42.34433\\glfw-3.4.bin.WIN64\\lib-vc2022\\glfw3.lib")

#set(OpenCV_DIR C:/Users/robda/Documents/opencv/build/)
#find_package(OpenCV REQUIRED)
#target_include_directories(VulkanRenderer PUBLIC ${OpenCV_INCLUDE_DIRS})
#target_link_libraries(VulkanRenderer PRIVATE ${OpenCV_LIBS})

#target_link_libraries(VulkanRenderer PUBLIC "C:\\Users\\robda\\Documents\\opencv\\build\\x64\\vc16\\lib\\opencv_world4120d.lib")

add_custom_command(TARGET VulkanRenderer POST_BUILD
				   COMMAND ${CMAKE_COMMAND} -E copy
				   ${OPENCV_DLL_DIR}/bin/opencv_world4100d.dll
				   $<TARGET_FILE_DIR:VulkanRenderer>)