<<<<<<< HEAD
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
		filtertype = OIOO;
		source.push_back(src->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1));
		texWidth = source[0]->texWidth;
		texHeight = source[0]->texHeight;

		filterShaderModule = Engine::get()->createShaderModule(sd->vertData);

		createFilterTarget();
		createDescriptorSetLayout();
		createDescriptorSet();
		createFilterPipelineLayout();
		createFilterPipeline();
	}

	filter(Texture* src0, Texture* src1, shaderData* sd) {
		filtertype = TIOO;

		source.push_back(src0->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1));
		source.push_back(src1->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1));
		
		texWidth = source[0]->texWidth;
		texHeight = source[0]->texHeight;

		filterShaderModule = Engine::get()->createShaderModule(sd->vertData);

		createFilterTarget();
		createDescriptorSetLayout();
		createDescriptorSet();
		createFilterPipelineLayout();
		createFilterPipeline();
	}

	void filterImage();

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

	VkDescriptorPool descPool = nullptr;
	VkDescriptorSetLayout filterDescriptorSetLayout = nullptr;
	VkDescriptorSet filterDescriptorSet;

	VkShaderModule filterShaderModule = nullptr; // Destroyed by default

	void createDescriptorSetLayout();
	void createDescriptorSet();

	void createFilterPipelineLayout();
	void createFilterPipeline();

	void createFilterTarget();
};

=======
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
		filtertype = OIOO;
		source.push_back(src->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1));
		texWidth = source[0]->texWidth;
		texHeight = source[0]->texHeight;

		filterShaderModule = Engine::get()->createShaderModule(sd->vertData);

		createFilterTarget();
		createDescriptorSetLayout();
		createDescriptorSet();
		createFilterPipelineLayout();
		createFilterPipeline();
	}

	filter(Texture* src0, Texture* src1, shaderData* sd) {
		filtertype = TIOO;

		source.push_back(src0->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1));
		source.push_back(src1->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1));
		
		texWidth = source[0]->texWidth;
		texHeight = source[0]->texHeight;

		filterShaderModule = Engine::get()->createShaderModule(sd->vertData);

		createFilterTarget();
		createDescriptorSetLayout();
		createDescriptorSet();
		createFilterPipelineLayout();
		createFilterPipeline();
	}

	void filterImage();

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

	VkDescriptorPool descPool = nullptr;
	VkDescriptorSetLayout filterDescriptorSetLayout = nullptr;
	VkDescriptorSet filterDescriptorSet;

	VkShaderModule filterShaderModule = nullptr; // Destroyed by default

	void createDescriptorSetLayout();
	void createDescriptorSet();

	void createFilterPipelineLayout();
	void createFilterPipeline();

	void createFilterTarget();
};

>>>>>>> 65e49fd884fc33b59605b3036ff7b8ff8393947b
#endif