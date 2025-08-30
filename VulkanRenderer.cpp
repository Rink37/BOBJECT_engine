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

class StaticObject {
public:
	StaticObject(string name) {
		mesh = new StaticMesh(name);
	}
	
	bool isVisible = false;
	StaticMesh* mesh = nullptr;
	Material* mat = nullptr;
};

class TomographyMenu : public Widget {
public:
	TomographyMenu(surfaceConstructor* sConst) {
		surface = sConst;
	}

	~TomographyMenu() {
		cleanup();
	}

	void setup() {
		if (isSetup) {
			return;
		}
		imageData OpenButton = OPENBUTTON;
		imageData normal = NORMALTEXT;

		std::function<void(UIItem*)> tomogLoadTop = bind(&TomographyMenu::loadTop, this, placeholders::_1);
		std::function<void(UIItem*)> tomogLoadBottom = bind(&TomographyMenu::loadBottom, this, placeholders::_1);
		std::function<void(UIItem*)> tomogLoadLeft = bind(&TomographyMenu::loadLeft, this, placeholders::_1);
		std::function<void(UIItem*)> tomogLoadRight = bind(&TomographyMenu::loadRight, this, placeholders::_1);
		std::function<void(UIItem*)> computeNormal = bind(&TomographyMenu::performTomog, this, placeholders::_1);

		vArrangement* tomogButtons = new vArrangement(0.0, 0.0, 0.1, 0.4, 0.01);

		tomogButtons->addItem(getPtr(new Button(&OpenButton, tomogLoadTop)));
		tomogButtons->addItem(getPtr(new Button(&OpenButton, tomogLoadBottom)));
		tomogButtons->addItem(getPtr(new Button(&OpenButton, tomogLoadLeft)));
		tomogButtons->addItem(getPtr(new Button(&OpenButton, tomogLoadRight)));
		tomogButtons->addItem(getPtr(new Button(&normal, computeNormal)));

		tomogButtons->updateDisplay();

		canvas.push_back(getPtr(tomogButtons));

		isSetup = true;
	}
private:
	Tomographer tomographer;

	surfaceConstructor* surface;

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
	Engine* engine = Engine::get();
	surfaceConstructor* sConst = surfaceConstructor::get();

	Camera camera;
	Tomographer tomographer;
	ImagePanel *diffuseView = nullptr;
	ImagePanel *normalView = nullptr;

	imageData ub = UNRENDEREDBUTTON;
	imageData tcb = TESTCHECKBOXBUTTON;

	vector<UIItem*> canvas{};

	TomographyMenu tomogUI{ sConst };

	vArrangement* ObjectButtons = nullptr;

	double mouseX = 0.0;
	double mouseY = 0.0;

	bool mouseDown = false;

	vector<StaticObject> staticObjects = {};
	map<string, int> ObjectMap = {};

	hArrangement *NormalButtons = nullptr;
	vArrangement *SurfacePanel = nullptr;

	Checkbox* diffuseTog = nullptr;
	Checkbox* normalTog = nullptr;

	bool lit = true;

	uint8_t viewIndex = 1;

	void newSession(UIItem* owner) {
		// Remove all meshes
		for (StaticObject obj : staticObjects) {
			obj.mesh->cleanup();
		}
		staticObjects.clear();
		for (UIItem* item : ObjectButtons->Items) {
			item->image->cleanup();
		}
		ObjectButtons->Items.clear();
		ObjectMap.clear();

		// Clear session data
		session::get()->clearStudio();
		
		// Clear all studio material data
		resetNormalUI(owner);
		sConst->clearSurface();
		sConst->normalAvailable = false;
		diffuseView->image->mat[0] = sConst->currentDiffuse();

		sConst->renderPipeline = "BFShading";
		sConst->updateSurfaceMat();
	}

