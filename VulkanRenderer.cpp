#include"Bobject_Engine.h"
#include"InputManager.h"
#include"WindowsFileManager.h"
#include"CameraController.h"
#include"UIelements.h"
#include"Webcam_feeder.h"
#include"Textures.h"
#include"Materials.h"
#include"Meshes.h"
#include"SurfaceConstructor.h"
#include"StudioSession.h"
#include"Tomography.h"
#include"LoadLists.h"

#include"include/BakedImages.h"

using namespace cv;
using namespace std;

vector<int> keybinds = { GLFW_KEY_L, GLFW_KEY_0, GLFW_KEY_1, GLFW_KEY_U, GLFW_KEY_I };

std::vector<KeyInput*> KeyInput::_instances;
KeyInput defaultKeyBinds(keybinds);

class SaveMenu : public Widget {
public:
	SaveMenu(LoadList* assets) {
		loadList = assets;
	}

	void setup(std::function<void(UIItem*)> loadSessionFunc, std::function<void(UIItem*)> newSessionFunc) {
		if (isSetup) {
			return;
		}
		hArrangement* SessionButtons = new hArrangement(1.0f, 1.0f, 0.15f, 0.05f, 0.01f);

		std::function<void(UIItem*)> saveSessionFunc = bind(&SaveMenu::save, this, placeholders::_1);

		imageData OpenButton = OPENBUTTON;
		Material* openMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&OpenButton), "OpenBtnTex")), "OpenBtnMat");
		
		imageData SaveButton = SAVEBUTTON;
		Material* saveMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&SaveButton), "SaveBtnTex")), "SaveBtnMat");

		imageData plusButton = PLUSBUTTON;
		Material* plusMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&plusButton), "PlusBtnTex")), "PlusBtnMat");

		SessionButtons->addItem(getPtr(new Button(plusMat, newSessionFunc)));
		SessionButtons->addItem(getPtr(new Button(openMat, loadSessionFunc)));
		SessionButtons->addItem(getPtr(new Button(saveMat, saveSessionFunc)));

		SessionButtons->arrangeItems();

		canvas.push_back(getPtr(SessionButtons));

		isSetup = true;
	}
private:
	void save(UIItem* owner) {
		string saveLocation;
		saveLocation = winFile::SaveFileDialog();
		if (saveLocation == "fail") {
			return;
		}
		session::get()->saveStudio(saveLocation);
	}
};

class RenderMenu : public Widget {
public:
	RenderMenu(LoadList* assets) {
		loadList = assets;
	}

	void setup(std::function<void(UIItem*)> loadObjectFunct, std::function<void(UIItem*)> pipelinefunction){
		if (isSetup) {
			return;
		}

		imageData rb = RENDEREDBUTTON;
		Material* renderedMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&rb), "RenderBtnTex")), "RenderBtnMat");

		imageData ub = WEBCAMVIEWBUTTON;
		Material* webcamViewMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&ub), "WebcamBtnTex")), "WebcamBtnMat");

		imageData wb = WIREFRAMEBUTTON;
		Material* wireframeViewMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&wb), "WireframeBtnTex")), "WireframeBtnMat");

		imageData lb = LOADBUTTON;
		Material* LoadBtnMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&lb), "LoadBtnTex")), "LoadBtnMat");

		hArrangement* Renderbuttons = new hArrangement(0.0f, 0.0f, 0.2f, 0.05f, 0.01f);

		Button* litRenderingButton = new Button(renderedMat);
		Button* unlitRenderingButton = new Button(webcamViewMat);
		Button* wireframeRenderingButton = new Button(wireframeViewMat);

		unlitRenderingButton->Name = "WebcamMat";
		unlitRenderingButton->setClickFunction(pipelinefunction);

		litRenderingButton->Name = "SurfaceMat";
		litRenderingButton->setClickFunction(pipelinefunction);

		wireframeRenderingButton->Name = "Wireframe";
		wireframeRenderingButton->setClickFunction(pipelinefunction);

		Renderbuttons->addItem(getPtr(unlitRenderingButton));
		Renderbuttons->addItem(getPtr(litRenderingButton));
		Renderbuttons->addItem(getPtr(wireframeRenderingButton));

		vArrangement* buttons = new vArrangement(-1.0f, 1.0f, 0.15f, 0.15f, 0.0f);

		buttons->addItem(getPtr(new Button(LoadBtnMat, loadObjectFunct)));
		buttons->addItem(getPtr(Renderbuttons));

		buttons->arrangeItems();
		
		canvas.push_back(getPtr(buttons));

		isSetup = true;
	}
};

