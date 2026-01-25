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
		for (Texture* src : srcs) {
			source.push_back(src);
		}

		getFilterLayout(sd);

		texWidth = source[0]->texWidth;
		texHeight = source[0]->texHeight;

		filterShaderModule = Engine::get()->createShaderModule(sd->compData);

		for (int i = 0; i != outputImageCount; i++) {
			createFilterTarget();
		}
		createDescriptorSetLayout();
		createDescriptorSet();
		createFilterPipelineLayout();
		createFilterPipeline();
	};

	filter(std::vector<Texture*> srcs, shaderData* sd, VkFormat outFormat) {
		for (Texture* src : srcs) {
			source.push_back(src);
		}

		getFilterLayout(sd);

		texWidth = source[0]->texWidth;
		texHeight = source[0]->texHeight;

		filterShaderModule = Engine::get()->createShaderModule(sd->compData);

		targetFormat = outFormat;

		for (int i = 0; i != outputImageCount; i++) {
			createFilterTarget();
		}
		createDescriptorSetLayout();
		createDescriptorSet();
		createFilterPipelineLayout();
		createFilterPipeline();
	};

	filter(std::vector<Texture*> srcs, shaderData* sd, VkFormat outFormat, VkBuffer buffer, uint32_t bufferSize) {
		for (Texture* src : srcs) {
			source.push_back(src);
		}

		getFilterLayout(sd);

		hasUniformBuffer = true;
		this->bufferRef = buffer;
		this->bufferSize = bufferSize;

		texWidth = source[0]->texWidth;
		texHeight = source[0]->texHeight;

		filterShaderModule = Engine::get()->createShaderModule(sd->compData);

		targetFormat = outFormat;

		for (int i = 0; i != outputImageCount; i++) {
			createFilterTarget();
		}
		createDescriptorSetLayout();
		createDescriptorSet();
		createFilterPipelineLayout();
		createFilterPipeline();
	};

	void getFilterLayout(shaderData*);

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
	std::vector<VkImageView> imageViews;
	std::vector<VkFormat> originalFormats;

	void updateDescriptorSet();

private:
	uint32_t outputImageCount = 0;

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

	std::vector<shaderIOValue> IObindings;

	void createDescriptorSetLayout();
	void createDescriptorSet();

	void createFilterPipelineLayout();
	void createFilterPipeline();

	void createFilterTarget();
};

class postProcessFilter {
// This class is intended to perform compute shaders on swapchain images so that compute operations can be performed for each rendered frame (e.g. colour grading or resolving multiple passes)
// The current setup defines only compute shaders which depend on the value of a single pixel - if we need to read multiple (e.g. to calculate image gradients) then we would need to create a separate image to write to so that we do not read from a value that has already been written
public:
	postProcessFilter() = default;

	void setup(shaderData*, drawImage*);
	void filterImage(VkCommandBuffer, uint32_t);

	void cleanup();

	VkImage getFilterResult(VkCommandBuffer, uint32_t);

	void recreateDescriptorSets();

private:
	std::vector<filter> filters{};
	std::vector<Texture*> constructedTextures{};

	drawImage* target = nullptr;
};

#endif