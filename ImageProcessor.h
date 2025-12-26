#ifndef IMAGEPROCESSOR
#define IMAGEPROCESSOR

#include"Bobject_Engine.h"
#include<chrono>
#include"Textures.h"
#include"ShaderDataType.h"

#define OIOO 0
#define TIOO 1
#define THIOO 2

class filter {
public:
	filter(std::vector<Texture*> srcs, shaderData* sd) {
		switch (srcs.size()) {
		case(1):
			filtertype = OIOO;
			break;
		case(2):
			filtertype = TIOO;
			break;
		case(3):
			filtertype = THIOO;
			break;
		default:
			filtertype = OIOO;
			break;
		}
		for (Texture* src : srcs) {
			source.push_back(src->copyTexture(src->textureFormat, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1));
		}

		texWidth = source[0]->texWidth;
		texHeight = source[0]->texHeight;

		filterShaderModule = Engine::get()->createShaderModule(sd->vertData);

		createFilterTarget();
		createDescriptorSetLayout();
		createDescriptorSet();
		createFilterPipelineLayout();
		createFilterPipeline();
	};

	filter(std::vector<Texture*> srcs, shaderData* sd, VkFormat outFormat) {
		switch (srcs.size()) {
		case (1):
			filtertype = OIOO;
			break;
		case(2):
			filtertype = TIOO;
			break;
		case(3):
			filtertype = THIOO;
			break;
		default:
			filtertype = OIOO;
			break;
		}
		for (Texture* src : srcs) {
			source.push_back(src->copyTexture(src->textureFormat, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1));
		}

		texWidth = source[0]->texWidth;
		texHeight = source[0]->texHeight;

		filterShaderModule = Engine::get()->createShaderModule(sd->vertData);

		targetFormat = outFormat;

		createFilterTarget();
		createDescriptorSetLayout();
		createDescriptorSet();
		createFilterPipelineLayout();
		createFilterPipeline();
	};

	filter(std::vector<Texture*> srcs, shaderData* sd, VkFormat outFormat, VkBuffer buffer, uint32_t bufferSize) {
		switch (srcs.size()) {
		case (1):
			filtertype = OIOO;
			break;
		case(2):
			filtertype = TIOO;
			break;
		case(3):
			filtertype = THIOO;
			break;
		default:
			filtertype = OIOO;
			break;
		}
		for (Texture* src : srcs) {
			source.push_back(src->copyTexture(src->textureFormat, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1));
		}

		hasUniformBuffer = true;
		bufferRef = buffer;
		this->bufferSize = bufferSize;

		texWidth = source[0]->texWidth;
		texHeight = source[0]->texHeight;

		filterShaderModule = Engine::get()->createShaderModule(sd->vertData);

		targetFormat = outFormat;

		createFilterTarget();
		createDescriptorSetLayout();
		createDescriptorSet();
		createFilterPipelineLayout();
		createFilterPipeline();
	};

	void filterImage();

	void filterImage(VkCommandBuffer);

	void cleanup() {
		for (Texture* tex : source) {
			if (!tex->cleaned) {
				tex->cleanup();
			}
		}
		for (Texture* tex : filterTarget) {
			if (!tex->cleaned) {
				tex->cleanup();
			}
		}
		vkDeviceWaitIdle(Engine::get()->device);

		vkDestroyPipeline(Engine::get()->device, filterPipeline, nullptr);
		vkDestroyPipelineLayout(Engine::get()->device, filterPipelineLayout, nullptr);

		vkDestroyDescriptorPool(Engine::get()->device, descPool, nullptr);
		vkDestroyDescriptorSetLayout(Engine::get()->device, filterDescriptorSetLayout, nullptr);
	};

	std::vector<Texture*> source;
	std::vector<Texture*> filterTarget;

private:
	uint32_t filtertype = OIOO;
	uint32_t texWidth = 0;
	uint32_t texHeight = 0;

	VkPipelineLayout filterPipelineLayout = nullptr;
	VkPipeline filterPipeline = nullptr;

	VkFormat targetFormat = VK_FORMAT_R8G8B8A8_UNORM;

	VkDescriptorPool descPool = nullptr;
	VkDescriptorSetLayout filterDescriptorSetLayout = nullptr;
	VkDescriptorSet filterDescriptorSet;

	VkShaderModule filterShaderModule = nullptr; // Destroyed by default

	bool hasUniformBuffer = false;
	uint32_t bufferSize = 0;
	VkBuffer bufferRef = nullptr;

	void createDescriptorSetLayout();
	void createDescriptorSet();

	void createFilterPipelineLayout();
	void createFilterPipeline();

	void createFilterTarget();
};

#endif