class ObjectMenu : public Widget {
public:
	ObjectMenu(LoadList* assets) {
		loadList = assets;
	}

	void setup() {
		if (isSetup) {
			return;
		}

		imageData ub = UNRENDEREDBUTTON;
		invisibleMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&ub), "UnrenderedBtnTex")), "UnrenderedBtnMat");

		imageData tcb = TESTCHECKBOXBUTTON;
		visibleMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&tcb), "TestCheckBtnTex")), "TestCheckBtnMat");

		canvas.push_back(getPtr(new vArrangement(-0.9f, -0.5f, 0.05f, 0.5f, 0.01f)));

		ObjectButtons = canvas[0];
		
		isSetup = true;
	}

	void addObject(std::function<void(UIItem*)> toggleFunction) {

		Checkbox* objectButton = new Checkbox(visibleMat, invisibleMat, toggleFunction);
		objectButton->Name = "Object button " + std::to_string(ObjectButtons->Items.size());

		ObjectMap.insert({ objectButton->Name, ObjectButtons->Items.size() });

		ObjectButtons->addItem(objectButton);
		ObjectButtons->arrangeItems();
	}

	void clearObjects() {
		for (UIItem* item : ObjectButtons->Items) {
			item->image->cleanup();
		}
		ObjectButtons->Items.clear();
		ObjectMap.clear();
		ObjectButtons->arrangeItems();
	}

	map<string, int> ObjectMap = {};
private:
	UIItem* ObjectButtons = nullptr;

	Material* visibleMat = nullptr;
	Material* invisibleMat = nullptr;
};

class WebcamMenu : public Widget {
public:
	WebcamMenu(LoadList* assets) {
		loadList = assets;
	}

	void setup(std::function<void(UIItem*)> lightingFunction) {
		if (isSetup) {
			return;
		}
		imageData rb = RENDEREDBUTTON;
		Material* renderedMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&rb), "RenderBtnTex")), "RenderBtnMat");
		
		imageData fb = UNRENDEREDBUTTON;
		Material* unrenderedMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&fb), "UnrenderedBtnTex")), "UnrenderedBtnMat");

		imageData plb = PLAYBUTTON;
		Material* playMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&plb), "PlayBtnTex")), "PlayBtnMat");

		imageData pb = PAUSEBUTTON;
		Material* pauseMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&pb), "PauseBtnTex")), "PauseBtnMat");

		imageData sb = SETTINGSBUTTON;
		Material* settingsMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&sb), "SettingsBtnTex")), "SettingsBtnMat");

		imageData webcamOn = WEBCAMONBUTTON;
		Material* webcamMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&webcamOn), "WebcamOnBtnTex")), "WebcamOnBtnMat");

		std::function<void(UIItem*)> toggleWebcamFunct = bind(&WebcamMenu::toggleWebcam, this, placeholders::_1);
		std::function<void(UIItem*)> configureWebcamFunct = bind(&WebcamMenu::calibrateWebcam, this, placeholders::_1);

		hArrangement* Videobuttons = new hArrangement(0.0f, 1.0f, 0.2f, 0.05f, 0.01f);

		Videobuttons->addItem(getPtr(new Button(webcamMat)));
		Videobuttons->addItem(getPtr(new Checkbox(playMat, pauseMat, toggleWebcamFunct)));
		Videobuttons->addItem(getPtr(new Button(settingsMat, configureWebcamFunct)));
		Videobuttons->addItem(getPtr(new Checkbox(renderedMat, unrenderedMat, lightingFunction)));

		Videobuttons->arrangeItems();

		canvas.push_back(getPtr(Videobuttons));

		isSetup = true;
	}
private:
	void toggleWebcam(UIItem* owner) {
		if (webcamTexture::get()->webCam != nullptr) {
			webcamTexture::get()->webCam->shouldUpdate = owner->activestate;
		}
	}

	void calibrateWebcam(UIItem* owner) {
		if (webcamTexture::get()->webCam != nullptr) {
			webcamTexture::get()->webCam->calibrateCornerFilter();
		}
	}
};

class TomographyMenu : public Widget {
public:
	TomographyMenu(LoadList* assets) {
		loadList = assets;
	}

