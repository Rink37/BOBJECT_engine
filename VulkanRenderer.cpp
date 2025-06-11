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
#include <opencv2/opencv.hpp>

#include"Bobject_Engine.h"
#include"InputManager.h"
#include"WindowsFileManager.h"
#include"CameraController.h"
#include"UIelements.h"
#include"Webcam_feeder.h"
#include"Textures.h"
#include"Materials.h"
#include"Meshes.h"
#include"GenerateNormalMap.h"

#include"include/LoadButton.h"
#include"include/PauseButton.h"
#include"include/PlayButton.h"
#include"include/RenderedButton.h"
#include"include/SettingsButton.h"
#include"include/UnrenderedButton.h"
#include"include/WireframeButton.h"
#include"include/TestCheckboxButton.h"

using namespace cv;
using namespace std;

//uint32_t currentFrame = 0;

vector<int> keybinds = { GLFW_KEY_L, GLFW_KEY_0, GLFW_KEY_1, GLFW_KEY_U, GLFW_KEY_I };

std::vector<KeyInput*> KeyInput::_instances;
KeyInput defaultKeyBinds(keybinds);

struct StaticObject {
	bool isVisible = false;
	StaticMesh mesh;
	Material* mat = nullptr;
};

class Application {
public:
	void run() {
		engine->initWindow("BOBERT_TradPainter");
		engine->initVulkan();
		KeyInput::setupKeyInputs(engine->window);
		glfwSetScrollCallback(engine->window, camera.scrollCallback);
		createWebcamMaterial();
		createCanvas();
		mapGenerator.setup();
		mainLoop();
		cleanup();
		Engine::destruct();
	}
private:
	Engine* engine = Engine::get();

	Camera camera;
	WebcamPanel* webcamView = nullptr;
	Material* webcamMaterial = nullptr;

	imageData* ub = new UNRENDEREDBUTTON;
	imageData* tcb = new TESTCHECKBOXBUTTON;

	vector<UIItem*> canvas{};

	vArrangement* ObjectButtons;

	double mouseX, mouseY = 0;

	bool mouseDown = false;

	vector<StaticObject> staticObjects = {};
	map<string, int> ObjectMap = {};

	bool webcamObjectView = true;

	NormalGen mapGenerator;
	bool shouldRenderOSN = false;
	bool OSNavailable = false;

	void createWebcamMaterial() {
		webcamTexture::get()->setup();
		webcamMaterial = new Material(webcamTexture::get());
	}

