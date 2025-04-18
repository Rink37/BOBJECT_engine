#define STB_IMAGE_IMPLEMENTATION
#include"stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include"tiny_obj_loader.h"

#include<iostream>
#include<stdexcept>
#include<cstdlib>
#include<cstring>
#include<vector>
#include<optional>
#include<set>
#include<cstdint>
#include<limits>
#include<algorithm>
#include<fstream>
#include<array>
#include<chrono>
#include<thread>
#include<unordered_map>

#include"Bobject_Engine.h"
#include"Pipelines.h"
#include"InputManager.h"
#include"WindowsFileManager.h"
#include"CameraController.h"
#include"UIelements.h"
#include<opencv2/opencv.hpp>
#include"Webcam_feeder.h"

using namespace cv;
using namespace std;

const string LOAD_BUTTON_PATH = "C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\LoadButton.png";
const string RENDERED_BUTTON_PATH = "C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\RenderedButton.png";
const string UNRENDERED_BUTTON_PATH = "C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\UnrenderedButton.png";

uint32_t currentFrame = 0;

vector<int> keybinds = { GLFW_KEY_L, GLFW_KEY_0, GLFW_KEY_1, GLFW_KEY_U, GLFW_KEY_I};

std::vector<KeyInput*> KeyInput::_instances;
KeyInput defaultKeyBinds(keybinds);

struct StaticObject {

	bool isVisible = false;

	string texPath = "C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\webcam_frame.jpeg";

	vector<Vertex> vertices;
	vector<uint32_t> indices;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	void* vBuffer;

	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	VkDescriptorPool descriptorPool;
	vector<VkDescriptorSet> descriptorSets;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView; 

	uint32_t mipLevels;

	int texWidth;
	int texHeight;

	bool loadModel() {
		tinyobj::attrib_t attrib;
		vector<tinyobj::shape_t> shapes;
		vector<tinyobj::material_t> materials;
		string warn, err;

		string testMODEL_PATH;

		try {
			testMODEL_PATH = winFile::OpenFileDialog();
		}
		catch (...) {
			return false;
		}
		

		if (tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, testMODEL_PATH.c_str())) {
			unordered_map<Vertex, uint32_t> uniqueVertices{};

			for (const auto& shape : shapes) {
				for (const auto& index : shape.mesh.indices) {
					Vertex vertex{};

					vertex.pos = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2]
					};

					vertex.normal = {
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2]
					};

					vertex.texCoord = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
					};


					if (uniqueVertices.count(vertex) == 0) {
						uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
						vertices.push_back(vertex);
					}

					indices.push_back(uniqueVertices[vertex]);
				}
			}
			return true;
		}
		return false;
	}
};

struct UIObject {

	string texPath = "C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\webcam_frame.jpeg";

	vector<Vertex> vertices;
	vector<uint32_t> indices = { 0, 3, 2, 2, 1, 0 };

	VkImage textureImage;
	VkImageView textureImageView;
	VkDeviceMemory textureImageMemory;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	void* vBuffer;

	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	VkDescriptorPool descriptorPool;
	vector<VkDescriptorSet> descriptorSets;

	uint32_t mipLevels; 

	int texHeight;
	int texWidth;

	float sizex;
	float sizey;

	float anchorX, anchorY; //Independent of image size

	int xoffset = 1;
	int yoffset = -1;

	float xpos, ypos;

	float offset = 0.02f; //Represents x offset, we scale y by x;

	float imageSize = 0.15f; //represents the scaling of the horizontal image axis

	bool isAbsolute = false;

	float absHeight; 
	float absWidth; 

	void setAnchor(float x, float y) {
		anchorX = x;
		anchorY = y;
	}

	void setOffsets(int x, int y) {
		xoffset = x;
		yoffset = y;
	}

	void updateAbsScale() {
		absHeight = static_cast<float>(texHeight) * imageSize;
		absWidth = static_cast<float>(texWidth) * imageSize;
	}

