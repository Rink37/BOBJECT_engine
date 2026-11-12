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

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

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

class Core {
	// Handles lowest level - the physical device, application etc.
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

	void createApp(std::string appName) {
		initWindow(appName);
		createInstance(appName);
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
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

	void initWindow(std::string);
	void createInstance(std::string);
	void createSurface();
	void setupDebugMessenger();
	void pickPhysicalDevice();
	void createLogicalDevice();
};

#endif
