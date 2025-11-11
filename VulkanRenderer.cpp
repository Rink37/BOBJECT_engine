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

#include"include/BakedImages.h"

using namespace cv;
using namespace std;

std::vector<KeyManager*> KeyManager::_instances;
KeyManager keyBinds;

std::vector<MouseManager*> MouseManager::_instances;
MouseManager mouseManager;

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

	void setup(std::function<void(UIItem*)> lightingFunction) {
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
		Material* openMat = newMaterial(&OpenButton, "OpenBtn");
		
		imageData normal = NORMALTEXT;
		Material* normalMat = newMaterial(&normal, "NormalBtn");

		imageData diffuse = DIFFUSETEXT;
		Material* diffuseMat = newMaterial(&diffuse, "DiffuseBtn");

		std::function<void(UIItem*)> tomogLoadTop = bind(&TomographyMenu::loadTop, this, placeholders::_1);
		std::function<void(UIItem*)> tomogLoadBottom = bind(&TomographyMenu::loadBottom, this, placeholders::_1);
		std::function<void(UIItem*)> tomogLoadLeft = bind(&TomographyMenu::loadLeft, this, placeholders::_1);
		std::function<void(UIItem*)> tomogLoadRight = bind(&TomographyMenu::loadRight, this, placeholders::_1);
		std::function<void(UIItem*)> computeNormal = bind(&TomographyMenu::performNormTomog, this, placeholders::_1);
		std::function<void(UIItem*)> computeDiffuse = bind(&TomographyMenu::performDiffTomog, this, placeholders::_1);

		Arrangement* tomogButtons = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.1f, 0.4f, 0.01f);

		tomogButtons->addItem(getPtr(new Button(openMat, tomogLoadTop)));
		tomogButtons->addItem(getPtr(new Button(openMat, tomogLoadBottom)));
		tomogButtons->addItem(getPtr(new Button(openMat, tomogLoadLeft)));
		tomogButtons->addItem(getPtr(new Button(openMat, tomogLoadRight)));
		tomogButtons->addItem(getPtr(new Button(normalMat, computeNormal)));
		tomogButtons->addItem(getPtr(new Button(diffuseMat, computeDiffuse)));

		tomogButtons->updateDisplay();

		canvas.push_back(getPtr(tomogButtons));
		canvas.push_back(getPtr(new Grid(ORIENT_HORIZONTAL, 0.0f, 0.5f, 0.5f, 0.5f, 0.01f)));

		isSetup = true;
	}