	void createVertices(int windowWidth, int windowHeight) { 
		vertices.clear();

		Vertex vertex;

		if (!isAbsolute) {
			sizex = imageSize;
			sizey = (imageSize * windowWidth * static_cast<float>(texHeight) / static_cast<float>(texWidth)) / (static_cast<float>(windowHeight));
		}
		else {
			sizex = absWidth / static_cast<float>(windowWidth);
			sizey = absHeight / static_cast<float>(windowHeight);
		}

		xpos = (sizex + offset) * static_cast<float>(xoffset);
		ypos = (sizey + offset * static_cast<float>(windowWidth) / static_cast<float>(windowHeight)) * static_cast<float>(yoffset);

		vertex.pos = { -sizex + anchorX + xpos, -sizey - anchorY - ypos, 0.0f };
		vertex.normal = { 0.0f, 0.0f, 0.0f };
		vertex.texCoord = { 0.0f, 0.0f };
		vertices.push_back(vertex);  
		vertex.pos = { sizex + anchorX + xpos, -sizey - anchorY - ypos, 0.0f };
		vertex.texCoord = { 1.0f, 0.0f };
		vertices.push_back(vertex);
		vertex.pos = { sizex + anchorX + xpos, sizey - anchorY - ypos, 0.0f };
		vertex.texCoord = {1.0f, 1.0f};
		vertices.push_back(vertex);
		vertex.pos = { -sizex + anchorX + xpos, sizey - anchorY - ypos, 0.0f };
		vertex.texCoord = {0.0f, 1.0f};
		vertices.push_back(vertex);
	}

	void setButtonExtent(OldButton& button, int windowWidth, int windowHeight) {
		float Bxpos = (anchorX + xpos + 1)/2*static_cast<float>(windowWidth);
		float Bypos = -(anchorY + ypos - 1)/2 * static_cast<float>(windowHeight);
		float xsize = sizex * static_cast<float>(windowWidth);
		float ysize = sizey * static_cast<float>(windowHeight);
		button.update(Bxpos, Bypos, xsize, ysize, windowWidth, windowHeight);
	}
};

class Application {
public:
	void run() {
		engine.initWindow();
		engine.initVulkan();
		KeyInput::setupKeyInputs(engine.window);
		createCanvas();
		mainLoop();
		cleanup();
	}
private:
	Engine engine;

	VkBuffer textureBuffer;
	VkDeviceMemory textureBufferMemory;
	void* tBuffer;

	Camera camera;
	Webcam webCam;

	uint32_t webHeight, webWidth, webChannels;

	vector<UIImage> UIimages;

	Button *Rendered;
	Button *Unrendered;
	Button *LB;

	vector<UIItem*> canvas;

	vArrangement* ObjectButtons;

	//int windowWidth = WIDTH;
	//int windowHeight = HEIGHT;

	double mouseX, mouseY;

	float lastModelRotation = 0.0f;

	vector<StaticObject> staticObjects;
	map<string, int> ObjectMap;

	vector<UIObject> UIobjects;

	bool webcamObjectView = true;

	std::function<void()> updateWebcamFrame;

	void updateWebcam() {

		webCam.getFrame();
		cv::Mat camFrame = webCam.webcamFrame;
		uchar* camData = new uchar[camFrame.total() * 4];
		Mat continuousRGBA(camFrame.size(), CV_8UC4, camData);
		cv::cvtColor(camFrame, continuousRGBA, cv::COLOR_BGR2RGBA, 4);

		int texWidth, texHeight, texChannels;

		texWidth = continuousRGBA.size().width;
		texHeight = continuousRGBA.size().height;
		texChannels = continuousRGBA.channels();

		VkDeviceSize imageSize = continuousRGBA.total() * continuousRGBA.elemSize();

		memcpy(tBuffer, continuousRGBA.ptr(), (size_t)imageSize);

		delete[] camData;
	}

	void updateWebcamImage(auto& object) {

		transitionImageLayout(object.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, object.mipLevels);
		copyBufferToImage(textureBuffer, object.textureImage, static_cast<uint32_t>(webWidth), static_cast<uint32_t>(webHeight));
		generateMipmaps(object.textureImage, VK_FORMAT_R8G8B8A8_SRGB, webWidth, webHeight, object.mipLevels);

	}

