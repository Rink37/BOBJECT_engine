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
#include"Remapper.h"

#include<chrono>

#include"include/BakedImages.h"

using namespace cv;
using namespace std;

std::vector<KeyManager*> KeyManager::_instances;
KeyManager keyBinds;

std::vector<MouseManager*> MouseManager::_instances;
MouseManager mouseManager;

class WebcamSettings : public Widget {
public:
	WebcamSettings(LoadList* assets) {
		loadList = assets;
	}

	void setup(std::function<void(UIItem*)> finishCallback, std::function<void()> reloadCallback) {
		if (isSetup) {
			return;
		}

		reload = reloadCallback;

		std::function<void(UIItem*)> webcamCalib = bind(&WebcamSettings::calibrateWebcam, this, placeholders::_1);
		
		std::function<void(UIItem*)> addRot = bind(&WebcamSettings::addRotation, this, placeholders::_1);
		std::function<void(UIItem*)> subtractRot = bind(&WebcamSettings::subtractRotation, this, placeholders::_1);

		std::function<void(UIItem*)> idUp = bind(&WebcamSettings::indexUp, this, placeholders::_1);
		std::function<void(UIItem*)> idDown = bind(&WebcamSettings::indexDown, this, placeholders::_1);
		
		Arrangement* mainArrangement = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.4f, 0.6f, 0.01f, ARRANGE_START);

		Arrangement* endButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.05f, 0.01f);
		Arrangement* idButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.05f, 0.01f);
		Arrangement* rotationButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.05f, 0.01f);

		webcamView = new ImagePanel(new Material(webcamTexture::get()), true);
		
		imageData finishBtnImage = FINISHBUTTON;
		Material* finishmat = newMaterial(&finishBtnImage, "FinishBtn");

		imageData sb = SETTINGSBUTTON;
		Material* settingsMat = newMaterial(&sb, "SettingsBtn");

		idButtons->addItem(getPtr(new Button(settingsMat, idDown)));
		idButtons->addItem(getPtr(new spacer));
		idButtons->addItem(getPtr(new Button(settingsMat, idUp)));
		
		rotationButtons->addItem(getPtr(new Button(settingsMat, subtractRot)));
		rotationButtons->addItem(getPtr(new spacer));
		rotationButtons->addItem(getPtr(new Button(settingsMat, addRot)));

		endButtons->addItem(getPtr(new Button(settingsMat, webcamCalib)));
		endButtons->addItem(getPtr(new spacer));
		endButtons->addItem(getPtr(new Button(finishmat, finishCallback)));

		imageData tcb = TESTCHECKBOXBUTTON;
		Material* visibleMat = newMaterial(&tcb, "TestCheckBtn");

		ratioSlider = new Slider(ORIENT_HORIZONTAL, visibleMat, 0.0f, 0.0f, 1.0f, 0.1f);
		ratioSlider->setFloatCallback(bind(&WebcamSettings::updateAspectRatio, this, placeholders::_1), true);
		ratioSlider->setSlideValues(0.5f, 2.0f, 1.41f);

		mainArrangement->addItem(webcamView);
		mainArrangement->addItem(getPtr(ratioSlider));
		mainArrangement->addItem(getPtr(idButtons));
		mainArrangement->addItem(getPtr(rotationButtons));
		mainArrangement->addItem(getPtr(endButtons));
		mainArrangement->arrangeItems();

		canvas.push_back(getPtr(mainArrangement));

		if (webcamTexture::get()->webCam != nullptr) {
			webcamTexture::get()->webCam->shouldUpdate = true;
		}

		isSetup = true;
	}

	std::function<void()> reload = nullptr;

	size_t clickIndex = 0;
	size_t posIndex = 0;
	int priorityLayer = 100;

	void cleanupSubClasses() {
		if (webcamView != nullptr) {
			webcamView->cleanup();
			webcamView->image->mat[0]->cleanupDescriptor();
			webcamView = nullptr;
		}
	}

	int webcamIndex = 0;

