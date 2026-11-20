#ifndef BOBJECT_ENGINE
#define BOBJECT_ENGINE

//#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include<GLFW/glfw3native.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtx/hash.hpp>

#include<vector>
#include<array>
#include<map>
#include<string>
#include<fstream>
#include<iostream>
#include<stdexcept>
#include<optional>
#include<set>
#include<cstdint>
#include<algorithm>
#include<memory>

#include"include/ShaderDataType.h"
#include"include/AC_Flat.h"
#include"include/Flat.h"
#include"include/AC_BF.h"
#include"include/BF.h"
#include"include/UI.h"
#include"include/UIGray.h"
#include"include/UV.h"
#include"include/W.h"
#include"include/NormalGenerator.h"
#include"include/AC_OS_BF.h"
#include"include/OS_BF.h"
#include"include/AC_TS_BF.h"
#include"include/TS_BF.h"
#include"include/UIText.h"
#include"include/UIRoundBox.h"
#include"include/Projector_Flat.h"
#include"include/Projector_BF.h"
#include"include/Projector_Background.h"
#include"include/SAC_BF.h"
#include"include/SAC_Flat.h"

#include"Bobject_Core.h"
#include"Bobject_VKMem.h"
#include"Bobject_VKCmd.h"

#include"Textures.h"
#include"Meshes.h"
//#include"Materials.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

struct drawImage {
	std::vector<Texture*> textures{};

	std::vector<VkFramebuffer> imageFrameBuffers = {};
	
	Texture* colourTex = nullptr;

	VkExtent2D imageExtent{};
	VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
	VkImageUsageFlags imageUsage{};

	VkRenderPass* boundRenderPass = nullptr;

	void cleanup(VkDevice device) {

		colourTex->cleanup();

		for (auto framebuffer : imageFrameBuffers) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}

		for (Texture* texture : textures) {
			texture->cleanup();
			delete texture;
		}
	}
};

struct GraphicsPass {
	VkRenderPass renderPass = nullptr;

	std::vector<VkPipeline*> GraphicsPipelines{};
	std::vector<shaderData> shaderDatas{};
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{};
	std::vector<VkPipelineLayout> pipelineLayouts{};
	std::map<std::string, uint32_t> layoutMap{};
	std::map<std::string, uint32_t> pipelineMap{};

	void cleanup(VkDevice device) {
		shaderDatas.clear();
		
		for (VkPipeline* pipeline : GraphicsPipelines) {
			vkDestroyPipeline(device, *pipeline, nullptr);
		}

		for (VkDescriptorSetLayout layout : descriptorSetLayouts) {
			vkDestroyDescriptorSetLayout(device, layout, nullptr);
		}

		for (VkPipelineLayout layout : pipelineLayouts) {
			vkDestroyPipelineLayout(device, layout, nullptr);
		}

		vkDestroyRenderPass(device, renderPass, nullptr);
	}
};

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::vec4 UVdistort;
	alignas(16) glm::vec3 backgroundColour;
	alignas(16) glm::vec3 lightPosition;
	alignas(16) glm::vec3 viewPosition;
};

struct ColourSchemeObject {
	alignas(16) glm::vec3 Primary;
	alignas(16) glm::vec3 Secondary;
	alignas(16) glm::vec3 Tertiary;
};

struct WindowSizeObject {
	alignas(8) glm::vec2 WindowSize;
};

class Engine {
public:
	static Engine* get(){
		if (nullptr == enginstance) enginstance = new Engine;
		return enginstance;
	}
	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;
	
	static void destruct() {
		delete enginstance;
		enginstance = nullptr;
	}

	GLFWwindow* window = nullptr;
	uint32_t pipelineindex = 1;

	VkDevice device = VK_NULL_HANDLE;

	Queue* graphicsQueueObj{};
	Queue* computeQueueObj{};
	Queue* presentQueueObj{};

	GraphicsPass defaultPass;

	std::vector<VkFence> inFlightFences = {};
	VkSwapchainKHR swapChain = nullptr;
	std::vector<VkSemaphore> imageAvailableSemaphores = {};
	std::vector<VkSemaphore> renderFinishedSemaphores = {};

	std::vector<VkCommandBuffer> commandBuffers = {};

	VkExtent2D swapChainExtent = {};

	VkSampler textureSampler = nullptr;

	std::vector<VkBuffer> uniformBuffers = {};
	std::vector<void*> uniformBuffersMapped = {};
	
	VkBuffer colourBuffer = nullptr;
	void* colourBufferMapped = nullptr;

	VkBuffer windowSizeBuffer = nullptr;
	void* windowSizeBufferMapped = nullptr;

	bool framebufferResized = false;

	void initEngine(std::string);

	void cleanup();

	void recreateSwapChain();
	std::uint32_t getRenderTarget();
	void beginRenderPass(VkCommandBuffer, GraphicsPass*, drawImage*, uint32_t, glm::vec3);
	void drawObject(VkCommandBuffer, VkBuffer, VkBuffer, VkPipelineLayout, VkDescriptorSet, std::uint32_t);
	VkResult submitAndPresentFrame(std::uint32_t);

	drawImage createDrawImage(uint32_t, int32_t, VkFormat, VkImageUsageFlags, VkRenderPass) const;
	void recreateDrawImage(drawImage*) const;

	VkShaderModule createShaderModule(const std::vector<unsigned char>&) const;

	uint32_t currentFrame = 0;

	VkFormat findDepthFormat();

	std::vector<Texture*> swapChainTextures{};
	VkFormat swapChainImageFormat = {};

	void createRenderPass(VkRenderPass&, VkFormat, VkImageLayout);
	void createGraphicsPipelines(GraphicsPass&, int blendMode = 0, bool alphaToCoverage = true) const;

	void copyImageToSwapchain(VkCommandBuffer, drawImage*, uint32_t);
	void copyImageToSwapchain(VkCommandBuffer, VkImage, VkExtent2D, uint32_t);

	void createDescriptorSetLayout(std::string, std::vector<shaderIOValue>, GraphicsPass&) const;
private:
	static Engine* enginstance;
	Engine() = default;
	~Engine() = default;

	uint32_t imageCount = 0;

	std::vector<VkDeviceMemory> uniformBuffersMemory = {};
	VkDeviceMemory colourBufferMemory = nullptr;
	VkDeviceMemory windowSizeBufferMemory = nullptr;
	Texture* depthTex = nullptr;

	void createSwapChain();
	void createGraphicsPipelines();
	void createDepthResources();
	void createTextureSampler();
	void createUniformBuffers();
	void createColourBuffer();
	void createWindowSizeBuffer();
	void createCommandBuffers();
	void createSyncObjects();

	void cleanupSwapChain();

	VkFormat findSupportedFormat(const std::vector<VkFormat>&, VkImageTiling, VkFormatFeatureFlags) const;
	
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>&);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR&) const;
};

#endif