	void createCanvas() {
		hArrangement *Renderbuttons = new hArrangement(0.0f, 0.0f, 0.15f, 0.1f, 0.01f);
		hArrangement *Videobuttons = new hArrangement(0.775f, 0.4f, 0.2f, 0.1f, 0.01f);

		std::function<void(UIItem*)> pipelinefunction = bind(&Application::setPipelineIndex, this, placeholders::_1);

		std::function<void(UIItem*)> enableWebcamFunct = bind(&Application::enableWebcam, this, placeholders::_1);
		std::function<void(UIItem*)> disableWebcamFunct = bind(&Application::disableWebcam, this, placeholders::_1);
		std::function<void(UIItem*)> configureWebcamFunct = bind(&Application::calibrateWebcam, this, placeholders::_1);

		std::function<void(UIItem*)> loadObjectFunct = bind(&Application::buttonLoadStaticObject, this, placeholders::_1);

		imageData* lb = new LOADBUTTON;
		imageData* rb = new RENDEREDBUTTON;
		imageData* ub = new UNRENDEREDBUTTON;
		imageData* wb = new WIREFRAMEBUTTON;
		imageData* plb = new PLAYBUTTON;
		imageData* pb = new PAUSEBUTTON;
		imageData* sb = new SETTINGSBUTTON;

		Button *loadObjectButton = new Button(0.0f, 0.0f, 0.2f, 0.1f, lb);
		
		Button *litRenderingButton = new Button(0.0f, 0.0f, 0.2f, 0.2f, rb);
		Button *unlitRenderingButton = new Button(0.0f, 0.0f, 0.2f, 0.2f, ub);
		Button *wireframeRenderingButton = new Button(0.0f, 0.0f, 0.2f, 0.2f, wb);
		
		Button* playButton = new Button(0.0f, 0.0f, 0.2f, 0.2f, plb);
		Button* pauseButton = new Button(0.0f, 0.0f, 0.2f, 0.2f, pb);
		Button* settingsButton = new Button(0.0f, 0.0f, 0.2f, 0.2f, sb);

		unlitRenderingButton->Name = "FlatShading";
		unlitRenderingButton->setClickFunction(pipelinefunction);
		
		litRenderingButton->Name = "BFShading";
		litRenderingButton->setClickFunction(pipelinefunction);
		
		wireframeRenderingButton->Name = "Wireframe";
		wireframeRenderingButton->setClickFunction(pipelinefunction);

		playButton->Name = "PlayWebcam";
		playButton->setClickFunction(enableWebcamFunct);
		
		pauseButton->Name = "PauseWebcam";
		pauseButton->setClickFunction(disableWebcamFunct);
		
		settingsButton->Name = "ConfigureWebcam";
		settingsButton->setClickFunction(configureWebcamFunct);

		loadObjectButton->Name = "LoadObject";
		loadObjectButton->setClickFunction(loadObjectFunct);

		Renderbuttons->addItem(unlitRenderingButton);
		Renderbuttons->addItem(litRenderingButton);
		Renderbuttons->addItem(wireframeRenderingButton);

		Videobuttons->addItem(playButton);
		Videobuttons->addItem(pauseButton);
		Videobuttons->addItem(settingsButton);

		canvas.push_back(Videobuttons);

		vArrangement* buttons = new vArrangement(-1.0f, 1.0f, 0.15f, 0.15f, 0.0f);

		buttons->addItem(loadObjectButton);
		buttons->addItem(Renderbuttons);

		ObjectButtons = new vArrangement(-0.9f, -0.5f, 0.05f, 0.5f, 0.01f);

		canvas.push_back(ObjectButtons);

		canvas.push_back(buttons);

		webcamView = new WebcamPanel(0.775f, 0.1f, 0.2f, 0.142f, webcamMaterial);
		webcamView->image->mat[0] = webcamMaterial;

		canvas.push_back(webcamView);

		for (UIItem* item : canvas) {
			item->updateDisplay();
		}
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(engine->window)) {
			glfwPollEvents();
			glfwGetCursorPos(engine->window, &mouseX, &mouseY);
			webcamTexture::get()->updateWebcam();
			int state = glfwGetMouseButton(engine->window, GLFW_MOUSE_BUTTON_LEFT);
			if (state == GLFW_PRESS) {
				mouseDown = 1;
			}
			else if (~state && mouseDown){
				for (UIItem* item : canvas) {
					vector<UIItem*> scs;
					item->getSubclasses(scs);
					for (UIItem* sitem : scs) {
						sitem->checkForEvent(mouseX, mouseY, GLFW_PRESS);
					}
				}
				mouseDown = 0;
			}
			if (OSNavailable) {
				mapGenerator.OSNormalMap = convertToCVMat(mapGenerator.objectSpaceMap.colour.image, 1024, 1024);
				OSNavailable = false;
				string filepath = winFile::OpenFileDialog();
				if (filepath != (string)"fail") {
					Mat srcImg = imread(filepath);
					mapGenerator.contextualConvertMap(srcImg);
				}
			}
			if (defaultKeyBinds.getIsKeyDown(GLFW_KEY_L)) {
				shouldRenderOSN = true;
			}
			drawFrame();
		}
		vkDeviceWaitIdle(engine->device);
	}

	Mat convertToCVMat(VkImage srcImage, uint32_t width, uint32_t height) {
		// see https://github.com/SaschaWillems/Vulkan/blob/master/examples/screenshot/screenshot.cpp 
		
		bool supportsBlit = true;

		VkFormatProperties formatProps;

		vkGetPhysicalDeviceFormatProperties(Engine::get()->physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
		if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
			std::cerr << "Device does not support blitting from optimal tiled images, using copy instead of blit!" << std::endl;
			supportsBlit = false;
		}

		vkGetPhysicalDeviceFormatProperties(Engine::get()->physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
		if (!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
			std::cerr << "Device does not support blitting to linear tiled images, using copy instead of blit!" << std::endl;
			supportsBlit = false;
		}

		VkImageCreateInfo imageCreateCi = {};
		imageCreateCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateCi.imageType = VK_IMAGE_TYPE_2D;
		imageCreateCi.format = VK_FORMAT_R8G8B8A8_UNORM;
		imageCreateCi.extent.width = width;
		imageCreateCi.extent.height = height;
		imageCreateCi.extent.depth = 1;
		imageCreateCi.arrayLayers = 1;
		imageCreateCi.mipLevels = 1;
		imageCreateCi.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateCi.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateCi.tiling = VK_IMAGE_TILING_LINEAR;
		imageCreateCi.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		VkImage dstImage;
		if (vkCreateImage(Engine::get()->device, &imageCreateCi, nullptr, &dstImage) != VK_SUCCESS) {
			throw runtime_error("Failed to create image");
		}

		VkMemoryRequirements memRequirements;
		VkMemoryAllocateInfo memAllocInfo = {};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		VkDeviceMemory dstImageMemory;
		vkGetImageMemoryRequirements(Engine::get()->device, dstImage, &memRequirements);
		memAllocInfo.allocationSize = memRequirements.size;
		
		memAllocInfo.memoryTypeIndex = Engine::get()->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		if (vkAllocateMemory(Engine::get()->device, &memAllocInfo, nullptr, &dstImageMemory) != VK_SUCCESS) {
			throw runtime_error("Failed to allocate memory");
		}
		if (vkBindImageMemory(Engine::get()->device, dstImage, dstImageMemory, 0) != VK_SUCCESS) {
			throw runtime_error("Failed to bind image memory");
		}

		VkCommandBuffer copyCmd;
		
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = Engine::get()->commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(Engine::get()->device, &allocInfo, &copyCmd) != VK_SUCCESS) {
			throw runtime_error("failed to allocate command buffer!");
		}

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(copyCmd, &beginInfo) != VK_SUCCESS) {
			throw runtime_error("failed to begin recording command buffer!");
		}

		VkImageMemoryBarrier imageMemoryBarrier = {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.image = dstImage;
		imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		vkCmdPipelineBarrier(
			copyCmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);

		imageMemoryBarrier = {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imageMemoryBarrier.image = srcImage;
		imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		vkCmdPipelineBarrier(
			copyCmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);

		if (supportsBlit)
		{
			VkOffset3D blitSize;
			blitSize.x = width;
			blitSize.y = height;
			blitSize.z = 1;
			VkImageBlit imageBlitRegion{};
			imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlitRegion.srcSubresource.layerCount = 1;
			imageBlitRegion.srcOffsets[1] = blitSize;
			imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlitRegion.dstSubresource.layerCount = 1;
			imageBlitRegion.dstOffsets[1] = blitSize;

			vkCmdBlitImage(
				copyCmd,
				srcImage , VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&imageBlitRegion,
				VK_FILTER_NEAREST);
		}
		else
		{
			VkImageCopy imageCopyRegion{};
			imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageCopyRegion.srcSubresource.layerCount = 1;
			imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageCopyRegion.dstSubresource.layerCount = 1;
			imageCopyRegion.extent.width = width;
			imageCopyRegion.extent.height = height;
			imageCopyRegion.extent.depth = 1;

			vkCmdCopyImage(
				copyCmd,
				srcImage , VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&imageCopyRegion);
		}
		imageMemoryBarrier = {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageMemoryBarrier.image = dstImage;
		imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		vkCmdPipelineBarrier(
			copyCmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);

		imageMemoryBarrier = {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imageMemoryBarrier.image = srcImage;
		imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		vkCmdPipelineBarrier(
			copyCmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);

		if (vkEndCommandBuffer(copyCmd) != VK_SUCCESS) {
			throw runtime_error("Failed to end command buffer");
		}

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &copyCmd;

		vkQueueSubmit(Engine::get()->graphicsQueue, 1, &submitInfo, nullptr);

		vkFreeCommandBuffers(Engine::get()->device, Engine::get()->commandPool, 1, &copyCmd);

		VkImageSubresource subResource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
		VkSubresourceLayout subResourceLayout;
		vkGetImageSubresourceLayout(Engine::get()->device, dstImage, &subResource, &subResourceLayout);

		const char* data;
		vkMapMemory(Engine::get()->device, dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&data);
		data += subResourceLayout.offset;

		const char* filename = "Temp.ppm";

		std::ofstream file(filename, std::ios::out | std::ios::binary);

		file << "P6\n" << width << "\n" << height << "\n" << 255 << "\n";

		bool colorSwizzle = false;

		if (!supportsBlit)
		{
			std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
			colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), VK_FORMAT_R8G8B8A8_UNORM) != formatsBGR.end());
		}

		for (uint32_t y = 0; y < height; y++)
		{
			unsigned int* row = (unsigned int*)data;
			for (uint32_t x = 0; x < width; x++)
			{
				if (colorSwizzle)
				{
					file.write((char*)row + 2, 1);
					file.write((char*)row + 1, 1);
					file.write((char*)row, 1);
				}
				else
				{
					file.write((char*)row, 3);
				}
				row++;
			}
			data += subResourceLayout.rowPitch;
		}
		file.close();

		std::cout << "Image saved" << std::endl;

		vkUnmapMemory(Engine::get()->device, dstImageMemory);
		vkFreeMemory(Engine::get()->device, dstImageMemory, nullptr);
		vkDestroyImage(Engine::get()->device, dstImage, nullptr);

		Mat cvImg = imread((cv::String)filename);

		std::remove(filename);

		//string windowName = "TestImage";
		//namedWindow(windowName);
		//while (true){
		//	imshow(windowName, cvImg);//Show the frame
		//	char c = (char)waitKey(25); //Waits for us to press 'Esc', then exits
		//	if (c == 27) {
		//		cv::destroyWindow(windowName);
		//		break;
		//	}
		//	if (getWindowProperty(windowName, WND_PROP_VISIBLE) < 1) {
		//		break;
		//	}
		//}

		return cvImg;
	}

	void enableWebcam(UIItem* owner) {
		webcamTexture::get()->webCam.shouldUpdate = true;
	}

	void disableWebcam(UIItem* owner) {
		webcamTexture::get()->webCam.shouldUpdate = false;
	}

	void calibrateWebcam(UIItem* owner) {
		webcamTexture::get()->webCam.calibrateCornerFilter();
	}

	void buttonLoadStaticObject(UIItem* owner) {
		loadStaticObject();
	}

	void setObjectVisibility(UIItem* owner) {
		staticObjects[ObjectMap.at(owner->Name)].isVisible = owner->activestate;
	}

	void setPipelineIndex(UIItem* owner) {
		engine->pipelineindex = engine->PipelineMap.at(owner->Name);
	}

	void loadStaticObject() {

		try {
			StaticObject newObject;
			newObject.mat = webcamMaterial;

			std::function<void(UIItem*)> testfunction = bind(&Application::setObjectVisibility, this, placeholders::_1);

			Checkbox* objectButton = new Checkbox(0.0f, 0.0f, 0.15f, 0.15f, tcb, ub);

			objectButton->updateDisplay();
			objectButton->setClickFunction(testfunction);

			newObject.isVisible = objectButton->activestate;

			objectButton->Name = "Object button " + std::to_string(ObjectButtons->Items.size());

			ObjectMap.insert({ objectButton->Name, staticObjects.size() });

			ObjectButtons->addItem(objectButton);
			ObjectButtons->arrangeItems();

			ObjectButtons->updateDisplay();

			staticObjects.push_back(newObject);
		}
		catch (...) {
			return;
		}
	}

	void cleanup() {

		mapGenerator.cleanup();

		for (uint32_t i = 0; i != staticObjects.size(); i++) {
			if (staticObjects[i].mat != webcamMaterial) {
				staticObjects[i].mat->cleanup();
			}

			staticObjects[i].mesh.cleanup();
		}

		for (uint32_t i = 0; i != canvas.size(); i++) {
			vector<UIImage*> images;
			canvas[i]->getImages(images);

			for (UIImage *image : images) {
				for (Material* mat : image->mat) {
					if (mat != webcamMaterial) {
						mat->cleanup();
					}
				}

				image->mesh.cleanup();

			}
		}

		webcamMaterial->cleanup();
		engine->cleanup();
	}

	void drawFrame() {
		uint32_t currentFrame = engine->currentFrame;
		vkWaitForFences(engine->device, 1, &engine->inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(engine->device, engine->swapChain, UINT64_MAX, engine->imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			engine->recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw runtime_error("failed to acquire swap chain image!");
		}

		updateUniformBuffer(currentFrame);

		vkResetFences(engine->device, 1, &engine->inFlightFences[currentFrame]);

		vkResetCommandBuffer(engine->commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		recordCommandBuffer(engine->commandBuffers[currentFrame], imageIndex);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { engine->imageAvailableSemaphores[currentFrame]};
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &engine->commandBuffers[currentFrame];

		VkSemaphore signalSemaphores[] = { engine->renderFinishedSemaphores[currentFrame]};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(engine->graphicsQueue, 1, &submitInfo, engine->inFlightFences[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { engine->swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &imageIndex;

		result = vkQueuePresentKHR(engine->presentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || engine->framebufferResized) {
			engine->framebufferResized = false;

			for (size_t i = 0; i != canvas.size(); i++) {
				canvas[i]->updateDisplay();
			}

			engine->recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS) {
			throw runtime_error("failed to acquire swap chain image!");
		}

		engine->currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void updateUniformBuffer(uint32_t currentImage) {

		camera.updateCamera(engine->window);

		UniformBufferObject ubo{};
		ubo.model = glm::mat4(1.0f);
		ubo.view = camera.view;
		ubo.proj = glm::perspective(glm::radians(camera.fov), engine->swapChainExtent.width / (float)engine->swapChainExtent.height, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		ubo.UVdistort[0] = 2*webcamView->extentx;
		ubo.UVdistort[1] = (webcamView->posx) - webcamView->extentx;
		ubo.UVdistort[2] = 2*webcamView->extenty;
		ubo.UVdistort[3] = (-webcamView->posy) - webcamView->extenty;

		memcpy(engine->uniformBuffersMapped[currentImage], &ubo, sizeof(ubo)); // uniformBuffersMapped is an array of pointers to each uniform buffer 
	} 

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
		uint32_t currentFrame = engine->currentFrame;

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw runtime_error("failed to begin recording command buffer!");
		}

		if (shouldRenderOSN) {
			commandBuffer = mapGenerator.draw(commandBuffer, &staticObjects[staticObjects.size() - 1].mesh);
			shouldRenderOSN = false;
			OSNavailable = true;
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = engine->renderPass;
		renderPassInfo.framebuffer = engine->swapChainFramebuffers[imageIndex];

		renderPassInfo.renderArea.offset = { 0,0 };
		renderPassInfo.renderArea.extent = engine->swapChainExtent;

		array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {0.812f, 0.2f, 0.2f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(engine->swapChainExtent.width);
		viewport.height = static_cast<float>(engine->swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0,0 };
		scissor.extent = engine->swapChainExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *engine->GraphicsPipelines[engine->PipelineMap.at("UVWireframe")]);

		for (uint32_t i = 0; i != staticObjects.size(); i++) {
			if (staticObjects[i].isVisible) {
				VkBuffer vertexBuffers[] = { staticObjects[i].mesh.vertexBuffer };
				VkDeviceSize offsets[] = { 0 };

				vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

				vkCmdBindIndexBuffer(commandBuffer, staticObjects[i].mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, engine->pipelineLayout, 0, 1, &staticObjects[i].mat->descriptorSets[currentFrame], 0, nullptr);

				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(staticObjects[i].mesh.indices.size()), 1, 0, 0, 0);
			}
		}
		
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *engine->GraphicsPipelines[engine->PipelineMap.at("UIShading")]);

		for (uint32_t i = 0; i != canvas.size(); i++) {
			vector<UIImage*> images;
			canvas[i]->getImages(images);

			for (UIImage *image : images) {
				VkBuffer vertexBuffers[] = { image->mesh.vertexBuffer };
				VkDeviceSize offsets[] = { 0 };

				vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

				vkCmdBindIndexBuffer(commandBuffer, image->mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, engine->pipelineLayout, 0, 1, &image->mat[image->matidx]->descriptorSets[currentFrame], 0, nullptr);

				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(image->mesh.indices.size()), 1, 0, 0, 0);
			}
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *engine->GraphicsPipelines[engine->pipelineindex]);
		
		for (uint32_t i = 0; i != staticObjects.size(); i++) {
			if (staticObjects[i].isVisible) {
				VkBuffer vertexBuffers[] = { staticObjects[i].mesh.vertexBuffer };
				VkDeviceSize offsets[] = { 0 };

				vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

				vkCmdBindIndexBuffer(commandBuffer, staticObjects[i].mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, engine->pipelineLayout, 0, 1, &staticObjects[i].mat->descriptorSets[currentFrame], 0, nullptr);

				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(staticObjects[i].mesh.indices.size()), 1, 0, 0, 0);
			}
		}

		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw runtime_error("failed to record command buffer!");
		}
	}
};

webcamTexture* webcamTexture::winstance = nullptr;
Engine* Engine::enginstance = nullptr;

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