private:

	ImagePanel* webcamView = nullptr;
	Slider* ratioSlider = nullptr;

	void calibrateWebcam(UIItem* owner) {
		if (webcamTexture::get()->webCam != nullptr) {
			webcamTexture::get()->webCam->calibrateCornerFilter();
		}
	}

	void addRotation(UIItem* owner) {
		webcamTexture::get()->webCam->setRotation(true);
		webcamTexture::get()->recreateWebcamImage();
		webcamView->image->mat[0]->cleanupDescriptor();
		webcamView->image->mat[0] = new Material(webcamTexture::get());
		ratioSlider->setSlideValues(0.5f, 2.0f, webcamTexture::get()->webCam->sizeRatio);
		reload();
		update();
	}

	void subtractRotation(UIItem* owner) {
		webcamTexture::get()->webCam->setRotation(false);
		webcamTexture::get()->recreateWebcamImage();
		webcamView->image->mat[0]->cleanupDescriptor();
		webcamView->image->mat[0] = new Material(webcamTexture::get());
		ratioSlider->setSlideValues(0.5f, 2.0f, webcamTexture::get()->webCam->sizeRatio);
		reload();
		update();
	}

	void indexUp(UIItem* owner) {
		webcamIndex++;
		webcamTexture::get()->webCam->switchWebcam(true);
		webcamTexture::get()->recreateWebcamImage();
		webcamView->image->mat[0]->cleanupDescriptor();
		webcamView->image->mat[0] = new Material(webcamTexture::get());
		reload();
		update();
	}

	void indexDown(UIItem* owner) {
		webcamIndex++;
		webcamTexture::get()->webCam->switchWebcam(false);
		webcamTexture::get()->recreateWebcamImage();
		webcamView->image->mat[0]->cleanupDescriptor();
		webcamView->image->mat[0] = new Material(webcamTexture::get());
		reload();
		update();
	}

	void updateAspectRatio(float newRatio) {
		webcamTexture::get()->webCam->updateAspectRatio(newRatio);
		webcamTexture::get()->recreateWebcamImage();
		webcamView->image->mat[0]->cleanupDescriptor();
		webcamView->image->mat[0] = new Material(webcamTexture::get());
		reload();
		update();
	}
};

class SaveMenu : public Widget {
public:
	SaveMenu(LoadList* assets) {
		loadList = assets;
	}

	void setup(std::function<void(UIItem*)> loadSessionFunc, std::function<void(UIItem*)> newSessionFunc) {
		if (isSetup) {
			return;
		}
		Arrangement* SessionButtons = new Arrangement(ORIENT_HORIZONTAL, 1.0f, 1.0f, 0.15f, 0.05f, 0.01f, ARRANGE_END);

		std::function<void(UIItem*)> saveSessionFunc = bind(&SaveMenu::save, this, placeholders::_1);

		imageData OpenButton = OPENBUTTON;
		Material* openMat = newMaterial(&OpenButton, "OpenBtn");

		imageData SaveButton = SAVEBUTTON;
		Material* saveMat = newMaterial(&SaveButton, "SaveBtn");

		imageData plusButton = PLUSBUTTON;
		Material* plusMat = newMaterial(&plusButton, "PlusBtn");

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

	void setup(std::function<void(UIItem*)> loadObjectFunct, std::function<void(UIItem*)> pipelinefunction, std::function<void(float)> polarCallback, std::function<void(float)> azimuthCallback){
		if (isSetup) {
			return;
		}

		imageData rb = RENDEREDBUTTON;
		Material* renderedMat = newMaterial(&rb, "RenderBtn");

		imageData ub = WEBCAMVIEWBUTTON;
		Material* webcamViewMat = newMaterial(&ub, "WebcamBtn");

		imageData wb = WIREFRAMEBUTTON;
		Material* wireframeViewMat = newMaterial(&wb, "WireframeBtn");

		imageData lb = LOADBUTTON;
		Material* LoadBtnMat = newMaterial(&lb, "LoadBtn");

		imageData tcb = TESTCHECKBOXBUTTON;
		Material* visibleMat = newMaterial(&tcb, "TestCheckBtn");

		Arrangement* Renderbuttons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.2f, 0.6f, 0.01f, ARRANGE_CENTER);

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

		Slider* polarSlider = new Slider(visibleMat, 0.0f, 0.0f, 1.0f, 0.25f);
		polarSlider->updateDisplay();
		polarSlider->setSlideValues(0.0f, 3.14159265f, 0.0f);
		polarSlider->setFloatCallback(polarCallback, true);

		Slider* azimuthSlider = new Slider(visibleMat, 0.0f, 0.0f, 1.0f, 0.25f);
		azimuthSlider->updateDisplay();
		azimuthSlider->setSlideValues(0.0f, 6.283185307f, 0.0f);
		azimuthSlider->setFloatCallback(azimuthCallback, true);

		Arrangement* buttons = new Arrangement(ORIENT_VERTICAL, -1.0f, 1.0f, 0.1f, 0.25f, 0.0f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		buttons->addItem(getPtr(new Button(LoadBtnMat, loadObjectFunct)));
		buttons->addItem(getPtr(Renderbuttons));
		buttons->addItem(getPtr(polarSlider));
		buttons->addItem(getPtr(azimuthSlider));

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
		invisibleMat = newMaterial(&ub, "UnrenderedBtn");

		imageData tcb = TESTCHECKBOXBUTTON;
		visibleMat = newMaterial(&tcb, "CheckboxBtn");

		imageData wb = WIREFRAMEBUTTON;
		wireframeMat = newMaterial(&wb, "WireframeBtn");

		canvas.push_back(getPtr(new Arrangement(ORIENT_VERTICAL, -1.0f, -0.75f, 0.1f, 0.5f, 0.01f, ARRANGE_START)));

		ObjectButtons = canvas[0];
		
		isSetup = true;
	}

	void addObject(std::function<void(UIItem*)> toggleFunction, std::function<void(UIItem*)> wireframeToggle) {
		ObjectButtons->arrangeItems();

		Arrangement* objButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 2.0f, 1.0f, 0.01f, ARRANGE_START);

		Checkbox* objectButton = new Checkbox(visibleMat, invisibleMat, toggleFunction);
		objectButton->Name = "Object button " + std::to_string(ObjectButtons->Items.size());

		Checkbox* objWireframeButton = new Checkbox(wireframeMat, invisibleMat, wireframeToggle);
		objWireframeButton->Name = objectButton->Name;

		ObjectMap.insert({ objectButton->Name, ObjectButtons->Items.size() });

		objButtons->addItem(getPtr(objectButton));
		objButtons->addItem(getPtr(objWireframeButton));
		objButtons->arrangeItems();

		ObjectButtons->addItem(getPtr(objButtons));
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
	Material* wireframeMat = nullptr;
};

class WebcamMenu : public Widget {
public:
	WebcamMenu(LoadList* assets) {
		loadList = assets;
	}

