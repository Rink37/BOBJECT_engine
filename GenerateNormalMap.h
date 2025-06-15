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
	cv::Mat TSNormalMap;

	struct ObjectSpaceMap {
		uint32_t width, height;
		VkFramebuffer frameBuffer;
		FrameBufferAttachment colour;
		VkRenderPass renderPass;
	} objectSpaceMap{};

	struct TangentSpaceMap {
		uint32_t width, height;
		VkFramebuffer frameBuffer;
		FrameBufferAttachment colour;
		VkRenderPass renderPass;
		VkDescriptorPool descriptorPool;
		VkDescriptorSet descriptorSet;
		VkDescriptorSetLayout descriptorSetLayout;
	} tangentSpaceMap{};

	void setupOSExtractor() {
		prepareOSMap();
		createOSPipeline();
	};

	void cleanupOS();

	VkCommandBuffer drawOSMap(VkCommandBuffer, Mesh*);

	void createOSImageFromMat(cv::Mat);

	void setupTSExtractor() {
		prepareTSMap();
		prepareTSDescriptor();
		createTSPipeline();
	};

	void contextualConvertMap(cv::Mat);

	VkCommandBuffer convertOStoTS(VkCommandBuffer, Mesh*);

	void cleanupTS();

private:
	VkPipelineLayout OSpipelineLayout;
	VkPipeline OSpipeline;

	void prepareOSMap();
	void createOSPipeline();

	VkPipelineLayout TSpipelineLayout;
	VkPipeline TSpipeline;

	void prepareTSMap();
	void prepareTSDescriptor();
	void createTSPipeline();
	
};

#endif