	void setup(surfaceConstructor* sConst) {
		if (isSetup) {
			return;
		}
		surface = sConst;
		imageData OpenButton = OPENBUTTON;
		Material* openMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&OpenButton), "OpenBtnTex")), "OpenBtnMat");
		
		imageData normal = NORMALTEXT;
		Material* normalMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&normal), "NormalBtnTex")), "NormalBtnMat");

		std::function<void(UIItem*)> tomogLoadTop = bind(&TomographyMenu::loadTop, this, placeholders::_1);
		std::function<void(UIItem*)> tomogLoadBottom = bind(&TomographyMenu::loadBottom, this, placeholders::_1);
		std::function<void(UIItem*)> tomogLoadLeft = bind(&TomographyMenu::loadLeft, this, placeholders::_1);
		std::function<void(UIItem*)> tomogLoadRight = bind(&TomographyMenu::loadRight, this, placeholders::_1);
		std::function<void(UIItem*)> computeNormal = bind(&TomographyMenu::performTomog, this, placeholders::_1);

		vArrangement* tomogButtons = new vArrangement(0.0f, 0.0f, 0.1f, 0.4f, 0.01f);

		tomogButtons->addItem(getPtr(new Button(openMat, tomogLoadTop)));
		tomogButtons->addItem(getPtr(new Button(openMat, tomogLoadBottom)));
		tomogButtons->addItem(getPtr(new Button(openMat, tomogLoadLeft)));
		tomogButtons->addItem(getPtr(new Button(openMat, tomogLoadRight)));
		tomogButtons->addItem(getPtr(new Button(normalMat, computeNormal)));

		tomogButtons->updateDisplay();

		canvas.push_back(getPtr(tomogButtons));

		isSetup = true;
	}
private:
	Tomographer tomographer;

	surfaceConstructor* surface = nullptr;

	void loadTop(UIItem* owner) {
		string fileName = winFile::OpenFileDialog();
		if (fileName != string("fail")) {
			tomographer.add_image(fileName, 90.0, 50.0);
		}
	}

	void loadBottom(UIItem* owner) {
		string fileName = winFile::OpenFileDialog();
		if (fileName != string("fail")) {
			tomographer.add_image(fileName, 270.0, 50.0);
		}
	}

	void loadLeft(UIItem* owner) {
		string fileName = winFile::OpenFileDialog();
		if (fileName != string("fail")) {
			tomographer.add_image(fileName, 180.0, 50.0);
		}
	}

	void loadRight(UIItem* owner) {
		string fileName = winFile::OpenFileDialog();
		if (fileName != string("fail")) {
			tomographer.add_image(fileName, 0.0, 50.0);
		}
	}

	void performTomog(UIItem* owner) {
		surface->diffTex->getCVMat();
		tomographer.outdims = Size(surface->diffTex->texMat.cols, surface->diffTex->texMat.rows);
		tomographer.alignTemplate = &surface->diffTex->texMat;
		tomographer.alignRequired = true;
		tomographer.calculate_normal();
		string saveName = winFile::SaveFileDialog();
		if (saveName != string("fail")) {
			imwrite(saveName, tomographer.computedNormal);
		}
		tomographer.clearData();
	}
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
		if (sConst->webTex->webCam != nullptr) {
			sConst->webTex->webCam->loadFilter();
		}
		mainLoop();
		cleanup();
		surfaceConstructor::destruct();
		webcamTexture::destruct();
		Engine::destruct();
	}
