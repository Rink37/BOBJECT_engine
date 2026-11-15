#ifndef IMAGEPROCESSOR
#define IMAGEPROCESSOR

#include"Bobject_Engine.h" // Contains textures and meshes
#include"ShaderDataType.h"
#include"Materials.h"
#include"include/EdgeFill.h"

class filter {
public:
	filter(std::vector<Texture*> srcs, shaderData* sd) {
		for (Texture* src : srcs) {
			source.push_back(src);
		}

		shader = sd;

		filterShaderModule = Engine::get()->createShaderModule(*(sd->compData));

		for (shaderIOValue IOval: sd->IO) {
			if (IOval.type == 0) {
				createFilterTarget();
			}
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

		shader = sd;

		filterShaderModule = Engine::get()->createShaderModule(*(sd->compData));

		targetFormat = outFormat;

		for (auto IOval : sd->IO) {
			if (IOval.type == 0) {
				createFilterTarget();
			}
		}
		createDescriptorSetLayout();
		createDescriptorSet();
		createFilterPipelineLayout();
		createFilterPipeline();
	};

	filter(std::vector<Texture*> srcs, shaderData* sd, VkBuffer buffer, uint32_t bufferSize) {
		for (Texture* src : srcs) {
			source.push_back(src);
		}

		shader = sd;

		hasUniformBuffer = true;
		this->bufferRef = buffer;
		this->bufferSize = bufferSize;

		filterShaderModule = Engine::get()->createShaderModule(*(sd->compData));

		for (auto IOval : sd->IO) {
			if (IOval.type == 0) {
				createFilterTarget();
			}
		}
		createDescriptorSetLayout();
		createDescriptorSet();
		createFilterPipelineLayout();
		createFilterPipeline();
	}

	filter(std::vector<Texture*> srcs, shaderData* sd, VkFormat outFormat, VkBuffer buffer, uint32_t bufferSize) {
		for (Texture* src : srcs) {
			source.push_back(src);
		}

		shader = sd;

		hasUniformBuffer = true;
		this->bufferRef = buffer;
		this->bufferSize = bufferSize;

		filterShaderModule = Engine::get()->createShaderModule(*(sd->compData));

		targetFormat = outFormat;

		for (auto IOval : sd->IO) {
			if (IOval.type == 0) {
				createFilterTarget();
			}
		}
		createDescriptorSetLayout();
		createDescriptorSet();
		createFilterPipelineLayout();
		createFilterPipeline();
	};

	void filterImage();

	void filterImage(VkCommandBuffer);

	void cleanup() {
		for (Texture* tex : filterTarget) {
			if (tex == nullptr) {
				continue;
			}
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

	std::vector<Texture*> source{};
	std::vector<Texture*> filterTarget{};

	void updateDescriptorSet();

private:
	VkPipelineLayout filterPipelineLayout = nullptr;
	VkPipeline filterPipeline = nullptr;

	VkFormat targetFormat = VK_FORMAT_R8G8B8A8_UNORM;

	VkDescriptorPool descPool = nullptr;
	VkDescriptorSetLayout filterDescriptorSetLayout = nullptr;
	VkDescriptorSet filterDescriptorSet;

	Material* filterMaterial = nullptr;
	shaderData* shader = nullptr;

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

class postProcessFilter {
// This class is intended to perform compute shaders on swapchain images so that compute operations can be performed for each rendered frame (e.g. colour grading or resolving multiple passes)
// The current setup defines only compute shaders which depend on the value of a single pixel - if we need to read multiple (e.g. to calculate image gradients) then we would need to create a separate image to write to so that we do not read from a value that has already been written
public:
	postProcessFilter() = default;

	void setup(shaderData*, drawImage*);
	void setup(shaderData*, std::vector<drawImage*>);
	void setup(shaderData*, std::vector<Texture*>);

	void setup(shaderData*, drawImage*, VkBuffer, uint32_t);
	void setup(shaderData*, std::vector<drawImage*>, VkBuffer, uint32_t);
	void setup(shaderData*, std::vector<Texture*>, VkBuffer, uint32_t);
	
	void filterImage(VkCommandBuffer, uint32_t);

	void cleanup();

	VkImage getFilterResult(VkCommandBuffer, uint32_t);

	void recreateDescriptorSets();

	std::vector<Texture*> getRenderTargets();

private:
	std::vector<filter> filters{};
	std::vector<Texture*> constructedTextures{};
	std::vector<drawImage*> targets{};
};

class textureWriter {
	// Used to draw textures using meshes
public:
	textureWriter(Texture* targetTexture, shaderData* drawShader, bool depthTestEnable, bool descriptorEnable = true, bool fullAttribs = false, bool fillEdges = true) {
		refTex = targetTexture;
		sD = drawShader;
		depthTested = depthTestEnable;
		fullAttributes = fullAttribs;
		edgeFill = fillEdges;
		if (!edgeFill) {
			clearColour.w = 1.0f;
		}

		map = new OverlayMap();
		prepMap();
		if (descriptorEnable) {
			prepareDrawDescriptor();
		}
		createTexWritePipeline();
	};

	textureWriter(Texture* targetTexture, shaderData* drawShader, std::vector<Texture*> textures, std::vector<std::pair<std::vector<VkBuffer>, uint32_t>> buffers, bool depthTestEnable, bool descriptorEnable = true, bool fullAttribs = false, bool fillEdges = true) {
		refTex = targetTexture;
		sD = drawShader;
		depthTested = depthTestEnable;
		fullAttributes = fullAttribs;
		edgeFill = fillEdges;
		if (!edgeFill) {
			clearColour.w = 1.0f;
		}

		map = new OverlayMap();
		prepMap();
		if (descriptorEnable) {
			prepareDrawDescriptor(textures, buffers);
		}
		createTexWritePipeline();
	}

	void setClearColour(glm::vec4 colour) {
		clearColour = colour;
	};

	Texture* drawMap(std::vector<Mesh*>);

	struct OverlayMap {
		VkFramebuffer frameBuffer = nullptr;
		VkRenderPass renderPass = nullptr;
		Texture* colour = nullptr;
		Material* drawMat = nullptr;
		VkPipelineLayout pipelineLayout = nullptr;
		VkPipeline pipeline = nullptr;

		void cleanup() {
			vkDeviceWaitIdle(Engine::get()->device);
			if (colour != nullptr) {
				colour->cleanup();
				delete colour;
			}
			if (drawMat != nullptr) {
				drawMat->cleanup();
				delete drawMat;
			}
			if (pipeline != nullptr) {
				vkDestroyPipeline(Engine::get()->device, pipeline, nullptr);
				vkDestroyPipelineLayout(Engine::get()->device, pipelineLayout, nullptr);
			}
			if (renderPass != nullptr) {
				vkDestroyRenderPass(Engine::get()->device, renderPass, nullptr);
			}
			if (frameBuffer != nullptr) {
				vkDestroyFramebuffer(Engine::get()->device, frameBuffer, nullptr);
			}
			cleaned = true;
		}

		bool cleaned = true;
	};
	OverlayMap* map = nullptr;

	void cleanup() {
		map->cleanup();
		if (depth != nullptr) {
			depth->cleanup();
			delete depth;
		}
		if (attachment != nullptr) {
			attachment->cleanup();
			delete attachment;
		}
	}

private:
	Texture* refTex = nullptr;
	Texture* depth = nullptr;
	Texture* attachment = nullptr;

	shaderData* sD = nullptr;

	glm::vec4 clearColour{ 0.0f };

	VkCommandBuffer drawMap(VkCommandBuffer, std::vector<Mesh*>);

	bool depthTested = false;
	bool fullAttributes = false;
	bool edgeFill = true;

	void prepMap();
	void prepareDrawDescriptor();
	void prepareDrawDescriptor(std::vector<Texture*>, std::vector<std::pair<std::vector<VkBuffer>, uint32_t>>);
	void createTexWritePipeline();
};

#endif