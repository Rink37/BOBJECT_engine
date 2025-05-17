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
#include"InputManager.h"
#include"WindowsFileManager.h"
#include"CameraController.h"
#include"UIelements.h"
#include"Webcam_feeder.h"
#include"Textures.h"
#include"Materials.h"
#include"Meshes.h"

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

uint32_t currentFrame = 0;

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
		engine->initWindow();
		engine->initVulkan();
		KeyInput::setupKeyInputs(engine->window);
		glfwSetScrollCallback(engine->window, camera.scrollCallback);
		createWebcamMaterial();
		createCanvas();
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
			drawFrame();
		}
		vkDeviceWaitIdle(engine->device);
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

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
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
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw runtime_error("failed to begin recording command buffer!");
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
