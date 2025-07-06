#ifndef TEXTURES
#define TEXTURES

#include"Bobject_engine.h"
#include"Webcam_feeder.h"
#include"include/ImageDataType.h"
#include<opencv2/opencv.hpp>
#include<string>

struct Texture {
	bool cleaned = false;
	
	uint32_t texWidth;
	uint32_t texHeight;
	uint32_t texChannels;

	cv::Mat texMat;

	VkImage textureImage = nullptr;
	VkDeviceMemory textureImageMemory = nullptr;
	VkImageView textureImageView = nullptr;

	VkFormat textureFormat = VK_FORMAT_R8G8B8A8_SRGB;
	VkImageLayout textureLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	bool hasStencilComponent(VkFormat);
	void createImage(uint32_t, uint32_t, uint32_t, VkSampleCountFlagBits, VkFormat, VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags, VkImage&, VkDeviceMemory&);
	void transitionImageLayout(VkImage, VkFormat, VkImageLayout, VkImageLayout, uint32_t);
	void copyBufferToImage(VkBuffer, VkImage, uint32_t, uint32_t);
	void generateMipmaps(VkImage, VkFormat, int32_t, int32_t, uint32_t);
	VkImageView createImageView(VkImage, VkFormat, VkImageAspectFlags, uint32_t);
	
	void getCVMat();
	void transitionMatToImg();

	virtual void setup() {

	}

	virtual void cleanup() {

	}
};

class imageTexture : public Texture {
public:
	imageTexture() = default;

	imageTexture(cv::Mat initMat) {
		texMat = initMat;
		transitionMatToImg();
		createTextureImageView();
	}

	imageTexture(std::string filename, VkFormat format) {
		// Image texture which is loaded from file
		texMat = cv::imread(filename);
		textureFormat = format;
		transitionMatToImg();
		createTextureImageView();
	}

	imageTexture(imageData* iD) {
		// Built-in image texture
		//imgData = iD;
		createTextureImage(iD);
		createTextureImageView();
	};

	//imageData* imgData = nullptr;

	uint32_t mipLevels = 0;

	void cleanup() {
		if (textureImage != nullptr) {
			vkDestroyImage(Engine::get()->device, textureImage, nullptr);
			vkFreeMemory(Engine::get()->device, textureImageMemory, nullptr);
			vkDestroyImageView(Engine::get()->device, textureImageView, nullptr);
		}
		cleaned = true;
	}

private:
	void createTextureImage(imageData*);
	void createTextureImageView();
};

class webcamTexture : public Texture{
public:
	VkFormat textureFormat = VK_FORMAT_R8G8B8A8_SRGB;
	VkFormat previousTextureFormat = VK_FORMAT_R8G8B8A8_SRGB;

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

	Webcam webCam;
	VkBuffer textureBuffer;
	VkDeviceMemory textureBufferMemory;
	void* tBuffer;
	void updateWebcam();

	void setup() {
		createWebcamImage();
		createWebcamTextureImageView();
	}

	void cleanup() {
		if (textureImage != nullptr) {
			vkDestroyImage(Engine::get()->device, textureImage, nullptr);
			vkFreeMemory(Engine::get()->device, textureImageMemory, nullptr);
			vkDestroyImageView(Engine::get()->device, textureImageView, nullptr);
			vkDestroyBuffer(Engine::get()->device, textureBuffer, nullptr);
			vkFreeMemory(Engine::get()->device, textureBufferMemory, nullptr);
		}
		destruct();
	}

	void getCVMat() {
		texMat = webCam.webcamFrame;
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
};

#endif