#include<iostream>
#include<stdexcept>
#include<cstdlib>
#include<cstring>
#include<vector>
#include<cstdint>
#include<opencv2/opencv.hpp>

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
#include"SurfaceConstructor.h"

#include"include/LoadButton.h"
#include"include/PauseButton.h"
#include"include/PlayButton.h"
#include"include/RenderedButton.h"
#include"include/SettingsButton.h"
#include"include/UnrenderedButton.h"
#include"include/WireframeButton.h"
#include"include/TestCheckboxButton.h"
#include"include/WebcamOffButton.h"
#include"include/WebcamOnButton.h"
#include"include/WebcamViewButton.h"
#include"include/D2NButton.h"
#include"include/DiffuseText.h"
#include"include/NormalText.h"
#include"include/PlusButton.h"
#include"include/SaveButton.h"
#include"include/TangentSpace.h"
#include"include/OSButton.h"
#include"include/OpenButton.h"

using namespace cv;
using namespace std;

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
		sConst->setupSurfaceConstructor();
		createCanvas();
		mainLoop();
		cleanup();
		Engine::destruct();
	}
private:
	Engine* engine = Engine::get();
	surfaceConstructor* sConst = surfaceConstructor::get();

	Camera camera;
	ImagePanel* diffuseView = nullptr;
	ImagePanel* normalView = nullptr;

	imageData* ub = new UNRENDEREDBUTTON;
	imageData* tcb = new TESTCHECKBOXBUTTON;

	vector<UIItem*> canvas{};

	vArrangement* ObjectButtons = nullptr;

	double mouseX = 0.0;
	double mouseY = 0.0;

	bool mouseDown = false;

	vector<StaticObject> staticObjects = {};
	map<string, int> ObjectMap = {};

	bool webcamObjectView = true;

	NormalGen mapGenerator;
	bool shouldRenderOSN = false;
	bool OSNavailable = false;

	bool shouldConvertOSN = false;
	bool TSNavailable = false;

	hArrangement *NormalButtons = nullptr;
	vArrangement* SurfacePanel = nullptr;

	Checkbox* diffuseTog = nullptr;
	Checkbox* normalTog = nullptr;

	Material* Diffuse = nullptr;
	Material* OSNormal = nullptr;

	bool lit = true;

	uint8_t viewIndex = 1;

	void createCanvas() {
		hArrangement *Renderbuttons = new hArrangement(0.0f, 0.0f, 0.2f, 0.05f, 0.01f);
		hArrangement *Videobuttons = new hArrangement(0.0f, 1.0f, 0.2f, 0.05f, 0.01f);

		SurfacePanel = new vArrangement(1.0f, 0.0f, 0.25f, 0.8f, 0.01f);

		std::function<void(UIItem*)> pipelinefunction = bind(&Application::setPipelineIndex, this, placeholders::_1);
		std::function<void(UIItem*)> lightingFunction = bind(&Application::toggleLighting, this, placeholders::_1);

		std::function<void(UIItem*)> enableWebcamFunct = bind(&Application::enableWebcam, this, placeholders::_1);
		std::function<void(UIItem*)> disableWebcamFunct = bind(&Application::disableWebcam, this, placeholders::_1);
		std::function<void(UIItem*)> toggleWebcamFunct = bind(&Application::toggleWebcam, this, placeholders::_1);
		std::function<void(UIItem*)> configureWebcamFunct = bind(&Application::calibrateWebcam, this, placeholders::_1);

		std::function<void(UIItem*)> loadObjectFunct = bind(&Application::buttonLoadStaticObject, this, placeholders::_1);

		std::function<void(UIItem*)> addNormalButton = bind(&Application::createNormalButtons, this, placeholders::_1);

		std::function<void(UIItem*)> toggleDiffuse = bind(&Application::toggleDiffuseCam, this, placeholders::_1);
		std::function<void(UIItem*)> loadDiffuse = bind(&Application::loadDiffuseImage, this, placeholders::_1);
		std::function<void(UIItem*)> saveWebcam = bind(&Application::saveDiffuseImage, this, placeholders::_1);

		imageData* lb = new LOADBUTTON;
		imageData* rb = new RENDEREDBUTTON;
		imageData* fb = new UNRENDEREDBUTTON;
		imageData* ub = new WEBCAMVIEWBUTTON;
		imageData* wb = new WIREFRAMEBUTTON;
		imageData* plb = new PLAYBUTTON;
		imageData* pb = new PAUSEBUTTON;
		imageData* sb = new SETTINGSBUTTON;
		imageData* diffuse = new DIFFUSETEXT;
		imageData* normal = new NORMALTEXT;
		imageData* webcamOn = new WEBCAMONBUTTON;
		imageData* webcamOff = new WEBCAMOFFBUTTON;
		imageData* OpenButton = new OPENBUTTON;
		imageData* SaveButton = new SAVEBUTTON;
		imageData* plusButton = new PLUSBUTTON;

		Button* diffuseTextPanel = new Button(0.0f, 0.0f, 1.0f, 1.0f, diffuse);
		diffuseTog = new Checkbox(0.0f, 0.0f, 1.0f, 1.0f, webcamOn, webcamOff);
		diffuseTog->Name = "ToggleDiffuseWebcam";
		diffuseTog->setClickFunction(toggleDiffuse);

		Button* diffLoad = new Button(0.0f, 0.0f, 1.0f, 1.0f, OpenButton);
		diffLoad->Name = "LoadDiffuse";
		diffLoad->setClickFunction(loadDiffuse);

		Button* diffSave = new Button(0.0f, 0.0f, 1.0f, 1.0f, SaveButton);
		diffSave->Name = "SaveDiffuse";
		diffSave->setClickFunction(saveWebcam);

		hArrangement* DiffuseButtons = new hArrangement(0.0f, 0.0f, 1.0f, 0.2f, 0.01f);

		Button* normalTextPanel = new Button(0.0f, 0.0f, 1.0f, 1.0f, normal);
		Button* normalPlus = new Button(0.0f, 0.0f, 1.0f, 1.0f, plusButton);
		normalPlus->Name = "add Normal";
		normalPlus->setClickFunction(addNormalButton);

		NormalButtons = new hArrangement(0.0f, 0.0f, 1.0f, 0.2f, 0.01f);

		spacer* testSpacer = new spacer;

		NormalButtons->addItem(normalTextPanel);
		NormalButtons->addItem(normalPlus);
		NormalButtons->addItem(testSpacer);

		DiffuseButtons->addItem(diffuseTextPanel);
		DiffuseButtons->addItem(diffuseTog);
		DiffuseButtons->addItem(new spacer);
		DiffuseButtons->addItem(diffLoad);
		DiffuseButtons->addItem(diffSave);
		
		Button *loadObjectButton = new Button(0.0f, 0.0f, 2.0f, 1.0f, lb);
		
		Button *litRenderingButton = new Button(0.0f, 0.0f, 1.0f, 1.0f, rb);
		Button *unlitRenderingButton = new Button(0.0f, 0.0f, 1.0f, 1.0f, ub);
		Button *wireframeRenderingButton = new Button(0.0f, 0.0f, 1.0f, 1.0f, wb);
		
		Checkbox* webcamToggle = new Checkbox(0.0f, 0.0f, 1.0f, 1.0f, plb, pb);
		webcamToggle->Name = "Webcam active toggle";
		webcamToggle->setClickFunction(toggleWebcamFunct);
		Button* settingsButton = new Button(0.0f, 0.0f, 1.0f, 1.0f, sb);

		unlitRenderingButton->Name = "WebcamMat";
		unlitRenderingButton->setClickFunction(pipelinefunction);
		
		litRenderingButton->Name = "SurfaceMat";
		litRenderingButton->setClickFunction(pipelinefunction);
		
		wireframeRenderingButton->Name = "Wireframe";
		wireframeRenderingButton->setClickFunction(pipelinefunction);
		
		settingsButton->Name = "ConfigureWebcam";
		settingsButton->setClickFunction(configureWebcamFunct);

		loadObjectButton->Name = "LoadObject";
		loadObjectButton->setClickFunction(loadObjectFunct);

		Renderbuttons->addItem(unlitRenderingButton);
		Renderbuttons->addItem(litRenderingButton);
		Renderbuttons->addItem(wireframeRenderingButton);

		Button* webcamImage = new Button(0.0f, 0.0f, 0.2f, 0.2f, webcamOn);

		Checkbox* litCheckbox = new Checkbox(0.0f, 0.0f, 1.0f, 1.0f, rb, fb);
		litCheckbox->Name = "Lighting toggle";
		litCheckbox->setClickFunction(lightingFunction);

		Videobuttons->addItem(webcamImage);
		Videobuttons->addItem(webcamToggle);
		Videobuttons->addItem(settingsButton);
		Videobuttons->addItem(litCheckbox);

		canvas.push_back(Videobuttons);

		vArrangement* buttons = new vArrangement(-1.0f, 1.0f, 0.15f, 0.15f, 0.0f);

		buttons->addItem(loadObjectButton);
		buttons->addItem(Renderbuttons);

		ObjectButtons = new vArrangement(-0.9f, -0.5f, 0.05f, 0.5f, 0.01f);

		canvas.push_back(ObjectButtons);

		canvas.push_back(buttons);

		cout << "Diffuse mat:" << sConst->currentDiffuse() << endl;
		diffuseView = new ImagePanel(0.0f, 0.0f, 1.0f, 0.71f, sConst->currentDiffuse(), true);
		SurfacePanel->addItem(DiffuseButtons);
		SurfacePanel->addItem(diffuseView);
		SurfacePanel->addItem(NormalButtons);
		SurfacePanel->addItem(new spacer);

		canvas.push_back(SurfacePanel);

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

	void toggleDiffuseCam(UIItem* owner) {
		sConst->toggleDiffWebcam();
		sConst->updateSurfaceMat();
		diffuseView->image->mat[0] = sConst->currentDiffuse();
	}

	void toggleNormalCam(UIItem* owner) {
		sConst->toggleNormWebcam();
		sConst->updateSurfaceMat();
		normalView->image->mat[0] = sConst->currentNormal();
	}

	void toggleNormalType(UIItem* owner) {
		sConst->toggleNormType();
		if (sConst->OSNormTex != nullptr && !sConst->TSmatching) {
			if (sConst->TSNormTex != nullptr) {
				sConst->TSNormTex->cleanup();
			}
			sConst->transitionToTS(&staticObjects[staticObjects.size() - 1].mesh);
			sConst->TSmatching = true;
		}
		normalView->image->mat[0] = sConst->currentNormal();
		sConst->updateSurfaceMat();
	}

	void loadNormalImage(UIItem* owner) {
		string fileName = winFile::OpenFileDialog();
		if (fileName != string("fail")) {
			imageTexture* loadedTexture = new imageTexture(fileName, VK_FORMAT_R8G8B8A8_UNORM);
			sConst->loadNormal(loadedTexture);
			sConst->normalIdx = 1 + sConst->normalType;
			normalView->image->mat[0] = sConst->currentNormal();
			normalTog->activestate = false;
			normalTog->image->matidx = 1;
		}
		sConst->updateSurfaceMat();
	}

	void saveNormalImage(UIItem* owner) {
		Mat saveNormal;
		if (sConst->normalIdx == 0) {
			saveNormal = webcamTexture::get()->webCam.webcamFrame;
		}
		else {
			if (sConst->normalType) {
				sConst->TSNormTex->getCVMat();
				saveNormal = sConst->TSNormTex->texMat;
			}
			else {
				sConst->OSNormTex->getCVMat();
				saveNormal = sConst->OSNormTex->texMat;
			}
		}
		string saveName = winFile::SaveFileDialog();
		if (saveName != string("fail")) {
			imwrite(saveName, saveNormal);
		}
	}

	void loadDiffuseImage(UIItem* owner) {
		string fileName = winFile::OpenFileDialog();
		if (fileName != string("fail")) {
			imageTexture* loadedTexture = new imageTexture(fileName, VK_FORMAT_R8G8B8A8_SRGB);
			sConst->loadDiffuse(loadedTexture);
			sConst->diffuseIdx = 1;
			diffuseView->image->mat[0] = sConst->currentDiffuse();
			diffuseTog->activestate = false;
			diffuseTog->image->matidx = 1;
		}
		sConst->updateSurfaceMat();
	}

	void saveDiffuseImage(UIItem* owner) {
		Mat saveDiffuse;
		if (sConst->diffuseIdx == 0) {
			saveDiffuse = webcamTexture::get()->webCam.webcamFrame;
		}
		else {
			sConst->diffTex->getCVMat();
			saveDiffuse = sConst->diffTex->texMat;
		}
		string saveName = winFile::SaveFileDialog();
		if (saveName != string("fail")) {
			imwrite(saveName, saveDiffuse);
		}
	}

	void contextConvertMap(UIItem* owner) {
		sConst->contextConvert();
		sConst->normalIdx = 1 + sConst->normalType;
		normalView->image->mat[0] = sConst->currentNormal();
		normalTog->activestate = false;
		normalTog->image->matidx = 1;
	}

	void createNormalButtons(UIItem* owner) {

		if (staticObjects.size() == 0) {
			return;
		}

		std::function<void(UIItem*)> toggleWebcam = bind(&Application::toggleNormalCam, this, placeholders::_1);
		std::function<void(UIItem*)> toggleType = bind(&Application::toggleNormalType, this, placeholders::_1);
		std::function<void(UIItem*)> saveNorm = bind(&Application::saveNormalImage, this, placeholders::_1);
		std::function<void(UIItem*)> loadNorm = bind(&Application::loadNormalImage, this, placeholders::_1);
		std::function<void(UIItem*)> convertImg = bind(&Application::contextConvertMap, this, placeholders::_1);

		sConst->generateOSMap(&staticObjects[staticObjects.size()-1].mesh);
		sConst->normalAvailable = true;
		webcamTexture::get()->changeFormat(VK_FORMAT_R8G8B8A8_UNORM);
		sConst->normalIdx = 1;
		sConst->updateSurfaceMat();

		SurfacePanel->removeItem(3);
		
		vector<UIImage*> images;
		NormalButtons->getImages(images);

		for (UIImage* image : images) {
			for (Material* mat : image->mat) {
				if (mat != sConst->webcamMaterial) {
					mat->cleanup();
				}
			}
			image->mesh.cleanup();
		}
		
		NormalButtons->Items.clear();

		imageData* normal = new NORMALTEXT;

		imageData* webcamOn = new WEBCAMONBUTTON;
		imageData* webcamOff = new WEBCAMOFFBUTTON;

		imageData* OpenButton = new OPENBUTTON;
		imageData* SaveButton = new SAVEBUTTON;

		imageData* osType = new OSBUTTON;
		imageData* tsType = new TANGENTSPACE;

		imageData* diffToNorm = new D2NBUTTON;

		Button* normalText = new Button(0.0f, 0.0f, 2.0f, 1.0f, normal);

		normalTog = new Checkbox(0.0f, 0.0f, 1.0f, 1.0f, webcamOn, webcamOff);
		normalTog->Name = "ToggleNormalWebcam";
		normalTog->activestate = false;
		normalTog->image->matidx = 1;
		normalTog->setClickFunction(toggleWebcam);

		Checkbox* mapTypeToggle = new Checkbox(0.0f, 0.0f, 1.0f, 1.0f, osType, tsType);
		mapTypeToggle->Name = "ToggleNormalType";
		mapTypeToggle->setClickFunction(toggleType);

		Button* copyLayout = new Button(0.0f, 0.0f, 1.0f, 1.0f, diffToNorm);
		copyLayout->Name = "copyDiffLayout";
		copyLayout->setClickFunction(convertImg);

		Button* normalLoad = new Button(0.0f, 0.0f, 1.0f, 1.0f, OpenButton);
		normalLoad->Name = "LoadNormal";
		normalLoad->setClickFunction(loadNorm);

		Button* normalSave = new Button(0.0f, 0.0f, 1.0f, 1.0f, SaveButton);
		normalSave->Name = "SaveNormal";
		normalSave->setClickFunction(saveNorm);

		NormalButtons->addItem(normalText);
		NormalButtons->addItem(normalTog);
		NormalButtons->addItem(mapTypeToggle);
		NormalButtons->addItem(copyLayout);
		NormalButtons->addItem(normalLoad);
		NormalButtons->addItem(normalSave);

		NormalButtons->arrangeItems();
		NormalButtons->updateDisplay();

		normalView = new ImagePanel(0.0f, 0.0f, 1.0f, 0.71f, sConst->currentNormal(), true);
		normalView->image->texHeight = static_cast<uint32_t>(0.71f * normalView->image->texWidth);
		normalView->updateDisplay();

		SurfacePanel->addItem(normalView);
		SurfacePanel->addItem(new spacer);
		SurfacePanel->arrangeItems();
	}

	void enableWebcam(UIItem* owner) {
		webcamTexture::get()->webCam.shouldUpdate = true;
	}

	void disableWebcam(UIItem* owner) {
		webcamTexture::get()->webCam.shouldUpdate = false;
	}

	void toggleWebcam(UIItem* owner) {
		webcamTexture::get()->webCam.shouldUpdate = owner->activestate;
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
		if (owner->Name == string("WebcamMat")) {
			viewIndex = 0;
		}
		else if (owner->Name == string("SurfaceMat")) {
			viewIndex = 1;
		}
		else if (owner->Name == string("Wireframe")) {
			viewIndex = 2;
		}
		updatePipelineIndex();
	}

	void toggleLighting(UIItem* owner) {
		lit = owner->activestate;
		updatePipelineIndex();
	}

	void updatePipelineIndex() {
		if ((viewIndex == 0 || viewIndex == 1) && lit) {
			engine->pipelineindex = 1;
		}
		else if (viewIndex != 2) {
			engine->pipelineindex = 0;
		}
		else if (viewIndex == 2) {
			engine->pipelineindex = 3;
		}
	}

	void loadStaticObject() {

		try {
			StaticObject newObject;
			newObject.mat = sConst->surfaceMat;

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
			staticObjects[i].mesh.cleanup();
		}

		for (uint32_t i = 0; i != canvas.size(); i++) {
			vector<UIImage*> images;
			canvas[i]->getImages(images);

			for (UIImage *image : images) {
				for (Material* mat : image->mat) {
					if (mat != sConst->webcamMaterial && !mat->cleaned) {
						mat->cleanup();
					}
				}
				image->mesh.cleanup();
			}
		}

		sConst->cleanup();
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

		ubo.UVdistort[0] = 2*diffuseView->extentx;
		ubo.UVdistort[1] = (diffuseView->posx) - diffuseView->extentx;
		ubo.UVdistort[2] = 2*diffuseView->extenty;
		ubo.UVdistort[3] = (diffuseView->posy) - diffuseView->extenty;

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
			commandBuffer = mapGenerator.drawOSMap(commandBuffer, &staticObjects[staticObjects.size() - 1].mesh);
			shouldRenderOSN = false;
			OSNavailable = true;
		}

		if (shouldConvertOSN) {
			commandBuffer = mapGenerator.convertOStoTS(commandBuffer, &staticObjects[staticObjects.size() - 1].mesh);
			shouldConvertOSN = false;
			TSNavailable = true;
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

				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, engine->diffusePipelineLayout, 0, 1, &staticObjects[i].mat->descriptorSets[currentFrame], 0, nullptr);

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

				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, engine->diffusePipelineLayout, 0, 1, &image->mat[image->matidx]->descriptorSets[currentFrame], 0, nullptr);

				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(image->mesh.indices.size()), 1, 0, 0, 0);
			}
		}

		if (viewIndex == 1 && lit) {
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *engine->GraphicsPipelines[engine->PipelineMap.at(sConst->renderPipeline)]);

			for (uint32_t i = 0; i != staticObjects.size(); i++) {
				if (staticObjects[i].isVisible) {
					VkBuffer vertexBuffers[] = { staticObjects[i].mesh.vertexBuffer };
					VkDeviceSize offsets[] = { 0 };

					vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

					vkCmdBindIndexBuffer(commandBuffer, staticObjects[i].mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

					if (sConst->normalAvailable) {
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, engine->diffNormPipelineLayout, 0, 1, &sConst->surfaceMat->descriptorSets[currentFrame], 0, nullptr);
					}
					else {
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, engine->diffusePipelineLayout, 0, 1, &sConst->surfaceMat->descriptorSets[currentFrame], 0, nullptr);
					}

					vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(staticObjects[i].mesh.indices.size()), 1, 0, 0, 0);
				}
			}
		}
		else {
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *engine->GraphicsPipelines[engine->pipelineindex]);

			for (uint32_t i = 0; i != staticObjects.size(); i++) {
				if (staticObjects[i].isVisible) {
					VkBuffer vertexBuffers[] = { staticObjects[i].mesh.vertexBuffer };
					VkDeviceSize offsets[] = { 0 };

					vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

					vkCmdBindIndexBuffer(commandBuffer, staticObjects[i].mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

					if (viewIndex == 1) {
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, engine->diffusePipelineLayout, 0, 1, &sConst->currentDiffuse()->descriptorSets[currentFrame], 0, nullptr);
					}
					else {
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, engine->diffusePipelineLayout, 0, 1, &sConst->webcamMaterial->descriptorSets[currentFrame], 0, nullptr);

					}

					vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(staticObjects[i].mesh.indices.size()), 1, 0, 0, 0);
				}
			}
		}

		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw runtime_error("failed to record command buffer!");
		}
	}
};


surfaceConstructor* surfaceConstructor::sinstance = nullptr;
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