	void setup(std::function<void(UIItem*)> lightingFunction, std::function<void(UIItem*)> openSettings) {
		if (isSetup) {
			return;
		}
		imageData rb = RENDEREDBUTTON;
		Material* renderedMat = newMaterial(&rb, "RenderBtn");
		
		imageData fb = UNRENDEREDBUTTON;
		Material* unrenderedMat = newMaterial(&fb, "UnrenderedBtn");

		imageData plb = PLAYBUTTON;
		Material* playMat = newMaterial(&plb, "PlayBtn");

		imageData pb = PAUSEBUTTON;
		Material* pauseMat = newMaterial(&pb, "PauseBtn");

		imageData sb = SETTINGSBUTTON;
		Material* settingsMat = newMaterial(&sb, "SettingsBtn");

		imageData webcamOn = WEBCAMONBUTTON;
		Material* webcamMat = newMaterial(&webcamOn, "WebcamOnBtn");

		std::function<void(UIItem*)> toggleWebcamFunct = bind(&WebcamMenu::toggleWebcam, this, placeholders::_1);
		std::function<void(UIItem*)> configureWebcamFunct = bind(&WebcamMenu::calibrateWebcam, this, placeholders::_1);

		Arrangement* Videobuttons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 1.0f, 0.2f, 0.05f, 0.01f, ARRANGE_CENTER);

