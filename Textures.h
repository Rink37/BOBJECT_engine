#ifndef TEXTURES
#define TEXTURES

#include"Bobject_Core.h"
#include"Bobject_VKMem.h"
#include"Bobject_VKCmd.h"

#include"Webcam_feeder.h"
#include"include/ImageDataType.h"
#include<opencv2/opencv.hpp>
#include<future>
#include<thread>

struct Texture {
	// Structure describing an arbitrary image in the application
	// I use a mix of OpenCV images and Vulkan images for various reasons, so it makes sense to have a structure which manages conversions

	//cv::Mat texMat; // The OpenCV matrix image
	VkImage textureImage = nullptr; 
	VkDeviceMemory textureImageMemory = nullptr;
	VkImageView textureImageView = nullptr;

	VkFormat textureFormat = VK_FORMAT_R8G8B8A8_SRGB;
	VkImageLayout textureLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VkImageUsageFlags textureUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	VkImageTiling textureTiling = VK_IMAGE_TILING_OPTIMAL;
	VkMemoryPropertyFlags textureMemFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	VkSampleCountFlagBits textureSamples = VK_SAMPLE_COUNT_1_BIT;
	VkImageAspectFlags textureAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

	uint8_t mipLevels = 0;

	bool isNormal = false;
	bool normalType = false; // false represents an OS normal, true represents a TS normal

	uint8_t texChannels = 0;
	uint16_t texWidth = 0;
	uint16_t texHeight = 0;

	void createImage(VkSampleCountFlagBits, VkMemoryPropertyFlags);
	
	void transitionImageLayout(VkCommandBuffer, VkImageLayout, VkImageLayout) const;
	void transitionImageLayout(VkImageLayout, VkImageLayout) const;
	
	void copyBufferToImage(VkBuffer, VkImage, uint32_t, uint32_t);
	void generateMipmaps() const;
	void createImageView(VkImageAspectFlags);
	
	cv::Mat getCVMat();
	//void destroyCVMat();
	void transitionMatToImg(cv::Mat);

	Texture* copyImage(VkFormat, VkImageLayout, VkImageUsageFlags, VkImageTiling, VkMemoryPropertyFlags, uint32_t, uint32_t, uint32_t);
	Texture* copyImage(VkFormat, VkImageLayout, VkImageUsageFlags, VkImageTiling, VkMemoryPropertyFlags, uint32_t);
	Texture* copyImage();
	Texture* copyImage(uint32_t, uint32_t);
	Texture* copyTexture();
	Texture* copyTexture(VkFormat, VkImageLayout, VkImageUsageFlags, VkImageTiling, uint32_t);
	Texture* copyTexture(VkFormat, VkImageLayout, VkImageUsageFlags, VkImageTiling, uint32_t, uint32_t, uint32_t);
	Texture* copyTexture(uint32_t, uint32_t);

	virtual void setup() {
		if (texWidth == 0) {
			texWidth = 512;
			texHeight = 512;
			texChannels = 4;
		}
		if (mipLevels == 0) {
			mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
		}
		createImage(textureSamples, textureMemFlags);
		if (mipLevels != 1) {
			transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			generateMipmaps();
		}
		else {
			transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, textureLayout);
		}	
		createImageView(textureAspectFlags);
		cleaned = false;
	}

	bool cleaned = true;

	virtual void cleanup() {
		if (cleaned) {
			return;
		}
		vkDeviceWaitIdle(Core::getDevice());
		if (textureImage != nullptr) {
			vkDestroyImage(Core::getDevice(), textureImage, nullptr);
		}
		if (textureImageMemory != nullptr) {
			vkFreeMemory(Core::getDevice(), textureImageMemory, nullptr);
		}
		if (textureImageView != nullptr) {
			vkDestroyImageView(Core::getDevice(), textureImageView, nullptr);
		}
		textureImage = nullptr;
		textureImageMemory = nullptr;
		textureImageView = nullptr;
		//destroyCVMat();
		cleaned = true;
	}

	Texture() = default;
	
	~Texture() {
		cleanup();
	}
};

class imageTexture : public Texture {
	// A specific class of texture which constructs an engine image using pixel data
	// OpenCV matrixes are not constructed by default if construction arguments are not OpenCV images, but they can be constructed later - this is just to save memory
	// If the image is constructed using an OpenCV matrix the matrix is stored since we assume the matrix will be used again
	// If it won't be, we can destroy the matrix using destroyCVMat();
public:
	imageTexture() = default;

