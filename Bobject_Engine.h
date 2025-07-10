#ifndef BOBJECT_ENGINE
#define BOBJECT_ENGINE

#define VK_USE_PLATFORM_WIN32_KHR
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

#include"include/ShaderDataType.h"
#include"include/Flat.h"
#include"include/BF.h"
#include"include/UI.h"
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

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

VkResult CreateDebugUtilsMessengerEXT(VkInstance, const VkDebugUtilsMessengerCreateInfoEXT*, const VkAllocationCallbacks*, VkDebugUtilsMessengerEXT*);

static std::vector<char> readFile(const std::string&);

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;
	glm::vec4 tangent;
	glm::vec3 biTangent;

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

	static std::array<VkVertexInputAttributeDescription, 5>getCompleteAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};
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
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, tangent);

		attributeDescriptions[4].binding = 0;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[4].offset = offsetof(Vertex, biTangent);

		return attributeDescriptions;
	}

	bool operator== (const Vertex& other) const {
		return pos == other.pos && normal == other.normal && texCoord == other.texCoord;
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

	GLFWwindow* window;
	uint32_t pipelineindex = 1;

	int windowWidth = WIDTH;
	int windowHeight = HEIGHT;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;

	VkCommandPool commandPool;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkQueue computeQueue = nullptr;

	VkRenderPass renderPass;

	std::vector<VkFence> inFlightFences;
	VkSwapchainKHR swapChain;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;

	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkDescriptorSetLayout diffuseDescriptorSetLayout;
	VkDescriptorSetLayout diffNormDescriptorSetLayout;

	VkExtent2D swapChainExtent;

	//uint32_t mipLevels = 1;

	VkSampler textureSampler;

	std::vector<void*> uniformBuffersMapped;

	VkPipelineLayout diffusePipelineLayout;
	VkPipelineLayout diffNormPipelineLayout;

	std::vector<VkBuffer> uniformBuffers;

	std::map<std::string, int> PipelineMap = {};
	std::vector<VkPipeline*> GraphicsPipelines = {};

	std::vector<shaderData*> shaderDatas;

	bool framebufferResized = false;

	void initWindow(const char*);
	void initVulkan();

	void cleanup();

	void recreateSwapChain();

	std::uint32_t findMemoryType(std::uint32_t, VkMemoryPropertyFlags);

	void createBuffer(VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags, VkBuffer&, VkDeviceMemory&);
	void copyBuffer(VkBuffer, VkBuffer, VkDeviceSize);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer);

	VkShaderModule createShaderModule(const std::vector<unsigned char>&);

	const char* appName = "BOBJECT_engine app";

	VkSampleCountFlagBits getMaxUseableSampleCount();

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice);

	uint32_t currentFrame = 0;

	VkFormat findDepthFormat();

	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;

private:
	static Engine* enginstance;
	Engine() = default;
	~Engine() = default;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;

	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

	std::vector<VkImageView> swapChainImageViews;

	std::vector<VkDeviceMemory> uniformBuffersMemory;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	VkImage colourImage;
	VkDeviceMemory colourImageMemory;
	VkImageView colourImageView;

	static void framebufferResizeCallback(GLFWwindow*, int, int);

	bool checkValidationLayerSupport();
	void createInstance();
	void setupDebugMessenger();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createSwapChain();
	void createImageViews();
	void createRenderPass();
	void createDescriptorSetLayout();
	void createGraphicsPipelines();
	void createCommandPool();
	void createColourResources();
	void createDepthResources();
	void createFramebuffers();
	void createTextureSampler();
	void createUniformBuffers();
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

	bool isDeviceSuitable(VkPhysicalDevice);

	bool checkDeviceExtensionSupport(VkPhysicalDevice);

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&);

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>&);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR&);
};

#endif

