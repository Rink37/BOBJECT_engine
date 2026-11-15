#ifndef BOBJECT_CORE
#define BOBJECT_CORE

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
#include<stdexcept>
#include<set>
#include<iostream>
#include<optional>
#include<map>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

const uint32_t QUEUE_SUPPORTS_PRESENT = 0x00001000; // Vulkan does not expose a queue flag for present queues, so this value has been selected to represent these flags without overlapping with existing flags: https://docs.vulkan.org/refpages/latest/refpages/source/VkQueueFlagBits.html

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;
	std::optional<uint32_t> computeFamily;

	bool isComplete() const {
		return graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities{};
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class Queue {
public:
	uint32_t accessMask = 0;
	VkQueue queue = nullptr;
	VkCommandPool commandPool = nullptr;

	Queue(uint32_t, uint32_t);
	void cleanup() const;
};

class Core {
	// Handles lowest level - the physical device and application
public:
	static Core* get() {
		if (nullptr == coreinstance) coreinstance = new Core;
		return coreinstance;
	}

	static void destruct() {
		delete coreinstance;
		coreinstance = nullptr;
	}

	Core(const Core&) = delete;
	Core& operator=(const Core&) = delete;

	static VkDevice getDevice() {
		return Core::get()->device;
	}

	static VkPhysicalDevice getPhysicalDevice() {
		return Core::get()->physicalDevice;
	}

	static VkSurfaceKHR getSurface() {
		return Core::get()->surface;
	}

	static GLFWwindow* getWindow() {
		return Core::get()->window;
	}

	static VkSampleCountFlagBits maxSamples() {
		return Core::get()->maxMSAA;
	}

	static Queue* getSupportedQueue(uint32_t accessMask) {
		for (uint32_t i = 0; i != Core::get()->queues.size(); i++) {
			if (Core::get()->queues[i].accessMask & accessMask) {
				return &Core::get()->queues[i];
			}
		}
		throw std::runtime_error("No queue supports the requested operation(s)!");
	}

	void createApp(std::string appName) {
		initWindow(appName);
		createInstance(appName);
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createUniqueQueues();
	}

	void cleanup();

	uint16_t windowWidth = 1920;
	uint16_t windowHeight = 1080;
	bool framebufferResized = false;

	SwapChainSupportDetails getSwapChainSupport() const;
	QueueFamilyIndices indices;
private:
	static Core* coreinstance;
	Core() = default;
	~Core() {
		if (coreinstance != nullptr) cleanup();
	};

	GLFWwindow* window = nullptr;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkInstance instance = nullptr;
	VkSurfaceKHR surface = nullptr;
	VkDebugUtilsMessengerEXT debugMessenger = nullptr;

	VkSampleCountFlagBits maxMSAA = VK_SAMPLE_COUNT_1_BIT;

	std::vector<Queue> queues{};

	void initWindow(std::string);
	void createInstance(std::string);
	void createSurface();
	void setupDebugMessenger();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createUniqueQueues();
};

#endif