	void resetNormalUI(UIItem* owner) {
		if (!sConst->normalAvailable) {
			return;
		}
		// Clear UI related to the normal component of the surface panel
		SurfacePanel->Items[SurfacePanel->Items.size() - 2]->cleanup();
		SurfacePanel->removeItem(SurfacePanel->Items.size() - 2);
		
		NormalButtons->cleanup();
		NormalButtons->Items.clear();

		std::function<void(UIItem*)> addNormalButton = bind(&Application::createNormalButtons, this, placeholders::_1);

		imageData normal = NORMALTEXT;
		imageData plusButton = PLUSBUTTON;

		Button* normalTextPanel = new Button(&normal);
		Button* normalPlus = new Button(&plusButton, addNormalButton);

		NormalButtons->addItem(normalTextPanel);
		NormalButtons->addItem(normalPlus);
		NormalButtons->addItem(new spacer);

		SurfacePanel->updateDisplay();
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

			Checkbox* objectButton = new Checkbox(0.0f, 0.0f, 0.15f, 0.15f, &tcb, &ub);

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
		if (session::get()->currentStudio.diffusePath != "None") {
			imageTexture* loadedTexture = new imageTexture(session::get()->currentStudio.diffusePath, VK_FORMAT_R8G8B8A8_SRGB);
			sConst->loadDiffuse(loadedTexture);
			sConst->diffuseIdx = 1;
			diffuseView->image->mat[0] = sConst->currentDiffuse();
			diffuseTog->activestate = false;
			diffuseTog->image->matidx = 1;
		}
		if (session::get()->currentStudio.OSPath != "None") {
			imageTexture* loadedTexture = new imageTexture(session::get()->currentStudio.OSPath, VK_FORMAT_R8G8B8A8_UNORM);
			if (!sConst->normalAvailable) {
				createNormalButtons(new UIItem);
			}
			sConst->normalType = 0;
			sConst->loadNormal(loadedTexture);
			normalView->image->mat[0] = sConst->currentNormal();
			normalTog->activestate = false;
			normalTog->image->matidx = 1;
			NormalButtons->Items[2]->activestate = true;
			NormalButtons->Items[2]->image->matidx = 0;
		}
		if (session::get()->currentStudio.TSPath != "None") {
			imageTexture* loadedTexture = new imageTexture(session::get()->currentStudio.TSPath, VK_FORMAT_R8G8B8A8_UNORM);
			if (!sConst->normalAvailable) {
				createNormalButtons(new UIItem);
			}
			sConst->normalType = 1;
			sConst->loadNormal(loadedTexture);
			sConst->TSmatching = true;
			normalView->image->mat[0] = sConst->currentNormal();
			normalTog->activestate = false;
			normalTog->image->matidx = 1;
			NormalButtons->Items[2]->activestate = false;
			NormalButtons->Items[2]->image->matidx = 1;
		}
		sConst->updateSurfaceMat();
		webcamTexture::get()->webCam->saveFilter();
	}

	void save(UIItem* owner) {
		string saveLocation;
		saveLocation = winFile::SaveFileDialog();
		if (saveLocation == "fail") {
			return;
		}
		session::get()->saveStudio(saveLocation);
	}
	
