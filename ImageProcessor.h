#ifndef IMAGEPROCESSOR
#define IMAGEPROCESSOR

#include"Bobject_Engine.h"
#include"Textures.h"
#include"ShaderDataType.h"

#define OIOO 0
#define TIOO 1

class filter {
public:
	filter(Texture* src, shaderData* sd) {
		src->getCVMat();
		cv::Mat srcMat = src->texMat;
		source = new imageTexture(srcMat, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1);
		texWidth = source->texWidth;
		texHeight = source->texHeight;

		filterShaderModule = Engine::get()->createShaderModule(sd->vertData);

		createFilterTarget();
		createDescriptorSetLayout();
		createDescriptorSet();
		createFilterPipelineLayout();
		createFilterPipeline();
	}

	void filterImage();
private:
	uint32_t filtertype = 0;
	uint32_t texWidth = 0;
	uint32_t texHeight = 0;

	VkPipelineLayout filterPipelineLayout = nullptr;
	VkPipeline filterPipeline = nullptr;

	VkDescriptorPool descPool = nullptr;
	VkDescriptorSetLayout filterDescriptorSetLayout = nullptr;
	VkDescriptorSet filterDescriptorSet;

	VkShaderModule filterShaderModule = nullptr; 

	Texture* source = nullptr;
	Texture* filterTarget = nullptr;

	void createDescriptorSetLayout();
	void createDescriptorSet();

	void createFilterPipelineLayout();
	void createFilterPipeline();

	void createFilterTarget();
};

#endif