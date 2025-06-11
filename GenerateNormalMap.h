#ifndef NORMALGEN
#define NORMALGEN

#include"Bobject_Engine.h"
#include"Meshes.h"
#include <opencv2/opencv.hpp>

#define MAPDIM 1024
#define MAP_COLOUR_FORMAT VK_FORMAT_R8G8B8A8_UNORM

struct FrameBufferAttachment {
	VkImage image;
	VkDeviceMemory mem;
	VkImageView view;
};

class NormalGen {
public:
	cv::Mat OSNormalMap;

	struct ObjectSpaceMap {
		uint32_t width, height;
		VkFramebuffer frameBuffer;
		FrameBufferAttachment colour, depth;
		VkRenderPass renderPass;
		VkDescriptorImageInfo descriptor;
	} objectSpaceMap{};

	void setup() {
		prepareMap();
		createPipeline();
	};

	void cleanup();

	VkCommandBuffer draw(VkCommandBuffer, Mesh*);
	void contextualConvertMap(cv::Mat);

private:
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

	void prepareMap();
	void createPipeline();
};

#endif