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
#include"Webcam_feeder.h"
#include"Textures.h"
#include"Materials.h"

#include"include/LoadButton.h"
#include"include/PauseButton.h"
#include"include/PlayButton.h"
#include"include/RenderedButton.h"
#include"include/SettingsButton.h"
#include"include/UnrenderedButton.h"
#include"include/WireframeButton.h"

using namespace cv;
using namespace std;

const string LOAD_BUTTON_PATH = "C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\LoadButton.png";
const string RENDERED_BUTTON_PATH = "C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\RenderedButton.png";
const string UNRENDERED_BUTTON_PATH = "C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\UnrenderedButton.png";
const string WIREFRAME_BUTTON_PATH = "C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\WireframeButton.png";
const string PLAY_BUTTON_PATH = "C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\PlayButton.png";
const string PAUSE_BUTTON_PATH = "C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\PauseButton.png";
const string SETTINGS_BUTTON_PATH = "C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\SettingsButton.png";

uint32_t currentFrame = 0;

vector<int> keybinds = { GLFW_KEY_L, GLFW_KEY_0, GLFW_KEY_1, GLFW_KEY_U, GLFW_KEY_I};

std::vector<KeyInput*> KeyInput::_instances;
KeyInput defaultKeyBinds(keybinds);

struct StaticObject {

	bool isVisible = false;

	vector<Vertex> vertices;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	vector<uint32_t> indices;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	Material* mat = nullptr;

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

	Button* Pause;
	Button* Play;
	Button* Settings;
	Button *LB;

	vector<UIItem*> canvas;

	vArrangement* ObjectButtons;

	double mouseX, mouseY;

	vector<StaticObject> staticObjects;
	map<string, int> ObjectMap;

	bool webcamObjectView = true;

	void createWebcamMaterial() {
		webcamTexture::get()->setup();
		webcamMaterial = new Material(webcamTexture::get());
	}

