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
#include"include/Flat.h"
#include"include/BF.h"
#include"include/UI.h"
#include"include/UIGray.h"
#include"include/UV.h"
#include"include/W.h"
#include"include/NormalGenerator.h"
#include"include/OS_BF.h"
#include"include/TS_BF.h"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const int MAX_FRAMES_IN_FLIGHT = 2;

const uint32_t WIDTH = 1920;
const uint32_t HEIGHT = 1080;

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

VkResult CreateDebugUtilsMessengerEXT(VkInstance, const VkDebugUtilsMessengerCreateInfoEXT*, const VkAllocationCallbacks*, VkDebugUtilsMessengerEXT*);

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;
	glm::vec4 tangent;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3>getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, normal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}

	static std::array<VkVertexInputAttributeDescription, 4>getCompleteAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, normal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, tangent);

		return attributeDescriptions;
	}

	bool operator== (const Vertex& other) const {
		return pos == other.pos && normal == other.normal && texCoord == other.texCoord;
	}
};

struct drawImage {
	std::vector<VkImage> images = {};
	std::vector<VkDeviceMemory> imageMemory = {};
	std::vector<VkImageView> imageViews = {};
	std::vector<VkFramebuffer> imageFrameBuffers = {};
	
	VkImage colourImage;
	VkImageView colourImageView;
	VkDeviceMemory colourImageMemory;
	
	VkExtent2D imageExtent;
	VkFormat imageFormat;
	VkImageUsageFlags imageUsage;

	void cleanup(VkDevice device) {

		vkDestroyImageView(device, colourImageView, nullptr);
		vkDestroyImage(device, colourImage, nullptr);
		vkFreeMemory(device, colourImageMemory, nullptr);

		for (auto framebuffer : imageFrameBuffers) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}

		for (auto image : images) {
			vkDestroyImage(device, image, nullptr);
		}

		for (auto mem : imageMemory) {
			vkFreeMemory(device, mem, nullptr);
		}

		for (auto imageView : imageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}
	}
};

struct GraphicsPass {
	VkRenderPass renderPass;
	VkPipelineLayout diffusePipelineLayout = nullptr;
	VkPipelineLayout diffNormPipelineLayout = nullptr;
	std::vector<VkPipeline*> GraphicsPipelines = {};

	void cleanup(VkDevice device) {
		for (VkPipeline* pipeline : GraphicsPipelines) {
			vkDestroyPipeline(device, *pipeline, nullptr);
		}

		vkDestroyPipelineLayout(device, diffusePipelineLayout, nullptr);
		vkDestroyPipelineLayout(device, diffNormPipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;
	std::optional<uint32_t> computeFamily;

	const bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
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

	int windowWidth = WIDTH;
	int windowHeight = HEIGHT;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;

	VkCommandPool commandPool = nullptr;
	VkCommandPool computeCommandPool = nullptr;
	VkQueue graphicsQueue = nullptr;
	VkQueue presentQueue = nullptr;
	VkQueue computeQueue = nullptr;

	GraphicsPass defaultPass;

	//VkRenderPass renderPass = nullptr;

	std::vector<VkFence> inFlightFences = {};
	VkSwapchainKHR swapChain = nullptr;
	std::vector<VkSemaphore> imageAvailableSemaphores = {};
	std::vector<VkSemaphore> renderFinishedSemaphores = {};

	std::vector<VkCommandBuffer> commandBuffers = {};

	std::vector<VkFramebuffer> swapChainFramebuffers = {};

	VkDescriptorSetLayout diffuseDescriptorSetLayout = nullptr;
	VkDescriptorSetLayout diffNormDescriptorSetLayout = nullptr;

	VkExtent2D swapChainExtent = {};

	VkSampler textureSampler = nullptr;

	std::vector<void*> uniformBuffersMapped = {};
	void* colourBufferMapped = nullptr;

	//VkPipelineLayout diffusePipelineLayout = nullptr;
	//VkPipelineLayout diffNormPipelineLayout = nullptr;

	std::vector<VkBuffer> uniformBuffers = {};
	VkBuffer colourBuffer = nullptr;

	std::map<std::string, int> PipelineMap = {};
	//std::vector<VkPipeline*> GraphicsPipelines = {};

	bool framebufferResized = false;

	void initWindow(const char*);
	void initVulkan();

	void cleanup();

	void recreateSwapChain();
	std::uint32_t getRenderTarget();
	void beginRenderPass(VkCommandBuffer, GraphicsPass*, drawImage*, uint32_t, glm::vec3);
	void beginRenderPass(VkCommandBuffer, uint32_t, glm::vec3);
	void drawObject(VkCommandBuffer, VkBuffer, VkBuffer, VkPipelineLayout, VkDescriptorSet, std::uint32_t);
	VkResult submitAndPresentFrame(std::uint32_t);

	std::uint32_t findMemoryType(std::uint32_t, VkMemoryPropertyFlags);

	void createBuffer(VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags, VkBuffer&, VkDeviceMemory&);
	void copyBuffer(VkBuffer, VkBuffer, VkDeviceSize);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer);

	VkCommandBuffer beginSingleTimeComputeCommand();
	void endSingleTimeComputeCommand(VkCommandBuffer);

	drawImage createDrawImage(uint32_t, int32_t, VkFormat, VkImageUsageFlags, VkRenderPass);

	VkShaderModule createShaderModule(const std::vector<unsigned char>&);

	const char* appName = "BOBJECT_engine app";

	VkSampleCountFlagBits getMaxUseableSampleCount();

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice);

