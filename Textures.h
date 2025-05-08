#ifndef TEXTURES
#define TEXTURES

#include"Bobject_engine.h"
#include"Webcam_feeder.h"
#include"include/ImageDataType.h"
#include<string>

struct Texture {
	int texWidth;
	int texHeight;
	int texChannels;

	VkImage textureImage = nullptr;
	VkDeviceMemory textureImageMemory = nullptr;
	VkImageView textureImageView = nullptr;

	bool hasStencilComponent(VkFormat);
	void createImage(uint32_t, uint32_t, uint32_t, VkSampleCountFlagBits, VkFormat, VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags, VkImage&, VkDeviceMemory&);
	void transitionImageLayout(VkImage, VkFormat, VkImageLayout, VkImageLayout, uint32_t);
	void copyBufferToImage(VkBuffer, VkImage, uint32_t, uint32_t);
	void generateMipmaps(VkImage, VkFormat, int32_t, int32_t, uint32_t);
	VkImageView createImageView(VkImage, VkFormat, VkImageAspectFlags, uint32_t);

	virtual void setup() {

	}

	virtual void cleanup() {

	}
};

class imageTexture : public Texture {
public:

	imageTexture(imageData* iD) {
		imgData = iD;
		//TEXTURE_PATH = path;
		createTextureImage();
		createTextureImageView();
	};

	imageData* imgData;

	std::string TEXTURE_PATH;

	uint32_t mipLevels;

	void cleanup() {
		if (textureImage != nullptr) {
			vkDestroyImage(Engine::get()->device, textureImage, nullptr);
			vkFreeMemory(Engine::get()->device, textureImageMemory, nullptr);
			vkDestroyImageView(Engine::get()->device, textureImageView, nullptr);
		}
	}

private:
	void createTextureImage();
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
private:
	static webcamTexture* winstance;
	webcamTexture() = default;
	~webcamTexture() = default;

	void updateWebcamImage();
	void createWebcamImage();
	void createWebcamTextureImageView();
};

#endif