		Videobuttons->addItem(getPtr(new Button(webcamMat)));
		Videobuttons->addItem(getPtr(new Checkbox(playMat, pauseMat, toggleWebcamFunct)));
		Videobuttons->addItem(getPtr(new Button(settingsMat, openSettings))); // configureWebcamFunct)));
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

class Application {
public:
	void run() {
		engine->initWindow("BOBERT_TradPainter");
		engine->initVulkan();
		keyBinds.initCallbacks(engine->window);
		mouseManager.initCallbacks(engine->window);
		glfwSetScrollCallback(engine->window, camera.scrollCallback);
		sConst->setupSurfaceConstructor();
		createCanvas();
		if (sConst->webTex->webCam != nullptr) {
			sConst->webTex->webCam->loadFilter();
		}
		std::function<void()> tomogFunct = bind(&Application::toggleTomogMenu, this);
		std::function<void()> colourChange = bind(&Application::colourChangeTest, this);
		std::function<void()> FPSTrack = bind(&Application::startFPSTrack, this);
		std::function<void()> drawUpdate = bind(&Application::updateDrawVariables, this);
		sConst->setCallback(drawUpdate);
		keyBinds.addBinding(GLFW_KEY_1, colourChange, PRESS_EVENT);
		keyBinds.addBinding(GLFW_KEY_T, tomogFunct, PRESS_EVENT);
		keyBinds.addBinding(GLFW_KEY_F, FPSTrack, PRESS_EVENT);
		webcamTexture::get()->webCam->shouldUpdate = false;
		webcamMenu.canvas[0]->Items[1]->activestate = false;
		webcamMenu.canvas[0]->Items[1]->image->matidx = 1;

		Engine::get()->createRenderPass(testGP.renderPass, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		test = Engine::get()->createDrawImage(Engine::get()->swapChainExtent.width, Engine::get()->swapChainExtent.height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, testGP.renderPass);
		Engine::get()->createGraphicsPipelines(testGP);
		blurX.setup(xBlur, &test);
		blurY.setup(yBlur, blurX.getRenderTargets());

		currentPass = &testGP;// &Engine::get()->defaultPass;

		updateColourScheme();
		updateLightAzimuth(0.0f);
		updateLightPolar(0.0f);
		updateDrawVariables();
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
	RemapUI remapMenu = RemapUI(&UIElements, sConst);
	WebcamSettings webSets = WebcamSettings(&UIElements);


	shaderData* xBlur = new GAUSSBLURXSHADER;
	shaderData* yBlur = new GAUSSBLURYSHADER;

	UIItem* UITestImage = nullptr;

	vector<Widget*> widgets;

	postProcessFilter blurX;
	postProcessFilter blurY;
	drawImage test;
	GraphicsPass testGP;
	GraphicsPass* currentPass = nullptr;

	bool mouseDown = false;
	bool tomogActive = false;

	bool showWireframe = true;

	vector<StaticObject> staticObjects = {};
	PlaneObject* tomographyPlane = nullptr;
	vector<uint32_t> visibleObjects = {};

	bool lit = true;

	uint8_t viewIndex = 1;

	bool isTrackingFPS = false;
	uint32_t frameCount = 0;
	std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

	// Light position

	glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 5.0f);
	float polarAngle = 0.0f;
	float azimuthAngle = 0.0f;
	float lightRadius = 10.0f;

	Material* drawMat = nullptr;
	std::string renderPipelineName = "";
	uint32_t graphicsPipelineIndex = 0;
	VkPipelineLayout pipelineLayout = nullptr;
	
	// colours in sRGB format (for krita users these colours match the sRGB-elle-V2-g10.icc profile)
	glm::vec3 primaryColour = glm::vec3(0.42f, 0.06f, 0.11f);
	glm::vec3 secondaryColour = glm::vec3(0.82f, 0.55f, 0.36f);
	glm::vec3 tertiaryColour = glm::vec3(0.812f, 0.2f, 0.2f);
	glm::vec3 backgroundColour = glm::vec3(0.812f, 0.2f, 0.2f);

	void reloadWebcamTex() {
		sConst->reloadWebcamMat();
		surfaceMenu.setDiffuse(sConst->currentDiffuse());
		surfaceMenu.setNormal(sConst->currentNormal());
		surfaceMenu.update();
	}

	void colourChangeTest() {
		primaryColour = glm::vec3(0.0f, 0.13f, 0.27f); 
		secondaryColour = glm::vec3(0.0f, 0.55f, 0.32f);
		tertiaryColour = glm::vec3(0.0f, 0.39f, 0.31f);
		backgroundColour = glm::vec3(0.0f, 0.55f, 0.32f);
		updateColourScheme();
	}

	void updateVisibleObjects() {
		visibleObjects.clear();
		for (int32_t i = 0; i != staticObjects.size(); i++) {
			if (staticObjects[i].isVisible) {
				visibleObjects.push_back(i);
			}
		}
	}

	void startFPSTrack() {
		if (isTrackingFPS) {
			return;
		}
		startTime = std::chrono::steady_clock::now();
		isTrackingFPS = true;
		frameCount = 0;
	}

	void updateFPSTrack() {
		auto endTime = std::chrono::steady_clock::now();
		auto timeInterval = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();
		if (timeInterval < 1) {
			frameCount++;
			return;
		}
		std::cout << "FrameRate is ~" << frameCount << " FPS" << std::endl;
		isTrackingFPS = false;
	}

	void createWebSettings(UIItem* owner) {
		std::function<void(UIItem*)> finishSelf = std::bind(&Application::finishWebSettings, this, std::placeholders::_1);
		std::function<void()> updateWebTex = std::bind(&Application::reloadWebcamTex, this);

		webSets.setup(finishSelf, updateWebTex);
		if (!webSets.isSetup) {
			return;
		}
		webSets.clickIndex = mouseManager.addClickListener(webSets.getClickCallback());
		webSets.posIndex = mouseManager.addPositionListener(webSets.getPosCallback());

		widgets.push_back(&webSets);

		sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });
	}