private:
	LoadList UIElements{};
	LoadList ObjectElements{};

	Engine* engine = Engine::get();
	surfaceConstructor* sConst = surfaceConstructor::get();

	Camera camera;
	Tomographer tomographer;

	TomographyMenu tomogUI = TomographyMenu(&UIElements);
	SaveMenu saveMenu = SaveMenu(&UIElements);
	WebcamMenu webcamMenu = WebcamMenu(&UIElements);
	RenderMenu renderMenu = RenderMenu(&UIElements);
	ObjectMenu objectMenu = ObjectMenu(&UIElements);
	SurfaceMenu surfaceMenu = SurfaceMenu(&UIElements);

	vector<Widget*> widgets;

	double mouseX = 0.0;
	double mouseY = 0.0;

	bool mouseDown = false;

	vector<StaticObject> staticObjects = {};
	map<string, int> ObjectMap = {};

	bool lit = true;

	uint8_t viewIndex = 1;

	void newSession(UIItem* owner) {
		// Remove all meshes
		for (StaticObject obj : staticObjects) {
			obj.mesh->cleanup();
		}
		staticObjects.clear();

		objectMenu.clearObjects();

		// Clear session data
		session::get()->clearStudio();
		
		// Clear all studio material data
		surfaceMenu.removeNormalMenu(owner);
		sConst->clearSurface();
		surfaceMenu.resetDiffuseTog(false);
		if (sConst->normalAvailable) {
			surfaceMenu.resetNormalTog(false);
		}
		sConst->normalAvailable = false;
		surfaceMenu.setDiffuse(sConst->currentDiffuse().get());

		sConst->renderPipeline = "BFShading";
		sConst->updateSurfaceMat();
	}
	
	void loadSave(UIItem* owner) {

		string saveLocation;
		saveLocation = winFile::OpenFileDialog();
		if (saveLocation == "fail") {
			return;
		}
		newSession(owner);
		session::get()->loadStudio(saveLocation);
		for (string path : session::get()->currentStudio.modelPaths) {
			StaticObject newObject(path);
			newObject.mat = &sConst->surfaceMat;

			std::function<void(UIItem*)> testfunction = bind(&Application::setObjectVisibility, this, placeholders::_1);

			objectMenu.addObject(testfunction);

			newObject.isVisible = true;

			staticObjects.push_back(newObject);
		}
		if (session::get()->currentStudio.diffusePath != "None") {
			imageTexture* loadedTexture = new imageTexture(session::get()->currentStudio.diffusePath, VK_FORMAT_R8G8B8A8_SRGB);
			sConst->loadDiffuse(loadedTexture);
			sConst->diffuseIdx = 1;
			surfaceMenu.setDiffuse(sConst->currentDiffuse().get());
			surfaceMenu.resetDiffuseTog(true);
		}
		if (session::get()->currentStudio.OSPath != "None") {
			imageTexture* loadedTexture = new imageTexture(session::get()->currentStudio.OSPath, VK_FORMAT_R8G8B8A8_UNORM);
			if (!sConst->normalAvailable) {
				surfaceMenu.createNormalMenu(new UIItem);
			}
			sConst->normalType = 0;
			sConst->loadNormal(loadedTexture);
			surfaceMenu.setNormal(sConst->currentNormal().get());
			surfaceMenu.resetNormalTog(true);
			surfaceMenu.toggleNormalState(true);
		}
		if (session::get()->currentStudio.TSPath != "None") {
			imageTexture* loadedTexture = new imageTexture(session::get()->currentStudio.TSPath, VK_FORMAT_R8G8B8A8_UNORM);
			if (!sConst->normalAvailable) {
				surfaceMenu.createNormalMenu(new UIItem);
			}
			sConst->normalType = 1;
			sConst->loadNormal(loadedTexture);
			sConst->TSmatching = true;
			surfaceMenu.setNormal(sConst->currentNormal().get());
			surfaceMenu.resetNormalTog(true);
			surfaceMenu.toggleNormalState(false);
		}
		sConst->updateSurfaceMat();
		webcamTexture::get()->webCam->saveFilter();
	}
	
	void createCanvas() {

		std::function<void(UIItem*)> pipelinefunction = bind(&Application::setPipelineIndex, this, placeholders::_1);
		std::function<void(UIItem*)> lightingFunction = bind(&Application::toggleLighting, this, placeholders::_1);
		std::function<void(UIItem*)> loadObjectFunct = bind(&Application::buttonLoadStaticObject, this, placeholders::_1);
		std::function<void(UIItem*)> loadSessionFunc = bind(&Application::loadSave, this, placeholders::_1);
		std::function<void(UIItem*)> newSessionFunc = bind(&Application::newSession, this, placeholders::_1);

		saveMenu.setup(loadSessionFunc, newSessionFunc);
		widgets.push_back(&saveMenu);

		webcamMenu.setup(lightingFunction);
		widgets.push_back(&webcamMenu);

		renderMenu.setup(loadObjectFunct, pipelinefunction);
		widgets.push_back(&renderMenu);

		objectMenu.setup();
		widgets.push_back(&objectMenu);

		surfaceMenu.setup(sConst, &staticObjects);
		widgets.push_back(&surfaceMenu);

	}

	void mainLoop() {
		while (!glfwWindowShouldClose(engine->window)) {
			glfwPollEvents();
			glfwGetCursorPos(engine->window, &mouseX, &mouseY);
			webcamTexture::get()->updateWebcam();
			int state = glfwGetMouseButton(engine->window, GLFW_MOUSE_BUTTON_LEFT);
			if (defaultKeyBinds.getIsKeyDown(GLFW_KEY_L)) {
				tomogUI.setup(sConst);
				widgets.push_back(&tomogUI);
			}
			if (state == GLFW_PRESS) {
				mouseDown = true;
			}
			else if (~state && mouseDown){
				for (size_t i = 0; i != widgets.size(); i++) {
					widgets[i]->checkForEvent(mouseX, mouseY, GLFW_PRESS);
				}
				//for (UIItem* item : canvas) {
				//	vector<UIItem*> scs;
				//	item->getSubclasses(scs);
				//	for (UIItem* sitem : scs) {
				//		sitem->checkForEvent(mouseX, mouseY, GLFW_PRESS);
				//	}
				//}
				mouseDown = false;
			}
			drawFrame();
		}
		vkDeviceWaitIdle(engine->device);
	}

	void buttonLoadStaticObject(UIItem* owner) {
		loadStaticObject();
	}

	void setObjectVisibility(UIItem* owner) {
		staticObjects[objectMenu.ObjectMap.at(owner->Name)].isVisible = owner->activestate;
	}

	void setPipelineIndex(UIItem* owner) {
		if (owner->Name == string("WebcamMat")) {
			viewIndex = 0;
			surfaceMenu.hide();
		}
		else if (owner->Name == string("SurfaceMat")) {
			viewIndex = 1;
			surfaceMenu.show();
		}
		else if (owner->Name == string("Wireframe")) {
			viewIndex = 2;
			surfaceMenu.hide();
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
			string modelPath;
			try {
				modelPath = winFile::OpenFileDialog();
			}
			catch (...) {
				return;
			}
			StaticObject newObject(modelPath);
			newObject.mat = &sConst->surfaceMat;

			std::function<void(UIItem*)> testfunction = bind(&Application::setObjectVisibility, this, placeholders::_1);

			objectMenu.addObject(testfunction);

			newObject.isVisible = true;

			staticObjects.push_back(newObject);
			session::get()->currentStudio.modelPaths.push_back(modelPath);
		}
		catch (...) {
			return;
		}
	}

	void cleanup() {
		for (uint32_t i = 0; i != staticObjects.size(); i++) {
			staticObjects[i].mesh->cleanup();
		}

		UIElements.empty();
		ObjectElements.empty();

		for (size_t i = 0; i != widgets.size(); i++) {
			widgets[i]->cleanup();
			widgets[i]->~Widget();
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

			for (size_t i = 0; i != widgets.size(); i++) {
				widgets[i]->update();
			}
			
			//for (size_t i = 0; i != canvas.size(); i++) {
			//	canvas[i]->updateDisplay();
			//}

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

		ubo.UVdistort[0] = 2*surfaceMenu.diffuseView->extentx;
		ubo.UVdistort[1] = (surfaceMenu.diffuseView->posx) - surfaceMenu.diffuseView->extentx;
		ubo.UVdistort[2] = 2*surfaceMenu.diffuseView->extenty;
		ubo.UVdistort[3] = (surfaceMenu.diffuseView->posy) - surfaceMenu.diffuseView->extenty;

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
		
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *engine->GraphicsPipelines[engine->PipelineMap.at("UIShading")]);

		for (size_t i = 0; i != widgets.size(); i++) {
			widgets[i]->draw(commandBuffer, currentFrame);
		}

		if (viewIndex == 1 && lit) {
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *engine->GraphicsPipelines[engine->PipelineMap.at(sConst->renderPipeline)]);

			for (uint32_t i = 0; i != staticObjects.size(); i++) {
				if (staticObjects[i].isVisible) {
					VkBuffer vertexBuffers[] = { staticObjects[i].mesh->vertexBuffer };
					VkDeviceSize offsets[] = { 0 };

					vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

					vkCmdBindIndexBuffer(commandBuffer, staticObjects[i].mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

					if (sConst->normalAvailable) {
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, engine->diffNormPipelineLayout, 0, 1, &sConst->surfaceMat.descriptorSets[currentFrame], 0, nullptr);
					}
					else {
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, engine->diffusePipelineLayout, 0, 1, &sConst->surfaceMat.descriptorSets[currentFrame], 0, nullptr);
					}

					vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(staticObjects[i].mesh->indices.size()), 1, 0, 0, 0);
				}
			}
		}
		else {
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *engine->GraphicsPipelines[engine->pipelineindex]);

			for (uint32_t i = 0; i != staticObjects.size(); i++) {
				if (staticObjects[i].isVisible) {
					VkBuffer vertexBuffers[] = { staticObjects[i].mesh->vertexBuffer };
					VkDeviceSize offsets[] = { 0 };

					vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

					vkCmdBindIndexBuffer(commandBuffer, staticObjects[i].mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

					if (viewIndex == 1) {
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, engine->diffusePipelineLayout, 0, 1, &sConst->currentDiffuse()->descriptorSets[currentFrame], 0, nullptr);
					}
					else {
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, engine->diffusePipelineLayout, 0, 1, &sConst->webcamPtr->descriptorSets[currentFrame], 0, nullptr);

					}

					vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(staticObjects[i].mesh->indices.size()), 1, 0, 0, 0);
				}
			}
		}

		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw runtime_error("failed to record command buffer!");
		}
	}
};

session* session::sessionInstance = nullptr;
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