	void createCanvas() {
		hArrangement *Renderbuttons = new hArrangement(0, 0, 0.4, 0.2, 0.1);

		Button *loadObjectButton = new Button(0, 0, 0.2f, 0.1f, LOAD_BUTTON_PATH, engine.windowWidth, engine.windowHeight);

		Button *litRenderingButton = new Button(0, 0, 0.2f, 0.2f, RENDERED_BUTTON_PATH, engine.windowWidth, engine.windowHeight);

		Button *unlitRenderingButton = new Button(0, 0, 0.2f, 0.2f, UNRENDERED_BUTTON_PATH, engine.windowWidth, engine.windowHeight);

		Renderbuttons->addItem(unlitRenderingButton);
		Renderbuttons->addItem(litRenderingButton);

		vArrangement* buttons = new vArrangement(-0.8, 0.75, 0.15, 0.15, 0);

		buttons->addItem(loadObjectButton);
		buttons->addItem(Renderbuttons);
		buttons->arrangeItems(engine.windowWidth, engine.windowHeight);

		Unrendered = unlitRenderingButton;
		Rendered = litRenderingButton;
		LB = loadObjectButton;

		ObjectButtons = new vArrangement(-0.8, -0.5, 0.15, 0.2, 0.1);

		canvas.push_back(ObjectButtons);

		for (Button *item : {loadObjectButton, litRenderingButton, unlitRenderingButton}) {
			item->updateDisplay(engine.windowWidth, engine.windowHeight);

			createTextureImage(item->image);
			createTextureImageView(item->image);
			createUIVertexBuffer(item->image);
			createIndexBuffer(item->image);
			createDescriptorPool(item->image);
			createDescriptorSets(item->image);
		}

		canvas.push_back(buttons);

		UIObject webcamView;
		webcamView.setAnchor(1.0f, 0.5f);
		webcamView.setOffsets(-1, -1);
		webcamView.imageSize = 0.2f;

		createWebcamImage();

		webcamView.texHeight = webHeight;
		webcamView.texWidth = webWidth;

		createObjectWebcamImage(webcamView);
		createWebcamTextureImageView(webcamView);

		if (webcamView.isAbsolute) {
			webcamView.updateAbsScale();
		}

		webcamView.createVertices(engine.windowWidth, engine.windowHeight);

		createUIVertexBuffer(&webcamView);
		createIndexBuffer(&webcamView);
		createDescriptorPool(&webcamView);
		createDescriptorSets(&webcamView);

		UIobjects.push_back(webcamView);
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(engine.window)) {
			glfwPollEvents();
			glfwGetCursorPos(engine.window, &mouseX, &mouseY);
			if (defaultKeyBinds.getIsKeyDown(GLFW_KEY_U)) {
				webCam.shouldUpdate = false;
			}
			if (defaultKeyBinds.getIsKeyDown(GLFW_KEY_I)) {
				webCam.shouldUpdate = true;
			}
			updateWebcam();
			updateWebcamImage(UIobjects[0]);
			int state = glfwGetMouseButton(engine.window, GLFW_MOUSE_BUTTON_LEFT);
			for (UIItem* item : ObjectButtons->Items) {
				item->checkForEvent(mouseX, mouseY, state);
			}
			if (defaultKeyBinds.getIsKeyDown(GLFW_KEY_L)) {
				engine.pipelineindex = 3;
			}
			if (defaultKeyBinds.getIsKeyDown(GLFW_KEY_0)) {
				webCam.calibrateCornerFilter();
				webCam.getCorners();
			}
			if (webcamObjectView) {
				for (int i = 0; i != staticObjects.size(); i++) {
					updateWebcamImage(staticObjects[i]);
				}
			}
			if (Rendered->isInArea(mouseX, mouseY) && state == GLFW_PRESS) {
				engine.pipelineindex = 1;
			}
			if (Unrendered->isInArea(mouseX, mouseY) && state == GLFW_PRESS) {
				engine.pipelineindex = 0;
			}
			if (LB->isInArea(mouseX, mouseY) && state == GLFW_PRESS) {
				loadStaticObject();
			}
			drawFrame();
		}