	~imageTexture() {
		cleanup();
	}

	imageTexture(cv::Mat initMat) {
		// Image texture which is created using a given openCV image
		//texMat = initMat.clone();
		transitionMatToImg(initMat);
		createTextureImageView();
		//destroyCVMat();
	}

	imageTexture(cv::Mat initMat, VkFormat format, VkImageLayout layout, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t ml) {
		// Image texture created from an openCV image with non-standard properties
		//texMat = initMat;
		textureFormat = format;
		textureLayout = layout;
		textureUsage = usage;
		textureTiling = tiling;
		mipLevels = ml;
		transitionMatToImg(initMat);
		createTextureImageView();
		//destroyCVMat();
	}

	imageTexture(cv::Mat initMat, VkFormat format) {
		// Image texture which is loaded from file
		//texMat = initMat.clone();
		textureFormat = format;
		transitionMatToImg(initMat);
		createTextureImageView();
		//destroyCVMat();
	}

	imageTexture(imageData* imageBytes) {
		// Built-in image texture
		createTextureImage(imageBytes);
		createTextureImageView();
	};

	imageTexture(imageData* imageBytes, VkFormat imageFormat) {
		// Built-in image texture
		textureFormat = imageFormat;
		createTextureImage(imageBytes);
		createTextureImageView();
	};

	void setup() {

	}

private:
	void createTextureImage(imageData*);
	void createTextureImageView();
};

class webcamTexture : public Texture{
public:

	static webcamTexture* get() {
		if (nullptr == winstance) winstance = new webcamTexture;
		return winstance;
	}
	webcamTexture(const webcamTexture&) = delete;
	webcamTexture& operator=(const webcamTexture&) = delete;
	
	static void destruct() {
		winstance->cleanup();
	}

	Webcam *webCam = nullptr;
	VkBuffer textureBuffer = nullptr;
	VkDeviceMemory textureBufferMemory = nullptr;
	void* tBuffer = nullptr;
	
	void updateWebcam();
	void asyncUpdate();
	void fetchFrame();

	void setup() {
		webCam = new Webcam(0);
		if (!webCam->isValid) {
			cv::Mat texMat = cv::Mat(96, 128, CV_8UC3);
			texMat = cv::Scalar(255, 0, 255);
			transitionMatToImg(texMat);
			createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
			return;
		}
		else {
			createWebcamImage();
			createWebcamTextureImageView();
		}
		frameUpdate.get();
		frameUpdate = std::async(std::launch::async, &webcamTexture::fetchFrame, this);
	}

	void interruptFrameUpdate() const {
		if (frameUpdate.valid()) {
			frameUpdate.wait();
		}
	}

	void startFrameUpdate() {
		frameUpdate = std::async(std::launch::async, &webcamTexture::fetchFrame, this);
	}

	void recreateWebcamImage();

	void cleanupImage() {
		if (textureImage != nullptr) {
			vkDestroyImage(Core::getDevice(), textureImage, nullptr);
			vkFreeMemory(Core::getDevice(), textureImageMemory, nullptr);
			vkDestroyImageView(Core::getDevice(), textureImageView, nullptr);
			vkDestroyBuffer(Core::getDevice(), textureBuffer, nullptr);
			vkFreeMemory(Core::getDevice(), textureBufferMemory, nullptr);
		}
		textureImage = nullptr;
	}

	void cleanup() {
		if (cleaned || webCam == nullptr) {
			return;
		}
		vkDeviceWaitIdle(Core::getDevice());
		cleanupImage();
		webCam->cleanup();
		delete webCam;
		webCam = nullptr;
		cleaned = true;
	}

	cv::Mat getCVMat() {
		return webCam->webcamFrame;
	}

	void changeFormat(VkFormat newFormat) {
		textureFormat = newFormat;
	}

	std::future<void> frameUpdate = std::async(std::launch::async, []() {return; });

private:
	static webcamTexture* winstance;
	webcamTexture() = default;
	~webcamTexture() = default;

	void updateWebcamImage();
	void createWebcamImage();
	void createWebcamTextureImageView();

	VkDeviceSize imageSize = 0;
};

#endif