	void finishWebSettings(UIItem* owner) {
		vkDeviceWaitIdle(Engine::get()->device);
		webSets.cleanup();

		mouseManager.removeClickListener(webSets.clickIndex);
		mouseManager.removePositionListener(webSets.posIndex);

		widgets.erase(find(widgets.begin(), widgets.end(), &webSets));

		sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });
	}

	void createRemapper(UIItem* owner) {
		std::function<void(UIItem*)> destroySelf = std::bind(&Application::destroyRemapper, this, std::placeholders::_1);
		std::function<void(UIItem*)> finishSelf = std::bind(&Application::finishRemapper, this, std::placeholders::_1);

		remapMenu.setup(destroySelf, finishSelf);
		if (!remapMenu.isSetup) {
			return;
		}
		remapMenu.clickIndex = mouseManager.addClickListener(remapMenu.getClickCallback());
		remapMenu.posIndex = mouseManager.addPositionListener(remapMenu.getPosCallback());

		surfaceMenu.hide();
		
		widgets.push_back(&remapMenu);

		sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });
	}

	void destroyRemapper(UIItem* owner) {
		vkDeviceWaitIdle(Engine::get()->device);

		sConst->normalType = 0;
		sConst->loadNormal(remapMenu.remapper->baseOSNormal->copyTexture());
		surfaceMenu.setNormal(sConst->currentNormal());

		remapMenu.cleanup();

		mouseManager.removeClickListener(remapMenu.clickIndex);
		mouseManager.removePositionListener(remapMenu.posIndex);

		surfaceMenu.show();

		widgets.erase(find(widgets.begin(), widgets.end(), &remapMenu));

		sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });
	}

	void finishRemapper(UIItem* owner) {
		sConst->normalType = 0;
		sConst->loadNormal(remapMenu.remapper->filteredOSNormal->copyTexture());
		surfaceMenu.setNormal(sConst->currentNormal());

		remapMenu.cleanup();
		mouseManager.removeClickListener(remapMenu.clickIndex);
		mouseManager.removePositionListener(remapMenu.posIndex);

		surfaceMenu.show();

		widgets.erase(find(widgets.begin(), widgets.end(), &remapMenu));

		sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });
	}

	void updateLightPolar(float angle) {
		polarAngle = angle;
		lightPos.x = lightRadius * sin(polarAngle) * cos(azimuthAngle);
		lightPos.y = lightRadius * sin(polarAngle) * sin(azimuthAngle);
		lightPos.z = lightRadius * cos(polarAngle);
	}

	void updateLightAzimuth(float angle) {
		azimuthAngle = angle;
		lightPos.x = lightRadius * sin(polarAngle) * cos(azimuthAngle);
		lightPos.y = lightRadius * sin(polarAngle) * sin(azimuthAngle);
		lightPos.z = lightRadius * cos(polarAngle);
	}

	void newSession(UIItem* owner) {
		// Remove all meshes
		for (StaticObject obj : staticObjects) {
			obj.mesh->cleanup();
		}
		staticObjects.clear();
		updateVisibleObjects();

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
		surfaceMenu.setDiffuse(sConst->currentDiffuse());

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

			std::function<void(UIItem*)> visibleFunction = std::bind(&Application::setObjectVisibility, this, placeholders::_1);
			std::function<void(UIItem*)> wireFunction = std::bind(&Application::setObjectWireframe, this, placeholders::_1);

			objectMenu.addObject(visibleFunction, wireFunction);

			newObject.isVisible = true;

			staticObjects.push_back(newObject);
		}
		if (session::get()->currentStudio.diffusePath != "None") {
			// This segment does not work properly because the surface menu construction produces errors
			imageTexture* loadedTexture = new imageTexture(session::get()->currentStudio.diffusePath, VK_FORMAT_R8G8B8A8_SRGB);

			sConst->diffuseIdx = 1;
			surfaceMenu.setDiffuse(sConst->currentDiffuse());
			surfaceMenu.resetDiffuseTog(true);
		}
		if (session::get()->currentStudio.OSPath != "None") {
			imageTexture* loadedTexture = new imageTexture(session::get()->currentStudio.OSPath, VK_FORMAT_R8G8B8A8_UNORM);
			if (!sConst->normalAvailable) {
				surfaceMenu.createNormalMenu(new UIItem);
			}
			sConst->normalType = 0;
			sConst->loadNormal(loadedTexture);
			surfaceMenu.setNormal(sConst->currentNormal());
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
			surfaceMenu.setNormal(sConst->currentNormal());
			surfaceMenu.resetNormalTog(true);
			surfaceMenu.toggleNormalState(false);
		}
		sConst->updateSurfaceMat();
		webcamTexture::get()->webCam->loadFilter();
		updateVisibleObjects();
	}
	
	void createCanvas() {

		std::function<void(UIItem*)> pipelinefunction = std::bind(&Application::setPipelineIndex, this, placeholders::_1);
		std::function<void(UIItem*)> lightingFunction = std::bind(&Application::toggleLighting, this, placeholders::_1);
		std::function<void(UIItem*)> loadObjectFunct = std::bind(&Application::buttonLoadStaticObject, this, placeholders::_1);
		std::function<void(UIItem*)> loadSessionFunc = std::bind(&Application::loadSave, this, placeholders::_1);
		std::function<void(UIItem*)> newSessionFunc = std::bind(&Application::newSession, this, placeholders::_1);
		std::function<void(UIItem*)> remapCallback = std::bind(&Application::createRemapper, this, placeholders::_1);

		std::function<void(UIItem*)> webcamSettings = std::bind(&Application::createWebSettings, this, placeholders::_1);

		std::function<void(float)> polarFunc = std::bind(&Application::updateLightPolar, this, placeholders::_1);
		std::function<void(float)> azimuthFunc = std::bind(&Application::updateLightAzimuth, this, placeholders::_1);

		objectMenu.setup();
		mouseManager.addClickListener(objectMenu.getClickCallback());
		widgets.push_back(&objectMenu);

		saveMenu.setup(loadSessionFunc, newSessionFunc);
		mouseManager.addClickListener(saveMenu.getClickCallback());
		widgets.push_back(&saveMenu);

		webcamMenu.setup(lightingFunction, webcamSettings);
		mouseManager.addClickListener(webcamMenu.getClickCallback());
		widgets.push_back(&webcamMenu);

		renderMenu.setup(loadObjectFunct, pipelinefunction, polarFunc, azimuthFunc);
		mouseManager.addClickListener(renderMenu.getClickCallback());
		mouseManager.addPositionListener(renderMenu.getPosCallback());
		widgets.push_back(&renderMenu);

		surfaceMenu.setup(sConst, &staticObjects, remapCallback);
		mouseManager.addClickListener(surfaceMenu.getClickCallback());
		widgets.push_back(&surfaceMenu);

		sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });
	}

	void updateColourScheme() {

		ColourSchemeObject cso{};
		cso.Primary = primaryColour;
		cso.Secondary = secondaryColour;
		cso.Tertiary = tertiaryColour;

		memcpy(engine->colourBufferMapped, &cso, sizeof(cso));
	}

	void toggleTomogMenu() {
		if (!tomogActive && sConst->diffTex != nullptr) {
			std::function<void(UIItem*)> toggleFunct = std::bind(&Application::toggleTomogMeshes, this, std::placeholders::_1);
			std::function<void(UIItem*)> tomogExit = std::bind(&Application::exitTomogMenu, this, std::placeholders::_1);
			
			if (!tomogUI.isSetup) {
				tomogUI.setup(sConst, toggleFunct, &mouseManager, tomogExit);
			}
			else {
				tomogUI.show();
			}
			
			tomographyPlane = new PlaneObject(sConst->diffTex->texWidth, sConst->diffTex->texHeight);
			tomographyPlane->isVisible = true;
			for (size_t i = 0; i != staticObjects.size(); i++) {
				staticObjects[i].isVisible = false;
			}
			updateVisibleObjects();
			objectMenu.hide();
			surfaceMenu.hide();
			
			tomogUI.clickIdx = mouseManager.addClickListener(tomogUI.getClickCallback());
			widgets.push_back(&tomogUI);

			sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });

			tomogActive = true;
			updateDrawVariables();
		}
	}

	void exitTomogMenu(UIItem* owner) {
		if (!tomogActive) {
			return;
		}
		vkQueueWaitIdle(engine->graphicsQueue);
		
		tomographyPlane->mesh->cleanup();
		delete tomographyPlane;
		tomographyPlane = nullptr;
		
		Texture* tomogDiff = UIElements.findTexPtr("TomogDiffTex");
		Texture* tomogNorm = UIElements.findTexPtr("TomogNormTex");
		if (tomogDiff != nullptr) {
			sConst->loadDiffuse(tomogDiff->copyTexture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, 0));
			surfaceMenu.setDiffuse(sConst->currentDiffuse());
		}
		if (tomogNorm != nullptr) {
			std::cout << "Normal found" << std::endl;
			sConst->normalType = 1;
			sConst->loadNormal(tomogNorm->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, 0));
			if (!sConst->normalAvailable) {
				surfaceMenu.createNormalMenu(owner);
			}
			sConst->normalType = 1;
			surfaceMenu.setNormal(sConst->currentNormal());
		}
		tomogActive = false;

		for (size_t i = 0; i != staticObjects.size(); i++) {
			staticObjects[i].isVisible = true;
		}
		updateVisibleObjects();
		
		objectMenu.show();
		surfaceMenu.show();

		mouseManager.removeClickListener(tomogUI.clickIdx);

		if (find(widgets.begin(), widgets.end(), &tomogUI) != widgets.end()) {
			widgets.erase(find(widgets.begin(), widgets.end(), &tomogUI));

			sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });
		}
		
		tomogUI.hide();
		updateDrawVariables();

	}

	void toggleTomogMeshes(UIItem* owner) {
		if (owner->activestate) {
			tomographyPlane->isVisible = true;
			for (size_t i = 0; i != staticObjects.size(); i++) {
				staticObjects[i].isVisible = false;
			}
			updateVisibleObjects();
			objectMenu.hide();
		}
		else {
			tomographyPlane->isVisible = false;
			for (size_t i = 0; i != staticObjects.size(); i++) {
				staticObjects[i].isVisible = true;
			}
			updateVisibleObjects();
			objectMenu.show();
		}
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(engine->window)) {
			if (isTrackingFPS) {
				updateFPSTrack();
			}
			glfwPollEvents();
			keyBinds.pollRepeatEvents();
			mouseManager.checkPositionEvents();
			webcamTexture::get()->updateWebcam();
			drawFrame();
		}
		vkDeviceWaitIdle(engine->device);
	}

	void buttonLoadStaticObject(UIItem* owner) {
		loadStaticObject();
	}

	void setObjectVisibility(UIItem* owner) {
		staticObjects[objectMenu.ObjectMap.at(owner->Name)].isVisible = owner->activestate;
		updateVisibleObjects();
	}

	void setObjectWireframe(UIItem* owner) {
		staticObjects[objectMenu.ObjectMap.at(owner->Name)].isWireframeVisible = owner->activestate;
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
		updateDrawVariables();
	}

	void loadStaticObject() {
		string modelPath;
		modelPath = winFile::OpenFileDialog();
		if (modelPath == "fail") {
			return;
		}
		StaticObject newObject(modelPath);
		newObject.mat = &sConst->surfaceMat;

		std::function<void(UIItem*)> visibleFunction = bind(&Application::setObjectVisibility, this, placeholders::_1);
		std::function<void(UIItem*)> wireFunction = bind(&Application::setObjectWireframe, this, placeholders::_1);

		objectMenu.addObject(visibleFunction, wireFunction);
		newObject.isVisible = true;

		staticObjects.push_back(newObject);
		session::get()->currentStudio.modelPaths.push_back(modelPath);

		updateVisibleObjects();
	}

	void cleanup() {
		for (uint32_t i = 0; i != staticObjects.size(); i++) {
			staticObjects[i].mesh->cleanup();
		}

		if (tomographyPlane != nullptr) {
			tomographyPlane->mesh->cleanup();
		}

		UIElements.empty();
		ObjectElements.empty();

		if (find(widgets.begin(), widgets.end(), &tomogUI) == widgets.end()) {
			tomogUI.cleanup();
		}
		
		for (size_t i = 0; i != widgets.size(); i++) {
			widgets[i]->cleanup();
		}

		blurX.cleanup();
		blurY.cleanup();
		test.cleanup(Engine::get()->device);
		testGP.cleanup(Engine::get()->device);
		//vkDestroyRenderPass(Engine::get()->device, testRP, nullptr);

		sConst->cleanup();
		engine->cleanup();
	}

	void drawFrame() {
		uint32_t imageIndex = engine->getRenderTarget();
		uint32_t currentFrame = engine->currentFrame;
		
		updateUniformBuffer(currentFrame);
		recordCommandBuffer(engine->commandBuffers[currentFrame], currentPass, imageIndex);

		VkResult result = engine->submitAndPresentFrame(imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || engine->framebufferResized) {
			engine->framebufferResized = false;

			if (!glfwGetWindowAttrib(engine->window, GLFW_ICONIFIED)) {
				for (size_t i = 0; i != widgets.size(); i++) {
					widgets[i]->update();
				}
			}	

			engine->recreateSwapChain();
			engine->recreateDrawImage(&test);
			blurX.cleanup();
			blurY.cleanup();
			blurX.setup(xBlur, &test);
			blurY.setup(yBlur, blurX.getRenderTargets());
			//normalizer.setup(testShader, &test);
			//normalizer.recreateDescriptorSets();
			return;
		}
		else if (result != VK_SUCCESS) {
			throw runtime_error("failed to acquire swap chain image!");
		}

		engine->currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void updateUniformBuffer(uint32_t currentImage) {

		// We probably don't need to do this for every frame

		camera.updateCamera(engine->window);

		UniformBufferObject ubo{};
		ubo.model = glm::mat4(1.0f);
		ubo.view = camera.view;
		ubo.proj = glm::perspective(glm::radians(camera.fov), engine->swapChainExtent.width / (float)engine->swapChainExtent.height, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		if (surfaceMenu.isVisible) {
			ubo.UVdistort[0] = 2 * surfaceMenu.diffuseView->extentx;
			ubo.UVdistort[1] = (surfaceMenu.diffuseView->posx) - surfaceMenu.diffuseView->extentx;
			ubo.UVdistort[2] = 2 * surfaceMenu.diffuseView->extenty;
			ubo.UVdistort[3] = (surfaceMenu.diffuseView->posy) - surfaceMenu.diffuseView->extenty;
		}
		else if (remapMenu.isVisible && remapMenu.isSetup) {
			ubo.UVdistort[0] = 2 * remapMenu.outMap->extentx;
			ubo.UVdistort[1] = (remapMenu.outMap->posx) - remapMenu.outMap->extentx;
			ubo.UVdistort[2] = 2 * remapMenu.outMap->extenty;
			ubo.UVdistort[3] = (remapMenu.outMap->posy) - remapMenu.outMap->extenty;
		}

		ubo.backgroundColour = backgroundColour;

		ubo.lightPosition = lightPos;
		ubo.viewPosition = camera.pos;

		memcpy(engine->uniformBuffersMapped[currentImage], &ubo, sizeof(ubo)); // uniformBuffersMapped is an array of pointers to each uniform buffer 
	} 

	void updateDrawVariables() {
		Material* activeSurfaceMat = &((lit) ? sConst->surfaceMat : sConst->unlitSurfaceMat);
		drawMat = ((!tomogActive) ? activeSurfaceMat : &tomogUI.scannedMaterial);
		renderPipelineName = (!tomogActive) ? sConst->renderPipeline : tomogUI.renderPipeline;
		graphicsPipelineIndex = (viewIndex == 1 && lit) ? engine->PipelineMap.at(renderPipelineName) : engine->pipelineindex;
		VkPipelineLayout pipelineLayoutSet[2] = { currentPass->diffusePipelineLayout, currentPass->diffNormPipelineLayout };
		pipelineLayout = (viewIndex == 1 && lit) ? pipelineLayoutSet[drawMat->pipelineLayoutIndex] : currentPass->diffusePipelineLayout;
	}

	void recordCommandBuffer(VkCommandBuffer commandBuffer, GraphicsPass* currentPass, uint32_t imageIndex) {
		uint32_t currentFrame = engine->currentFrame;

		//engine->beginRenderPass(commandBuffer, testRP, &test, imageIndex, backgroundColour);
		engine->beginRenderPass(commandBuffer, currentPass, &test, imageIndex, backgroundColour);

		if (showWireframe) {
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *currentPass->GraphicsPipelines[engine->PipelineMap.at("UVWireframe")]);

			for (uint32_t i : visibleObjects) {
				if (staticObjects[i].isWireframeVisible) {
					engine->drawObject(commandBuffer, staticObjects[i].mesh->vertexBuffer, staticObjects[i].mesh->indexBuffer, currentPass->diffusePipelineLayout, sConst->webcamPtr->descriptorSets[currentFrame], static_cast<uint32_t>(staticObjects[i].mesh->indices.size()));
				}
			}
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *currentPass->GraphicsPipelines[engine->PipelineMap.at("UIGrayShading")]);

		for (size_t i = 0; i != widgets.size(); i++) {
			widgets[i]->drawUI(commandBuffer, currentFrame);
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *currentPass->GraphicsPipelines[engine->PipelineMap.at("UIShading")]);

		for (size_t i = 0; i != widgets.size(); i++) {
			widgets[i]->drawImages(commandBuffer, currentFrame);
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *currentPass->GraphicsPipelines[graphicsPipelineIndex]);

		for (uint32_t i : visibleObjects) {
			engine->drawObject(commandBuffer, staticObjects[i].mesh->vertexBuffer, staticObjects[i].mesh->indexBuffer, pipelineLayout, drawMat->descriptorSets[currentFrame], static_cast<uint32_t>(staticObjects[i].mesh->indices.size()));
		}

		if (tomographyPlane != nullptr && tomographyPlane->isVisible) {
			engine->drawObject(commandBuffer, tomographyPlane->mesh->vertexBuffer, tomographyPlane->mesh->indexBuffer, tomogUI.scannedMaterial.pipelineLayout, tomogUI.scannedMaterial.descriptorSets[currentFrame], static_cast<uint32_t>(tomographyPlane->mesh->indices.size()));	
		}

		vkCmdEndRenderPass(commandBuffer);

		// Post-processing can be put here

		//blurX.filterImage(commandBuffer, imageIndex);
		//blurY.filterImage(commandBuffer, imageIndex);

		//VkImage resultImage = blurY.getFilterResult(commandBuffer, imageIndex);

		Engine::get()->copyImageToSwapchain(commandBuffer, &test, imageIndex);

		//Engine::get()->copyImageToSwapchain(commandBuffer, resultImage, test.imageExtent, imageIndex);

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