	void createCanvas() {
		hArrangement *Renderbuttons = new hArrangement(0.0f, 0.0f, 0.15f, 0.1f, 0.01f);
		hArrangement *Videobuttons = new hArrangement(0.775f, 0.4f, 0.2f, 0.1f, 0.01f);

		std::function<void(UIItem*)> pipelinefunction = bind(&Application::setPipelineIndex, this, placeholders::_1);

		imageData* lb = new LOADBUTTON;
		imageData* rb = new RENDEREDBUTTON;
		imageData* ub = new UNRENDEREDBUTTON;
		imageData* wb = new WIREFRAMEBUTTON;
		imageData* plb = new PLAYBUTTON;
		imageData* pb = new PAUSEBUTTON;
		imageData* sb = new SETTINGSBUTTON;

		Button *loadObjectButton = new Button(0.0f, 0.0f, 0.2f, 0.1f, lb, engine->windowWidth, engine->windowHeight);
		
		Button *litRenderingButton = new Button(0.0f, 0.0f, 0.2f, 0.2f, rb, engine->windowWidth, engine->windowHeight);
		Button *unlitRenderingButton = new Button(0.0f, 0.0f, 0.2f, 0.2f, ub, engine->windowWidth, engine->windowHeight);
		Button *wireframeRenderingButton = new Button(0.0f, 0.0f, 0.2f, 0.2f, wb, engine->windowWidth, engine->windowHeight);
		
		Button* playButton = new Button(0.0f, 0.0f, 0.2f, 0.2f, plb, engine->windowWidth, engine->windowHeight);
		Button* pauseButton = new Button(0.0f, 0.0f, 0.2f, 0.2f, pb, engine->windowWidth, engine->windowHeight);
		Button* settingsButton = new Button(0.0f, 0.0f, 0.2f, 0.2f, sb, engine->windowWidth, engine->windowHeight);

		unlitRenderingButton->Name = "FlatShading";
		unlitRenderingButton->setClickFunction(pipelinefunction);
		litRenderingButton->Name = "BFShading";
		litRenderingButton->setClickFunction(pipelinefunction);
		wireframeRenderingButton->Name = "Wireframe";
		wireframeRenderingButton->setClickFunction(pipelinefunction);

		Renderbuttons->addItem(unlitRenderingButton);
		Renderbuttons->addItem(litRenderingButton);
		Renderbuttons->addItem(wireframeRenderingButton);

		Videobuttons->addItem(playButton);
		Videobuttons->addItem(pauseButton);
		Videobuttons->addItem(settingsButton);
		
		Videobuttons->arrangeItems(engine->windowWidth, engine->windowHeight);

		canvas.push_back(Videobuttons);

		vArrangement* buttons = new vArrangement(-1.0f, 1.0f, 0.15f, 0.15f, 0.0f);

		buttons->addItem(loadObjectButton);
		buttons->addItem(Renderbuttons);
		buttons->arrangeItems(engine->windowWidth, engine->windowHeight);

		Play = playButton;
		Pause = pauseButton;
		Settings = settingsButton;
		LB = loadObjectButton;

		ObjectButtons = new vArrangement(-0.9f, -0.5f, 0.05f, 0.5f, 0.01f);

		canvas.push_back(ObjectButtons);

		for (Button *item : {loadObjectButton, litRenderingButton, unlitRenderingButton, wireframeRenderingButton, playButton, pauseButton, settingsButton}) {
			item->updateDisplay(engine->windowWidth, engine->windowHeight);
		}

		canvas.push_back(buttons);

		webcamView = new WebcamPanel(0.775f, 0.1f, 0.2f, 0.142f, engine->windowWidth, engine->windowHeight, webcamMaterial);
		webcamView->updateDisplay(engine->windowWidth, engine->windowHeight);
		webcamView->image->mat = webcamMaterial;

		canvas.push_back(webcamView);
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(engine->window)) {
			glfwPollEvents();
			glfwGetCursorPos(engine->window, &mouseX, &mouseY);
			webcamTexture::get()->updateWebcam();
			int state = glfwGetMouseButton(engine->window, GLFW_MOUSE_BUTTON_LEFT);
			if (state == GLFW_PRESS) {
				if (Play->isInArea(mouseX, mouseY)) {
					webcamTexture::get()->webCam.shouldUpdate = true;
				}
				if (Pause->isInArea(mouseX, mouseY)) {
					webcamTexture::get()->webCam.shouldUpdate = false;
				}
				if (Settings->isInArea(mouseX, mouseY)) {
					webcamTexture::get()->webCam.calibrateCornerFilter();
				}
				for (UIItem* item : canvas) {
					vector<UIItem*> scs;
					item->getSubclasses(scs);
					for (UIItem* sitem : scs) {
						sitem->checkForEvent(mouseX, mouseY, state);
					}
				}
				if (LB->isInArea(mouseX, mouseY) && state == GLFW_PRESS) {
					loadStaticObject();
				}
			}
			drawFrame();
		}
		vkDeviceWaitIdle(engine->device);
	}

	void setObjectVisibilities(UIItem* owner) {
		for (int i = 0; i != staticObjects.size(); i++) {
			staticObjects[i].isVisible = false;
		}
		staticObjects[ObjectMap.at(owner->Name)].isVisible = true;
	}

	void setPipelineIndex(UIItem* owner) {
		engine->pipelineindex = engine->PipelineMap.at(owner->Name);
	}

	void loadStaticObject() {
		StaticObject newObject;
		
		bool res = newObject.loadModel();

		if (res) {
			createVertexBuffer(newObject);
			createIndexBuffer(&newObject);
			newObject.mat = webcamMaterial;

			std::function<void(UIItem*)> testfunction = bind(&Application::setObjectVisibilities, this, placeholders::_1);

			imageData* ub = new UNRENDEREDBUTTON;

			Button* objectButton = new Button(0.0f, 0.0f, 0.15f, 0.15f, ub, engine->windowWidth, engine->windowHeight);

			objectButton->updateDisplay(engine->windowWidth, engine->windowHeight);
			objectButton->setClickFunction(testfunction);

			objectButton->Name = "Object button " + std::to_string(ObjectButtons->Items.size());

			ObjectMap.insert({ objectButton->Name, staticObjects.size() });

			ObjectButtons->addItem(objectButton);
			ObjectButtons->arrangeItems(engine->windowWidth, engine->windowHeight);

			ObjectButtons->updateDisplay(engine->windowWidth, engine->windowHeight);

			for (int i = 0; i != staticObjects.size(); i++) {
				staticObjects[i].isVisible = false;
			}
			newObject.isVisible = true;

			staticObjects.push_back(newObject);
		}
	}

	void cleanup() {

		for (uint32_t i = 0; i != staticObjects.size(); i++) {
			if (staticObjects[i].mat != webcamMaterial) {
				staticObjects[i].mat->cleanup();
			}

			vkDestroyBuffer(engine->device, staticObjects[i].indexBuffer, nullptr);
			vkFreeMemory(engine->device, staticObjects[i].indexBufferMemory, nullptr);

			vkDestroyBuffer(engine->device, staticObjects[i].vertexBuffer, nullptr);
			vkFreeMemory(engine->device, staticObjects[i].vertexBufferMemory, nullptr);
		}

		for (uint32_t i = 0; i != canvas.size(); i++) {
			vector<UIImage*> images;
			canvas[i]->getImages(images);

			for (UIImage *image : images) {
				if (image->mat != webcamMaterial) {
					image->mat->cleanup();
				}

				vkDestroyBuffer(engine->device, image->indexBuffer, nullptr);
				vkFreeMemory(engine->device, image->indexBufferMemory, nullptr);

				vkDestroyBuffer(engine->device, image->vertexBuffer, nullptr);
				vkFreeMemory(engine->device, image->vertexBufferMemory, nullptr);
			}
		}

		webcamMaterial->cleanup();
		engine->cleanup();
	}

	VkSampleCountFlagBits getMaxUseableSampleCount() {
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(engine->physicalDevice, &physicalDeviceProperties);

		VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;

		if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
		if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
		if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
		if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
		if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
		if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

		return VK_SAMPLE_COUNT_1_BIT;
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
				canvas[i]->updateDisplay(engine->windowWidth, engine->windowHeight);
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

	void createVertexBuffer(auto& object) {

		VkDeviceSize bufferSize = sizeof(object.vertices[0]) * object.vertices.size();
		
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		engine->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(engine->device, stagingBufferMemory, 0, bufferSize, 0, &data);
			memcpy(data, object.vertices.data(), (size_t) bufferSize);
		vkUnmapMemory(engine->device, stagingBufferMemory);

		engine->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, object.vertexBuffer, object.vertexBufferMemory);

		engine->copyBuffer(stagingBuffer, object.vertexBuffer, bufferSize);

		vkDestroyBuffer(engine->device, stagingBuffer, nullptr);
		vkFreeMemory(engine->device, stagingBufferMemory, nullptr);
	}

	void createIndexBuffer(auto *object) {

		VkDeviceSize bufferSize = sizeof(object->indices[0]) * object->indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		engine->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(engine->device, stagingBufferMemory, 0, bufferSize, 0, &data);
			memcpy(data, object->indices.data(), (size_t)bufferSize);
		vkUnmapMemory(engine->device, stagingBufferMemory);

		engine->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, object->indexBuffer, object->indexBufferMemory);

		engine->copyBuffer(stagingBuffer, object->indexBuffer, bufferSize);

		vkDestroyBuffer(engine->device, stagingBuffer, nullptr);
		vkFreeMemory(engine->device, stagingBufferMemory, nullptr);
	}

	void OldcreateDescriptorPool(auto *object) {
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

		if (vkCreateDescriptorPool(engine->device, &poolInfo, nullptr, &object->descriptorPool) != VK_SUCCESS) {
			throw runtime_error("failed to create descriptor pool!");
		}
	}


	void OldcreateDescriptorSets(auto *object) {
		vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, engine->descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = object->descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		object->descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		
		if (vkAllocateDescriptorSets(engine->device, &allocInfo, object->descriptorSets.data()) != VK_SUCCESS) {
			throw runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = engine->uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			if (object->texture != nullptr) {
				imageInfo.imageView = object->texture->textureImageView;
			}
			else if (object -> wtexture != nullptr) {
				imageInfo.imageView = object->wtexture->textureImageView;
			}
			imageInfo.sampler = engine->textureSampler;

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

			vkUpdateDescriptorSets(engine->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
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
				VkBuffer vertexBuffers[] = { staticObjects[i].vertexBuffer };
				VkDeviceSize offsets[] = { 0 };

				vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

				vkCmdBindIndexBuffer(commandBuffer, staticObjects[i].indexBuffer, 0, VK_INDEX_TYPE_UINT32);

				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, engine->pipelineLayout, 0, 1, &staticObjects[i].mat->descriptorSets[currentFrame], 0, nullptr);

				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(staticObjects[i].indices.size()), 1, 0, 0, 0);
			}
		}
		
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *engine->GraphicsPipelines[engine->PipelineMap.at("UIShading")]);

		for (uint32_t i = 0; i != canvas.size(); i++) {
			vector<UIImage*> images;
			canvas[i]->getImages(images);

			for (UIImage *image : images) {
				VkBuffer vertexBuffers[] = { image->vertexBuffer };
				VkDeviceSize offsets[] = { 0 };

				vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

				vkCmdBindIndexBuffer(commandBuffer, image->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, engine->pipelineLayout, 0, 1, &image->mat->descriptorSets[currentFrame], 0, nullptr);

				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(image->indices.size()), 1, 0, 0, 0);
			}
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *engine->GraphicsPipelines[engine->pipelineindex]);
		
		for (uint32_t i = 0; i != staticObjects.size(); i++) {
			if (staticObjects[i].isVisible) {
				VkBuffer vertexBuffers[] = { staticObjects[i].vertexBuffer };
				VkDeviceSize offsets[] = { 0 };

				vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

				vkCmdBindIndexBuffer(commandBuffer, staticObjects[i].indexBuffer, 0, VK_INDEX_TYPE_UINT32);

				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, engine->pipelineLayout, 0, 1, &staticObjects[i].mat->descriptorSets[currentFrame], 0, nullptr);

				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(staticObjects[i].indices.size()), 1, 0, 0, 0);
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
