#ifndef TEXTURES
#define TEXTURES

#include"Bobject_engine.h"
#include"Webcam_feeder.h"
#include"include/ImageDataType.h"
#include<opencv2/opencv.hpp>

struct Texture {
	// Structure describing an arbitrary image in the application
	// I use a mix of OpenCV images and Vulkan images for various reasons, so it makes sense to have a structure which manages conversions

	uint32_t texWidth = 0;
	uint32_t texHeight = 0; 
	uint32_t texChannels = 0; 

	cv::Mat texMat; // The OpenCV matrix image
	VkImage textureImage = nullptr; 
	VkDeviceMemory textureImageMemory = nullptr;
	VkImageView textureImageView = nullptr;

	VkFormat textureFormat = VK_FORMAT_R8G8B8A8_SRGB;
	VkImageLayout textureLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VkImageUsageFlags textureUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	VkImageTiling textureTiling = VK_IMAGE_TILING_OPTIMAL;
	uint32_t mipLevels = 0;

	bool hasStencilComponent(VkFormat);
	void createImage(VkSampleCountFlagBits, VkMemoryPropertyFlags);
	void transitionImageLayout(VkImage, VkFormat, VkImageLayout, VkImageLayout, uint32_t); // We need to be able to explicitly pass an image for image copying functions
	void copyBufferToImage(VkBuffer, VkImage, uint32_t, uint32_t);
	void generateMipmaps();
	VkImageView createImageView(VkImageAspectFlags);
	
	void getCVMat();
	void destroyCVMat();
	void transitionMatToImg();

	Texture* copyImage(VkFormat, VkImageLayout, VkImageUsageFlags, VkImageTiling, VkMemoryPropertyFlags, uint32_t);
	Texture* copyTexture(VkFormat, VkImageLayout, VkImageUsageFlags, VkImageTiling, uint32_t);

	virtual void setup() {
		if (texWidth == 0) {
			texWidth = 512;
			texHeight = 512;
			texChannels = 4;
		}
		if (mipLevels == 0) {
			mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
		}
		createImage(VK_SAMPLE_COUNT_1_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		transitionImageLayout(textureImage, textureFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
		generateMipmaps();
		textureImageView = createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
		cleaned = false;
	}

	bool cleaned = true;

	virtual void cleanup() {
		if (cleaned) {
			return;
		}
		if (textureImage != nullptr) {
			vkDestroyImage(Engine::get()->device, textureImage, nullptr);
			vkFreeMemory(Engine::get()->device, textureImageMemory, nullptr);
			vkDestroyImageView(Engine::get()->device, textureImageView, nullptr);
		}
		textureImage = nullptr;
		textureImageMemory = nullptr;
		textureImageView = nullptr;
		destroyCVMat();
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
		texMat = initMat.clone();
		transitionMatToImg();
		createTextureImageView();
		destroyCVMat();
		//cleaned = false;
	}

	imageTexture(cv::Mat initMat, VkFormat format, VkImageLayout layout, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t ml) {
		// Image texture created from an openCV image with non-standard properties
		texMat = initMat;
		textureFormat = format;
		textureLayout = layout;
		textureUsage = usage;
		textureTiling = tiling;
		mipLevels = ml;
		transitionMatToImg();
		createTextureImageView();
		destroyCVMat();
		//cleaned = false;
	}

	imageTexture(std::string filename, VkFormat format) {
		// Image texture which is loaded from file
		texMat = cv::imread(filename);
		textureFormat = format;
		transitionMatToImg();
		createTextureImageView();
		destroyCVMat();
		//cleaned = false;
	}

	imageTexture(cv::Mat initMat, VkFormat format) {
		// Image texture which is loaded from file
		texMat = initMat.clone();
		textureFormat = format;
		transitionMatToImg();
		createTextureImageView();
		destroyCVMat();
		//cleaned = false;
	}

	imageTexture(imageData* imageBytes) {
		// Built-in image texture
		createTextureImage(imageBytes);
		createTextureImageView();
		//cleaned = false;
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
	Engine& operator=(const webcamTexture&) = delete;
	
	static void destruct() {
		delete winstance;
		winstance = nullptr;
	}

	Webcam *webCam = nullptr;
	VkBuffer textureBuffer = nullptr;
	VkDeviceMemory textureBufferMemory = nullptr;
	void* tBuffer = nullptr;
	void updateWebcam();

	void setup() {
		webCam = new Webcam(0);
		if (!webCam->isValid) {
			std::cout << "New image created" << std::endl;
			delete webCam;
			webCam = nullptr;
			cv::Mat camNotFoundImg(128, 128, CV_8UC3);
			camNotFoundImg = cv::Scalar(255, 0, 255);
			Texture* ptr = new imageTexture(camNotFoundImg);
			texChannels = ptr->texChannels;
			texHeight = ptr->texHeight;
			texWidth = ptr->texWidth;
			textureImage = ptr->textureImage;
			textureImageMemory = ptr->textureImageMemory;
			textureImageView = ptr->textureImageView;
			delete ptr;
			camNotFoundImg.release();
			return;
		}
		createWebcamImage();
		createWebcamTextureImageView();
	}

	void cleanup() {
		if (cleaned || webCam == nullptr) {
			return;
		}
		if (textureImage != nullptr) {
			vkDestroyImage(Engine::get()->device, textureImage, nullptr);
			vkFreeMemory(Engine::get()->device, textureImageMemory, nullptr);
			vkDestroyImageView(Engine::get()->device, textureImageView, nullptr);
			vkDestroyBuffer(Engine::get()->device, textureBuffer, nullptr);
			vkFreeMemory(Engine::get()->device, textureBufferMemory, nullptr);
		}
		textureImage = nullptr;
		webCam->cleanup();
		delete webCam;
		webCam = nullptr;
		cleaned = true;
	}

	void getCVMat() {
		texMat = webCam->webcamFrame;
	}

	void changeFormat(VkFormat newFormat) {
		textureFormat = newFormat;
	}

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