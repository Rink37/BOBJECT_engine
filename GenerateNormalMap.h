#ifndef NORMALGEN
#define NORMALGEN

#include"Bobject_Engine.h"
#include"LoadLists.h"
#include"Meshes.h"
#include"Textures.h"
#include <opencv2/opencv.hpp>

#define DEFAULTMAPDIM 1024
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
		VkDescriptorPool descriptorPool;
		VkDescriptorSet descriptorSet;
		VkDescriptorSetLayout descriptorSetLayout;
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
		prepGenerateOSMap();
		createGenerateOSPipeline();
	};

	void cleanupGenOS();

	VkCommandBuffer drawOSMap(VkCommandBuffer, Mesh*);

	void createOSImageFromMat(cv::Mat);
	void createTSImageFromMat(cv::Mat);

	void setupTSConverter() {
		prepareTSMap();
		prepareTSDescriptor();
		createTSPipeline();
	};

	void setupOSConverter() {
		prepareOSMap();
		prepareOSDescriptor();
		createOSPipeline();
	};

	VkCommandBuffer convertOStoTS(VkCommandBuffer, Mesh*);
	VkCommandBuffer convertTStoOS(VkCommandBuffer, Mesh*);

	void cleanupTS();
	void cleanupOS();

private:
	LoadList* loadList = nullptr;

	VkPipelineLayout OSpipelineLayout = nullptr;
	VkPipeline OSpipeline = nullptr;

	void prepGenerateOSMap();
	void createGenerateOSPipeline();

	VkPipelineLayout TSpipelineLayout = nullptr;
	VkPipeline TSpipeline = nullptr;

	void prepareTSMap();
	void prepareTSDescriptor();
	void createTSPipeline();

	void prepareOSMap();
	void prepareOSDescriptor();
	void createOSPipeline();
	
};

#endif