	uint32_t currentFrame = 0;

	VkFormat findDepthFormat();

	std::vector<VkImage> swapChainImages = {};
	std::vector<VkImageView> swapChainImageViews = {};
	VkFormat swapChainImageFormat = {};

	void createRenderPass(VkRenderPass&, VkFormat, VkImageLayout);
	void createGraphicsPipelines(GraphicsPass&);

	void copyImageToSwapchain(VkCommandBuffer, drawImage*, uint32_t);

private:
	static Engine* enginstance;
	Engine() = default;
	~Engine() = default;

	uint32_t imageCount = 0;

	VkInstance instance = nullptr;
	VkDebugUtilsMessengerEXT debugMessenger = nullptr;
	VkSurfaceKHR surface = nullptr;

	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

	std::vector<VkDeviceMemory> uniformBuffersMemory = {};
	VkDeviceMemory colourBufferMemory = nullptr;

	VkImage depthImage = nullptr;
	VkDeviceMemory depthImageMemory = nullptr;
	VkImageView depthImageView = nullptr;

	VkImage colourImage = nullptr;
	VkDeviceMemory colourImageMemory = nullptr;
	VkImageView colourImageView = nullptr;

	static void framebufferResizeCallback(GLFWwindow*, int, int);

	void transitionImageLayout(VkImage& image, VkImageLayout oldLayout, VkImageLayout newLayout);

	bool checkValidationLayerSupport();
	void createInstance();
	void setupDebugMessenger();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createSwapChain();
	void createImageViews();
	void createDescriptorSetLayout();
	void createGraphicsPipelines();
	void createCommandPool();
	void createComputeCommandPool();
	void createColourResources();
	void createDepthResources();
	void createFramebuffers();
	void createTextureSampler();
	void createUniformBuffers();
	void createColourBuffer();
	void createCommandBuffers();
	void createSyncObjects();

	void cleanupSwapChain();

	std::vector<const char*> getRequiredExtensions();

	void DestroyDebugUtilsMessengerEXT(VkInstance, VkDebugUtilsMessengerEXT, const VkAllocationCallbacks*);

	VkFormat findSupportedFormat(const std::vector<VkFormat>&, VkImageTiling, VkFormatFeatureFlags);

	void createImage(uint32_t, uint32_t, uint32_t, VkSampleCountFlagBits, VkFormat, VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags, VkImage&, VkDeviceMemory&);
	VkImageView createImageView(VkImage, VkFormat, VkImageAspectFlags, uint32_t);

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT&);

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
	) {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
		return VK_FALSE;
	}

	uint32_t getDeviceSuitability(VkPhysicalDevice);

	bool checkDeviceExtensionSupport(VkPhysicalDevice);

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&);

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>&);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR&);
};

#endif