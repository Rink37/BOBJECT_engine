#ifndef NORMALGEN
#define NORMALGEN

#include"Bobject_Engine.h"
#include"LoadLists.h"
#include"Meshes.h"
#include"Textures.h"
#include <opencv2/opencv.hpp>

#define MAPDIM 1024
#define MAP_COLOUR_FORMAT VK_FORMAT_R8G8B8A8_UNORM

class NormalGen {
public:
	NormalGen(LoadList* assets) {
		loadList = assets;
	}

	cv::Mat OSNormalMap;
	cv::Mat TSNormalMap;

	struct ObjectSpaceMap {
		VkFramebuffer frameBuffer;
		Texture *colour;
		VkRenderPass renderPass;
	} objectSpaceMap{};

	struct TangentSpaceMap {
		VkFramebuffer frameBuffer;
		Texture *colour;
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
	LoadList* loadList = nullptr;

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