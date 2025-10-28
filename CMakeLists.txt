cmake_minimum_required(VERSION 3.21)

project(VulkanRenderer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



add_executable(VulkanRenderer
																		# Source files
				"include/tinyfiledialogs.c"
				"VulkanRenderer.cpp"
				"Bobject_Engine.cpp"
				"InputManager.cpp"
				"FileManager.cpp"
				"CameraController.cpp"
				"UIelements.cpp"
				"Webcam_feeder.cpp"
				"Textures.cpp"
				"Materials.cpp"
				"Meshes.cpp"
				"ExtractNormalMap.cpp"
				"ContextConverter.cpp"
				"MapSpaceTranslator.cpp"
				"SurfaceConstructor.cpp"
				"ImageProcessor.cpp"
				"StudioSession.cpp"
				"Tomography.cpp"
				"include/BakedImages.cpp"
																			# header files
				"Bobject_Engine.h"
				"InputManager.h"
				"FileManager.h"
				"CameraController.h"
				"UIelements.h"
				"Webcam_feeder.h"
				"Pipelines.h"
				"Textures.h"
				"Materials.h"
				"Meshes.h"
				"GenerateNormalMap.h"
				"SurfaceConstructor.h"
				"ImageProcessor.h"
				"StudioSession.h"
				"Tomography.h"
				"LoadLists.h"
				"include/tinyfiledialogs.h"
)

find_package(glfw3 3.4 REQUIRED)
target_link_libraries(VulkanRenderer PUBLIC glfw)

find_package(Vulkan REQUIRED)
target_link_libraries(VulkanRenderer PUBLIC Vulkan::Vulkan)

find_package(OpenCV REQUIRED)
if(TARGET OpenCV::OpenCV)
	target_link_libraries(VulkanRenderer PUBLIC OpenCV::OpenCV)
else()
	target_include_directories(VulkanRenderer PUBLIC ${OpenCV_INCLUDE_DIRS})
	target_link_libraries(VulkanRenderer PUBLIC ${OpenCV_LIBS})
endif()

find_package(glm QUIET)
target_link_libraries(VulkanRenderer PUBLIC glm::glm)

# Project includes
target_include_directories(VulkanRenderer PUBLIC
				${CMAKE_CURRENT_SOURCE_DIR}
				${CMAKE_CURRENT_SOURCE_DIR}/include
)


# Platform-specific settings
if(UNIX AND NOT APPLE)
	find_package(Threads REQUIRED)
	target_link_libraries(VulkanRenderer PUBLIC Threads::Threads dl)
endif()