		vkDeviceWaitIdle(engine.device);
	}

	void setObjectVisibilities(UIItem* owner) {
		cout << "Heard an event from " << owner->Name << endl;
		for (int i = 0; i != staticObjects.size(); i++) {
			staticObjects[i].isVisible = false;
		}
		staticObjects[ObjectMap.at(owner->Name)].isVisible = true;
	}

	void loadStaticObject() {
		StaticObject newObject;
		
		bool res = newObject.loadModel();

		if (res) {
			createVertexBuffer(newObject);
			createIndexBuffer(&newObject);
			//createTextureImage(newObject); 
			createObjectWebcamImage(newObject);
			//createTextureImageView(newObject);
			createWebcamTextureImageView(newObject);
			createDescriptorPool(&newObject);
			createDescriptorSets(&newObject);

			std::function<void(UIItem*)> testfunction = bind(&Application::setObjectVisibilities, this, placeholders::_1);

			Button* objectButton = new Button(0, 0, 0.15, 0.15, UNRENDERED_BUTTON_PATH, engine.windowWidth, engine.windowHeight);

			objectButton->updateDisplay(engine.windowWidth, engine.windowHeight);
			objectButton->setClickFunction(testfunction);

			createTextureImage(objectButton->image);
			createTextureImageView(objectButton->image);
			createUIVertexBuffer(objectButton->image);
			createIndexBuffer(objectButton->image);
			createDescriptorPool(objectButton->image);
			createDescriptorSets(objectButton->image);

			objectButton->Name = "Object button " + std::to_string(ObjectButtons->Items.size());

			ObjectMap.insert({ objectButton->Name, staticObjects.size() });

			ObjectButtons->addItem(objectButton);
			ObjectButtons->arrangeItems(engine.windowWidth, engine.windowHeight);

			ObjectButtons->updateDisplay(engine.windowWidth, engine.windowHeight);
			vector<UIImage*> images;
			ObjectButtons->getImages(images);
			for (UIImage* image : images) {
				updateUIVertexBuffer(*image);
			}

			for (int i = 0; i != staticObjects.size(); i++) {
				staticObjects[i].isVisible = false;
			}
			newObject.isVisible = true;

			staticObjects.push_back(newObject);
		}
	}

	void cleanup() {

		for (uint32_t i = 0; i != staticObjects.size(); i++) {
			vkDestroyDescriptorPool(engine.device, staticObjects[i].descriptorPool, nullptr);

			vkDestroyBuffer(engine.device, staticObjects[i].indexBuffer, nullptr);
			vkFreeMemory(engine.device, staticObjects[i].indexBufferMemory, nullptr);

			vkDestroyBuffer(engine.device, staticObjects[i].vertexBuffer, nullptr);
			vkFreeMemory(engine.device, staticObjects[i].vertexBufferMemory, nullptr);
			
			vkDestroyImage(engine.device, staticObjects[i].textureImage, nullptr);
			vkFreeMemory(engine.device, staticObjects[i].textureImageMemory, nullptr);

			vkDestroyImageView(engine.device, staticObjects[i].textureImageView, nullptr);
		}

		for (uint32_t i = 0; i != canvas.size(); i++) {
			vector<UIImage*> images;
			canvas[i]->getImages(images);

			for (UIImage *image : images) {
				vkDestroyDescriptorPool(engine.device, image->descriptorPool, nullptr);

				vkDestroyBuffer(engine.device, image->indexBuffer, nullptr);
				vkFreeMemory(engine.device, image->indexBufferMemory, nullptr);

				vkDestroyBuffer(engine.device, image->vertexBuffer, nullptr);
				vkFreeMemory(engine.device, image->vertexBufferMemory, nullptr);

				vkDestroyImage(engine.device, image->textureImage, nullptr);
				vkFreeMemory(engine.device, image->textureImageMemory, nullptr);

				vkDestroyImageView(engine.device, image->textureImageView, nullptr);
			}
		}

		for (uint32_t i = 0; i != UIobjects.size(); i++) {
			vkDestroyDescriptorPool(engine.device, UIobjects[i].descriptorPool, nullptr);

			vkDestroyBuffer(engine.device, UIobjects[i].indexBuffer, nullptr);
			vkFreeMemory(engine.device, UIobjects[i].indexBufferMemory, nullptr);

			vkDestroyBuffer(engine.device, UIobjects[i].vertexBuffer, nullptr);
			vkFreeMemory(engine.device, UIobjects[i].vertexBufferMemory, nullptr);

			vkDestroyImage(engine.device, UIobjects[i].textureImage, nullptr);
			vkFreeMemory(engine.device, UIobjects[i].textureImageMemory, nullptr);

			vkDestroyImageView(engine.device, UIobjects[i].textureImageView, nullptr);
		}

		engine.cleanup();
	}

	VkSampleCountFlagBits getMaxUseableSampleCount() {
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(engine.physicalDevice, &physicalDeviceProperties);

		VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;

		if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
		if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
		if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
		if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
		if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
		if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

		return VK_SAMPLE_COUNT_1_BIT;
	}

	void createTextureImage(auto *object) {
		int texWidth, texHeight, texChannels;
		//string TEXTURE_PATH = winFile::OpenFileDialog();
		
		string TEXTURE_PATH = object->texPath;
		stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		object->texWidth = texWidth;
		object->texHeight = texHeight;

		if (!pixels) {
			throw runtime_error("failed to load texture image!");
		}

		engine.mipLevels = static_cast<uint32_t>(floor(log2(max(texWidth, texHeight)))) + 1; 

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		engine.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(engine.device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(engine.device, stagingBufferMemory);

		stbi_image_free(pixels);

		createImage(texWidth, texHeight, engine.mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, object->textureImage, object->textureImageMemory);
		
		transitionImageLayout(object->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, engine.mipLevels);
		copyBufferToImage(stagingBuffer, object->textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

		vkDestroyBuffer(engine.device, stagingBuffer, nullptr);
		vkFreeMemory(engine.device, stagingBufferMemory, nullptr);

		generateMipmaps(object->textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, engine.mipLevels);
	}

	void createWebcamImage() {
		
		webCam.getFrame();
		cv::Mat camFrame = webCam.webcamFrame;
		uchar* camData = new uchar[camFrame.total() * 4];
		Mat continuousRGBA(camFrame.size(), CV_8UC4, camData);
		cv::cvtColor(camFrame, continuousRGBA, cv::COLOR_BGR2RGBA, 4);
		//cv::imshow("camFrame", webCam.webcamFrame);

		int texWidth, texHeight, texChannels;

		texWidth = continuousRGBA.size().width;
		texHeight = continuousRGBA.size().height;
		texChannels = continuousRGBA.channels();

		VkDeviceSize imageSize = continuousRGBA.total() * continuousRGBA.elemSize();

		webWidth = texWidth;
		webHeight = texHeight;

		engine.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, textureBuffer, textureBufferMemory);

		vkMapMemory(engine.device, textureBufferMemory, 0, imageSize, 0, &tBuffer);
		memcpy(tBuffer, continuousRGBA.ptr(), static_cast<size_t>(imageSize));
	}

	void createObjectWebcamImage(auto& object) {
		object.mipLevels = 1;
		
		createImage(webWidth, webHeight, object.mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, object.textureImage, object.textureImageMemory);

		transitionImageLayout(object.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, object.mipLevels);
		copyBufferToImage(textureBuffer, object.textureImage, static_cast<uint32_t>(webWidth), static_cast<uint32_t>(webHeight));
		generateMipmaps(object.textureImage, VK_FORMAT_R8G8B8A8_SRGB, webWidth, webHeight, object.mipLevels);
	}

	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(engine.physicalDevice, imageFormat, &formatProperties);

		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
			throw runtime_error("texture image format does not support linear blitting!");
		}
		
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = texWidth;
		int32_t mipHeight = texHeight;

		for (uint32_t i = 1; i < mipLevels; i++) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0,0,0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0,0,0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit, VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		endSingleTimeCommands(commandBuffer);
	}

	void createTextureImageView(auto *object) {
		object->textureImageView = createImageView(object->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, engine.mipLevels);
	}
	void createWebcamTextureImageView(auto& object) {
		object.textureImageView = createImageView(object.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, object.mipLevels);
	}

	bool hasStencilComponent(VkFormat format) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;

		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = mipLevels;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		if (vkCreateImageView(engine.device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
			throw runtime_error("failed to create texture image view!");
		}

		return imageView;
	}

	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = mipLevels;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;

		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		imageInfo.samples = numSamples;

		if (vkCreateImage(engine.device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
			throw runtime_error("failed to create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(engine.device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = engine.findMemoryType(memRequirements.memoryTypeBits, properties); 

		if (vkAllocateMemory(engine.device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
			throw runtime_error("failed to allocate image memory!");
		}

		vkBindImageMemory(engine.device, image, imageMemory, 0);
	}

	VkCommandBuffer beginSingleTimeCommands() {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = engine.commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(engine.device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(engine.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(engine.graphicsQueue);

		vkFreeCommandBuffers(engine.device, engine.commandPool, 1, &commandBuffer);
	}

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();
		
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;

		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = image;
		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			if (hasStencilComponent(format)) {
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = mipLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else {
			throw invalid_argument("unsupported layout transition!");
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		endSingleTimeCommands(commandBuffer);
	}

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0,0,0 };
		region.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);

		endSingleTimeCommands(commandBuffer);
	}

	void drawFrame() {
		vkWaitForFences(engine.device, 1, &engine.inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(engine.device, engine.swapChain, UINT64_MAX, engine.imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			engine.recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw runtime_error("failed to acquire swap chain image!");
		}

		updateUniformBuffer(currentFrame);

		vkResetFences(engine.device, 1, &engine.inFlightFences[currentFrame]);

		vkResetCommandBuffer(engine.commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		recordCommandBuffer(engine.commandBuffers[currentFrame], imageIndex);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { engine.imageAvailableSemaphores[currentFrame]};
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &engine.commandBuffers[currentFrame];

		VkSemaphore signalSemaphores[] = { engine.renderFinishedSemaphores[currentFrame]};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(engine.graphicsQueue, 1, &submitInfo, engine.inFlightFences[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { engine.swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &imageIndex;

		result = vkQueuePresentKHR(engine.presentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || engine.framebufferResized) {
			engine.framebufferResized = false;

			for (size_t i = 0; i != canvas.size(); i++) {
				canvas[i]->updateDisplay(engine.windowWidth, engine.windowHeight);
				vector<UIImage*> images;
				canvas[i]->getImages(images);
				for (UIImage *image : images) {
					updateUIVertexBuffer(*image);
				}
			}

			for (uint32_t i = 0; i != UIobjects.size(); i++) {
				UIobjects[i].createVertices(engine.windowWidth, engine.windowHeight);

				updateUIVertexBuffer(UIobjects[i]);
			}

			engine.recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS) {
			throw runtime_error("failed to acquire swap chain image!");
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void updateUniformBuffer(uint32_t currentImage) {

		camera.updateCamera(engine.window);

		UniformBufferObject ubo{};
		ubo.model = glm::mat4(1.0f); //Defines model translation, rotation and scale 
		ubo.view = camera.view;
		ubo.proj = glm::perspective(glm::radians(camera.fov), engine.swapChainExtent.width / (float)engine.swapChainExtent.height, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		memcpy(engine.uniformBuffersMapped[currentImage], &ubo, sizeof(ubo)); // uniformBuffersMapped is an array of pointers to each uniform buffer 
	} 

	void createVertexBuffer(auto& object) {
		// copies mesh vertex data into GPU memory

		VkDeviceSize bufferSize = sizeof(object.vertices[0]) * object.vertices.size();
		
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		engine.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(engine.device, stagingBufferMemory, 0, bufferSize, 0, &data);
			memcpy(data, object.vertices.data(), (size_t) bufferSize);
		vkUnmapMemory(engine.device, stagingBufferMemory);

		engine.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, object.vertexBuffer, object.vertexBufferMemory);

		copyBuffer(stagingBuffer, object.vertexBuffer, bufferSize);

		vkDestroyBuffer(engine.device, stagingBuffer, nullptr);
		vkFreeMemory(engine.device, stagingBufferMemory, nullptr);
	}

	void createUIVertexBuffer(auto *object) {
		// copies mesh vertex data into GPU memory

		VkDeviceSize bufferSize = sizeof(object->vertices[0]) * object->vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		engine.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(engine.device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, object->vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(engine.device, stagingBufferMemory);

		engine.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, object->vertexBuffer, object->vertexBufferMemory);

		copyBuffer(stagingBuffer, object->vertexBuffer, bufferSize);

		vkMapMemory(engine.device, object->vertexBufferMemory, 0, bufferSize, 0, &object->vBuffer);

		vkDestroyBuffer(engine.device, stagingBuffer, nullptr);
		vkFreeMemory(engine.device, stagingBufferMemory, nullptr);
	}

	void updateUIVertexBuffer(auto& object) {
		VkDeviceSize bufferSize = sizeof(object.vertices[0]) * object.vertices.size();
		memcpy(object.vBuffer, object.vertices.data(), (size_t)bufferSize);
	}

	void createIndexBuffer(auto *object) {
		// copies mesh index data into GPU memory

		VkDeviceSize bufferSize = sizeof(object->indices[0]) * object->indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		engine.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(engine.device, stagingBufferMemory, 0, bufferSize, 0, &data);
			memcpy(data, object->indices.data(), (size_t)bufferSize);
		vkUnmapMemory(engine.device, stagingBufferMemory);

		engine.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, object->indexBuffer, object->indexBufferMemory);

		copyBuffer(stagingBuffer, object->indexBuffer, bufferSize);

		vkDestroyBuffer(engine.device, stagingBuffer, nullptr);
		vkFreeMemory(engine.device, stagingBufferMemory, nullptr);
	}

	void createDescriptorPool(auto *object) {
		array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		if (vkCreateDescriptorPool(engine.device, &poolInfo, nullptr, &object->descriptorPool) != VK_SUCCESS) {
			throw runtime_error("failed to create descriptor pool!");
		}
	}


	void createDescriptorSets(auto *object) {
		vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, engine.descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = object->descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		object->descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		
		if (vkAllocateDescriptorSets(engine.device, &allocInfo, object->descriptorSets.data()) != VK_SUCCESS) {
			throw runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = engine.uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = object->textureImageView;
			imageInfo.sampler = engine.textureSampler;

			array<VkWriteDescriptorSet, 2> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = object->descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = object->descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(engine.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		endSingleTimeCommands(commandBuffer);
	}

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw runtime_error("failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = engine.renderPass;
		renderPassInfo.framebuffer = engine.swapChainFramebuffers[imageIndex];

		renderPassInfo.renderArea.offset = { 0,0 };
		renderPassInfo.renderArea.extent = engine.swapChainExtent;

		array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {0.812f, 0.2f, 0.2f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(engine.swapChainExtent.width);
		viewport.height = static_cast<float>(engine.swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0,0 };
		scissor.extent = engine.swapChainExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *engine.GraphicsPipelines[engine.PipelineMap.at("UIShading")]);

		for (uint32_t i = 0; i != UIobjects.size(); i++) {
			VkBuffer vertexBuffers[] = { UIobjects[i].vertexBuffer};
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(commandBuffer, UIobjects[i].indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, engine.pipelineLayout, 0, 1, &UIobjects[i].descriptorSets[currentFrame], 0, nullptr);

			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(UIobjects[i].indices.size()), 1, 0, 0, 0);
		}

		for (uint32_t i = 0; i != canvas.size(); i++) {
			vector<UIImage*> images;
			canvas[i]->getImages(images);

			for (UIImage *image : images) {
				VkBuffer vertexBuffers[] = { image->vertexBuffer };
				VkDeviceSize offsets[] = { 0 };

				vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

				vkCmdBindIndexBuffer(commandBuffer, image->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, engine.pipelineLayout, 0, 1, &image->descriptorSets[currentFrame], 0, nullptr);

				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(image->indices.size()), 1, 0, 0, 0);
			}
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *engine.GraphicsPipelines[engine.pipelineindex]);
		
		for (uint32_t i = 0; i != staticObjects.size(); i++) {
			if (staticObjects[i].isVisible) {
				VkBuffer vertexBuffers[] = { staticObjects[i].vertexBuffer };
				VkDeviceSize offsets[] = { 0 };

				vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

				vkCmdBindIndexBuffer(commandBuffer, staticObjects[i].indexBuffer, 0, VK_INDEX_TYPE_UINT32);

				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, engine.pipelineLayout, 0, 1, &staticObjects[i].descriptorSets[currentFrame], 0, nullptr);

				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(staticObjects[i].indices.size()), 1, 0, 0, 0);
			}
		}	

		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw runtime_error("failed to record command buffer!");
		}
	}
};

int main()
{
	Application app;

	try {
		app.run();
	}
	catch (const exception& e) {
		cerr << e.what() << endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