private:
	Tomographer tomographer;

	surfaceConstructor* surface = nullptr;

	void loadTop(UIItem* owner) {
		string fileName = winFile::OpenFileDialog();
		if (fileName != string("fail")) {
			tomographer.add_image(fileName, 90.0, 50.0);
			cv::Mat loadedImage = cv::imread(fileName);
			UIItem* loadedUI = new ImagePanel(new Material(new imageTexture(loadedImage)), false);
			canvas[1]->addItem(loadedUI);
			canvas[1]->updateDisplay();
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

	void performNormTomog(UIItem* owner) {
		surface->diffTex->getCVMat();
		tomographer.outdims = Size(surface->diffTex->texMat.cols, surface->diffTex->texMat.rows);
		tomographer.alignTemplate = &surface->diffTex->texMat;
		tomographer.alignRequired = true;
		tomographer.calculate_normal();
		string saveName = winFile::SaveFileDialog();
		if (saveName != string("fail")) {
			imwrite(saveName, tomographer.computedNormal);
		}
		//tomographer.clearData();
	}

	void performDiffTomog(UIItem* owner) {
		surface->diffTex->getCVMat();
		tomographer.outdims = Size(surface->diffTex->texMat.cols, surface->diffTex->texMat.rows);
		tomographer.alignTemplate = &surface->diffTex->texMat;
		tomographer.alignRequired = true;
		tomographer.calculate_diffuse();
		string saveName = winFile::SaveFileDialog();
		if (saveName != string("fail")) {
			imwrite(saveName, tomographer.computedDiffuse);
		}
		//tomographer.clearData();
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
		//sliderTestFunc();
		if (sConst->webTex->webCam != nullptr) {
			sConst->webTex->webCam->loadFilter();
		}
		std::function<void()> tomogFunct = bind(&Application::toggleTomogMenu, this);
		std::function<void()> colourChange = bind(&Application::colourChangeTest, this);
		keyBinds.addBinding(GLFW_KEY_1, colourChange, PRESS_EVENT);
		keyBinds.addBinding(GLFW_KEY_T, tomogFunct, PRESS_EVENT);
		webcamTexture::get()->webCam->shouldUpdate = false;
		webcamMenu.canvas[0]->Items[1]->activestate = false;
		webcamMenu.canvas[0]->Items[1]->image->matidx = 1;
		updateColourScheme();
		updateLightAzimuth(0);
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
	Widget sliderTest = Widget(&UIElements);
	RemapUI remapper = RemapUI(&UIElements, sConst);

	UIItem* UITestImage = nullptr;

	vector<Widget*> widgets;

	double mouseX = 0.0;
	double mouseY = 0.0;

	bool mouseDown = false;
	bool tomogActive = false;

	bool showWireframe = true;

	vector<StaticObject> staticObjects = {};
	//map<string, int> ObjectMap = {};

	bool lit = true;

	uint8_t viewIndex = 1;

	// Light position

	glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 5.0f);
	float polarAngle, azimuthAngle = 0.0f;
	float lightRadius = 10.0f;
	
	// colours in sRGB format (for krita users these colours match the sRGB-elle-V2-g10.icc profile)
	glm::vec3 primaryColour = glm::vec3(0.42f, 0.06f, 0.11f);
	glm::vec3 secondaryColour = glm::vec3(0.82f, 0.55f, 0.36f);
	glm::vec3 tertiaryColour = glm::vec3(0.812f, 0.2f, 0.2f);
	glm::vec3 backgroundColour = glm::vec3(0.812f, 0.2f, 0.2f);

	void colourChangeTest() {
		primaryColour = glm::vec3(0.0f, 0.13f, 0.27f); 
		secondaryColour = glm::vec3(0.0f, 0.55f, 0.32f);
		tertiaryColour = glm::vec3(0.0f, 0.39f, 0.31f);
		backgroundColour = glm::vec3(0.0f, 0.55f, 0.32f);
		updateColourScheme();
	}

	void sliderTestFunc() {
		imageData tcb = TESTCHECKBOXBUTTON;
		Material* visibleMat = sliderTest.loadList->getPtr(new Material(sliderTest.loadList->getPtr(new imageTexture(&tcb, VK_FORMAT_R8_UNORM), "TestCheckBtnTex")), "TestCheckBtnMat");

		std::function<void(int)> intFunct = std::bind(&Application::testIntFunct, this, std::placeholders::_1);
		std::function<void(float)> floatFunct = std::bind(&Application::testFloatFunct, this, std::placeholders::_1);
		
		Rotator* test = new Rotator(visibleMat, 0.0f, 0.0f, 0.25f, 0.25f);
		test->setSlideValues(0.0f, 10.0f, 0.0f);
		test->setFloatCallback(floatFunct, false);
		test->updateDisplay();
		sliderTest.canvas.push_back(sliderTest.getPtr(test));
		sliderTest.isSetup = true;

		mouseManager.addClickListener(sliderTest.getClickCallback());
		mouseManager.addPositionListener(sliderTest.getPosCallback());
		widgets.push_back(&sliderTest);

		sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });
	}

	void createRemapper(UIItem* owner) {
		remapper.setup(sConst->diffTex, sConst->OSNormTex);
		mouseManager.addClickListener(remapper.getClickCallback());
		mouseManager.addPositionListener(remapper.getPosCallback());

		surfaceMenu.hide();
		
		widgets.push_back(&remapper);

		sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });
	}

	void destroyRemapper(UIItem* owner) {
		remapper.cleanup();
	}

	void updateLightPolar(float angle) {
		polarAngle = angle;
		//cout << polarAngle << endl;
		lightPos.x = lightRadius * sin(polarAngle) * cos(azimuthAngle);
		lightPos.y = lightRadius * sin(polarAngle) * sin(azimuthAngle);
		lightPos.z = lightRadius * cos(polarAngle);
	}

	void updateLightAzimuth(float angle) {
		azimuthAngle = angle;
		//cout << azimuthAngle << endl;
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
	}
	
	void createCanvas() {

		std::function<void(UIItem*)> pipelinefunction = std::bind(&Application::setPipelineIndex, this, placeholders::_1);
		std::function<void(UIItem*)> lightingFunction = std::bind(&Application::toggleLighting, this, placeholders::_1);
		std::function<void(UIItem*)> loadObjectFunct = std::bind(&Application::buttonLoadStaticObject, this, placeholders::_1);
		std::function<void(UIItem*)> loadSessionFunc = std::bind(&Application::loadSave, this, placeholders::_1);
		std::function<void(UIItem*)> newSessionFunc = std::bind(&Application::newSession, this, placeholders::_1);
		std::function<void(UIItem*)> remapCallback = std::bind(&Application::createRemapper, this, placeholders::_1);

		std::function<void(float)> polarFunc = std::bind(&Application::updateLightPolar, this, placeholders::_1);
		std::function<void(float)> azimuthFunc = std::bind(&Application::updateLightAzimuth, this, placeholders::_1);

		objectMenu.setup();
		mouseManager.addClickListener(objectMenu.getClickCallback());
		widgets.push_back(&objectMenu);

		saveMenu.setup(loadSessionFunc, newSessionFunc);
		mouseManager.addClickListener(saveMenu.getClickCallback());
		widgets.push_back(&saveMenu);

		webcamMenu.setup(lightingFunction);
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
		if (!tomogActive) {
			tomogUI.setup(sConst);
			mouseManager.addClickListener(tomogUI.getClickCallback());
			widgets.push_back(&tomogUI);
			tomogActive = true;
		}
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(engine->window)) {
			glfwPollEvents();
			keyBinds.pollRepeatEvents();
			mouseManager.checkPositionEvents();
			glfwGetCursorPos(engine->window, &mouseX, &mouseY);
			webcamTexture::get()->updateWebcam();
			drawFrame();
		}
		vkDeviceWaitIdle(engine->device);
	}

	//void testUIShader() {
	//	imageData test = D2NBUTTON_GRAY;
	//	UITestImage = new ImagePanel(0.0f, 0.0f, 0.1f, 0.1f, new Material(new imageTexture(&test, VK_FORMAT_R8_UNORM), true), false);
	//	UITestImage->updateDisplay();
	//}

	void buttonLoadStaticObject(UIItem* owner) {
		loadStaticObject();
	}

	void setObjectVisibility(UIItem* owner) {
		staticObjects[objectMenu.ObjectMap.at(owner->Name)].isVisible = owner->activestate;
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

	}

	void cleanup() {
		for (uint32_t i = 0; i != staticObjects.size(); i++) {
			staticObjects[i].mesh->cleanup();
		}

		UIElements.empty();
		ObjectElements.empty();

		for (size_t i = 0; i != widgets.size(); i++) {
			widgets[i]->cleanup();
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

		VkSwapchainKHR swapChains[] = { engine->swapChain };

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;

		result = vkQueuePresentKHR(engine->presentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || engine->framebufferResized) {
			engine->framebufferResized = false;

			if (!glfwGetWindowAttrib(engine->window, GLFW_ICONIFIED)) {
				for (size_t i = 0; i != widgets.size(); i++) {
					widgets[i]->update();
				}
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

		ubo.UVdistort[0] = 2*surfaceMenu.diffuseView->extentx;
		ubo.UVdistort[1] = (surfaceMenu.diffuseView->posx) - surfaceMenu.diffuseView->extentx;
		ubo.UVdistort[2] = 2*surfaceMenu.diffuseView->extenty;
		ubo.UVdistort[3] = (surfaceMenu.diffuseView->posy) - surfaceMenu.diffuseView->extenty;

		ubo.backgroundColour = backgroundColour;

		ubo.lightPosition = lightPos;
		ubo.viewPosition = camera.pos;

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
		clearValues[0].color = { {backgroundColour.r, backgroundColour.g, backgroundColour.b, 1.0f} };
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

		if (showWireframe) {

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *engine->GraphicsPipelines[engine->PipelineMap.at("UVWireframe")]);

			for (uint32_t i = 0; i != staticObjects.size(); i++) {
				if (staticObjects[i].isVisible && staticObjects[i].isWireframeVisible) {

					VkBuffer vertexBuffers[] = { staticObjects[i].mesh->vertexBuffer };
					VkDeviceSize offsets[] = { 0 };

					vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

					vkCmdBindIndexBuffer(commandBuffer, staticObjects[i].mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, engine->diffusePipelineLayout, 0, 1, &sConst->webcamPtr->descriptorSets[currentFrame], 0, nullptr);

					vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(staticObjects[i].mesh->indices.size()), 1, 0, 0, 0);
				}
			}
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *engine->GraphicsPipelines[engine->PipelineMap.at("UIGrayShading")]);

		for (size_t i = 0; i != widgets.size(); i++) {
			widgets[i]->drawUI(commandBuffer, currentFrame);
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *engine->GraphicsPipelines[engine->PipelineMap.at("UIShading")]);

		for (size_t i = 0; i != widgets.size(); i++) {
			widgets[i]->drawImages(commandBuffer, currentFrame);
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