	void createCanvas() {
		hArrangement *Renderbuttons = new hArrangement(0.0f, 0.0f, 0.2f, 0.05f, 0.01f);
		hArrangement *Videobuttons = new hArrangement(0.0f, 1.0f, 0.2f, 0.05f, 0.01f);
		hArrangement *SessionButtons = new hArrangement(1.0f, 1.0f, 0.15f, 0.05f, 0.01f);

		SurfacePanel = new vArrangement(1.0f, 0.0f, 0.25f, 0.8f, 0.01f);

		std::function<void(UIItem*)> pipelinefunction = bind(&Application::setPipelineIndex, this, placeholders::_1);
		std::function<void(UIItem*)> lightingFunction = bind(&Application::toggleLighting, this, placeholders::_1);
		std::function<void(UIItem*)> toggleWebcamFunct = bind(&Application::toggleWebcam, this, placeholders::_1);
		std::function<void(UIItem*)> configureWebcamFunct = bind(&Application::calibrateWebcam, this, placeholders::_1);
		std::function<void(UIItem*)> loadObjectFunct = bind(&Application::buttonLoadStaticObject, this, placeholders::_1);
		std::function<void(UIItem*)> addNormalButton = bind(&Application::createNormalButtons, this, placeholders::_1);
		std::function<void(UIItem*)> toggleDiffuse = bind(&Application::toggleDiffuseCam, this, placeholders::_1);
		std::function<void(UIItem*)> loadDiffuse = bind(&Application::loadDiffuseImage, this, placeholders::_1);
		std::function<void(UIItem*)> saveWebcam = bind(&Application::saveDiffuseImage, this, placeholders::_1);
		std::function<void(UIItem*)> saveSessionFunc = bind(&Application::save, this, placeholders::_1);
		std::function<void(UIItem*)> loadSessionFunc = bind(&Application::loadSave, this, placeholders::_1);
		std::function<void(UIItem*)> newSessionFunc = bind(&Application::newSession, this, placeholders::_1);

		imageData lb = LOADBUTTON;
		imageData rb = RENDEREDBUTTON;
		imageData fb = UNRENDEREDBUTTON;
		imageData ub = WEBCAMVIEWBUTTON;
		imageData wb = WIREFRAMEBUTTON;
		imageData plb = PLAYBUTTON;
		imageData pb = PAUSEBUTTON;
		imageData sb = SETTINGSBUTTON;
		imageData diffuse = DIFFUSETEXT;
		imageData normal = NORMALTEXT;
		imageData webcamOn = WEBCAMONBUTTON;
		imageData webcamOff = WEBCAMOFFBUTTON;
		imageData OpenButton = OPENBUTTON;
		imageData SaveButton = SAVEBUTTON;
		imageData plusButton = PLUSBUTTON;

		SessionButtons->addItem(new Button(&plusButton, newSessionFunc));
		SessionButtons->addItem(new Button(&OpenButton, loadSessionFunc));
		SessionButtons->addItem(new Button(&SaveButton, saveSessionFunc));

		canvas.push_back(SessionButtons);

		Button* diffuseTextPanel = new Button(&diffuse);
		
		diffuseTog = new Checkbox(&webcamOn, &webcamOff, toggleDiffuse);
		diffuseTog->Name = "ToggleDiffuseWebcam";
		diffuseTog->setClickFunction(toggleDiffuse);

		Button* diffLoad = new Button(&OpenButton, loadDiffuse);
		Button* diffSave = new Button(&SaveButton, saveWebcam);

		hArrangement* DiffuseButtons = new hArrangement(0.0f, 0.0f, 1.0f, 0.2f, 0.01f);

		Button* normalTextPanel = new Button(&normal);
		Button* normalPlus = new Button(&plusButton, addNormalButton);

		NormalButtons = new hArrangement(0.0f, 0.0f, 1.0f, 0.2f, 0.01f);

		NormalButtons->addItem(normalTextPanel);
		NormalButtons->addItem(normalPlus);
		NormalButtons->addItem(new spacer);

		DiffuseButtons->addItem(diffuseTextPanel);
		DiffuseButtons->addItem(diffuseTog);
		DiffuseButtons->addItem(new spacer);
		DiffuseButtons->addItem(diffLoad);
		DiffuseButtons->addItem(diffSave);
		
		Button* loadObjectButton = new Button(&lb, loadObjectFunct);
		
		Button* litRenderingButton = new Button(&rb);
		Button* unlitRenderingButton = new Button(&ub);
		Button* wireframeRenderingButton = new Button(&wb);
		
		Checkbox* webcamToggle = new Checkbox(&plb, &pb, toggleWebcamFunct);
		Button* settingsButton = new Button(&sb, configureWebcamFunct);

		unlitRenderingButton->Name = "WebcamMat";
		unlitRenderingButton->setClickFunction(pipelinefunction);
		
		litRenderingButton->Name = "SurfaceMat";
		litRenderingButton->setClickFunction(pipelinefunction);
		
		wireframeRenderingButton->Name = "Wireframe";
		wireframeRenderingButton->setClickFunction(pipelinefunction);

		Renderbuttons->addItem(unlitRenderingButton);
		Renderbuttons->addItem(litRenderingButton);
		Renderbuttons->addItem(wireframeRenderingButton);

		Button* webcamImage = new Button(&webcamOn);

		Checkbox* litCheckbox = new Checkbox(&rb, &fb, lightingFunction);

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

		diffuseView = new ImagePanel(sConst->currentDiffuse(), true);
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
			if (defaultKeyBinds.getIsKeyDown(GLFW_KEY_L)) {
				tomogUI.setup();
			}
			if (state == GLFW_PRESS) {
				mouseDown = true;
			}
			else if (~state && mouseDown){
				tomogUI.checkForEvent(mouseX, mouseY, GLFW_PRESS);
				for (UIItem* item : canvas) {
					vector<UIItem*> scs;
					item->getSubclasses(scs);
					for (UIItem* sitem : scs) {
						sitem->checkForEvent(mouseX, mouseY, GLFW_PRESS);
					}
				}
				mouseDown = false;
			}
			drawFrame();
		}
		vkDeviceWaitIdle(engine->device);
	}

	void toggleDiffuseCam(UIItem* owner) {
		sConst->toggleDiffWebcam();
		diffuseView->image->mat[0] = sConst->currentDiffuse();
	}

	void toggleNormalCam(UIItem* owner) {
		sConst->toggleNormWebcam();
		normalView->image->mat[0] = sConst->currentNormal();
	}

	void toggleNormalType(UIItem* owner) {
		sConst->toggleNormType();
		if (sConst->normalType == 1 && sConst->OSNormTex != nullptr && !sConst->TSmatching) {
			if (sConst->TSNormTex != nullptr) {
				sConst->TSNormTex->cleanup();
			}
			sConst->transitionToTS(staticObjects[staticObjects.size() - 1].mesh);
			sConst->TSmatching = true;
			normalView->image->mat[0] = sConst->currentNormal();
		}
		else {
			normalView->image->mat[0] = sConst->currentNormal();
		}
	}

	void loadNormalImage(UIItem* owner) {
		string fileName = winFile::OpenFileDialog();
		if (fileName != string("fail")) {
			imageTexture* loadedTexture = new imageTexture(fileName, VK_FORMAT_R8G8B8A8_UNORM);
			sConst->loadNormal(loadedTexture);
			sConst->normalIdx = 1 + sConst->normalType;
			normalView->image->mat[0] = sConst->currentNormal();
			normalView->image->texHeight = diffuseView->image->mat[0]->textures[0]->texHeight;
			normalView->image->texWidth = diffuseView->image->mat[0]->textures[0]->texWidth;
			normalView->sqAxisRatio = static_cast<float>(normalView->image->texHeight) / static_cast<float>(normalView->image->texWidth);
			normalTog->activestate = false;
			normalTog->image->matidx = 1;
			sConst->updateSurfaceMat();
			SurfacePanel->arrangeItems();
			if (!sConst->normalType) {
				session::get()->currentStudio.OSPath = fileName;
			}
			else {
				session::get()->currentStudio.TSPath = fileName;
			}
		}
	}

	void saveNormalImage(UIItem* owner) {
		Mat saveNormal;
		if (sConst->normalIdx == 0) {
			saveNormal = webcamTexture::get()->webCam->webcamFrame;
		}
		else {
			if (sConst->normalType) {
				sConst->TSNormTex->getCVMat();
				saveNormal = sConst->TSNormTex->texMat.clone();
				sConst->TSNormTex->destroyCVMat();
			}
			else {
				sConst->OSNormTex->getCVMat();
				saveNormal = sConst->OSNormTex->texMat.clone();
				sConst->OSNormTex->destroyCVMat();
			}
		}
		string saveName = winFile::SaveFileDialog();
		if (saveName != string("fail")) {
			if (sConst->normalIdx == 1) {
				session::get()->currentStudio.OSPath = saveName;
			}
			else if (sConst->normalIdx == 2) {
				session::get()->currentStudio.TSPath = saveName;
			}
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
			diffuseView->image->texHeight = diffuseView->image->mat[0]->textures[0]->texHeight;
			diffuseView->image->texWidth = diffuseView->image->mat[0]->textures[0]->texWidth;
			diffuseView->sqAxisRatio = static_cast<float>(diffuseView->image->texHeight) / static_cast<float>(diffuseView->image->texWidth);
			diffuseTog->activestate = false;
			diffuseTog->image->matidx = 1;
			sConst->updateSurfaceMat();
			SurfacePanel->arrangeItems();
			session::get()->currentStudio.diffusePath = fileName;
		}
	}

	void saveDiffuseImage(UIItem* owner) {
		Mat saveDiffuse;
		if (sConst->diffuseIdx == 0) {
			saveDiffuse = webcamTexture::get()->webCam->webcamFrame;
		}
		else {
			sConst->diffTex->getCVMat();
			saveDiffuse = sConst->diffTex->texMat.clone();
			sConst->diffTex->destroyCVMat();
		}
		string saveName = winFile::SaveFileDialog();
		if (saveName != string("fail")) {
			session::get()->currentStudio.diffusePath = saveName;
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

		sConst->normalType = 0;
		if (sConst->OSNormTex == nullptr) {
			sConst->generateOSMap(staticObjects[staticObjects.size() - 1].mesh);
		}
		sConst->normalAvailable = true;
		webcamTexture::get()->changeFormat(VK_FORMAT_R8G8B8A8_UNORM);
		sConst->normalIdx = 1;
		sConst->updateSurfaceMat();

		SurfacePanel->removeItem(3);
		
		vector<UIImage*> images;
		NormalButtons->getImages(images);

		for (UIImage* image : images) {
			image->cleanup();
		}
		
		NormalButtons->Items.clear();

		imageData normal = NORMALTEXT;
		imageData webcamOn = WEBCAMONBUTTON;
		imageData webcamOff = WEBCAMOFFBUTTON;
		imageData OpenButton = OPENBUTTON;
		imageData SaveButton = SAVEBUTTON;
		imageData osType = OSBUTTON;
		imageData tsType = TANGENTSPACE;
		imageData diffToNorm = D2NBUTTON;

		Button* normalText = new Button(&normal);

		normalTog = new Checkbox(&webcamOn, &webcamOff, toggleWebcam);
		normalTog->activestate = false;
		normalTog->image->matidx = 1;

		Checkbox* mapTypeToggle = new Checkbox(&osType, &tsType, toggleType);
		Button* copyLayout = new Button(&diffToNorm, convertImg);
		Button* normalLoad = new Button(&OpenButton, loadNorm);
		Button* normalSave = new Button(&SaveButton, saveNorm);

		NormalButtons->addItem(normalText);
		NormalButtons->addItem(normalTog);
		NormalButtons->addItem(mapTypeToggle);
		NormalButtons->addItem(copyLayout);
		NormalButtons->addItem(normalLoad);
		NormalButtons->addItem(normalSave);

		NormalButtons->arrangeItems();
		NormalButtons->updateDisplay();

		normalView = new ImagePanel(sConst->currentNormal(), true);
		normalView->image->texHeight = diffuseView->image->texHeight;
		normalView->image->texWidth = diffuseView->image->texWidth;
		normalView->sqAxisRatio = static_cast<float>(normalView->image->texHeight) / static_cast<float>(normalView->image->texWidth);
		normalView->updateDisplay();

		SurfacePanel->addItem(normalView);
		SurfacePanel->addItem(new spacer);
		SurfacePanel->arrangeItems();
	}

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

	void buttonLoadStaticObject(UIItem* owner) {
		loadStaticObject();
	}

	void setObjectVisibility(UIItem* owner) {
		staticObjects[ObjectMap.at(owner->Name)].isVisible = owner->activestate;
	}

	void setPipelineIndex(UIItem* owner) {
		if (owner->Name == string("WebcamMat")) {
			viewIndex = 0;
			SurfacePanel->setVisibility(false);
			SurfacePanel->setIsEnabled(false);
		}
		else if (owner->Name == string("SurfaceMat")) {
			viewIndex = 1;
			SurfacePanel->setVisibility(true);
			SurfacePanel->setIsEnabled(true);
		}
		else if (owner->Name == string("Wireframe")) {
			viewIndex = 2;
			SurfacePanel->setVisibility(false);
			SurfacePanel->setIsEnabled(false);
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

			Checkbox* objectButton = new Checkbox(0.0f, 0.0f, 0.15f, 0.15f, &tcb, &ub);

			objectButton->updateDisplay();
			objectButton->setClickFunction(testfunction);

			newObject.isVisible = objectButton->activestate;

			objectButton->Name = "Object button " + std::to_string(ObjectButtons->Items.size());

			ObjectMap.insert({ objectButton->Name, staticObjects.size() });

			ObjectButtons->addItem(objectButton);
			ObjectButtons->arrangeItems();

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

		for (uint32_t i = 0; i != canvas.size(); i++) {
			canvas[i]->cleanup();
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

		for (uint32_t i = 0; i != canvas.size(); i++) {
			canvas[i]->draw(commandBuffer, currentFrame);
		}

		tomogUI.draw(commandBuffer, currentFrame);

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
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, engine->diffusePipelineLayout, 0, 1, &sConst->webcamMaterial.descriptorSets[currentFrame], 0, nullptr);

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