#include"Bobject_Engine.h"
#include"InputManager.h"
#include"WindowsFileManager.h"
#include"CameraController.h"
#include"Webcam_feeder.h"
#include"Materials.h"
#include"Tomography.h"
#include"LoadLists.h"
#include"Remapper.h"
#include"SeamFixer.h"
#include"Projector.h"
#include"Objects.h"

#include"WidgetTemplates.h"

#include<chrono>

#include"include/BakedImages.h"
#include"include/BakeAlpha.h"
#include"include/ExtractAlpha.h"
#include"include/SimpleIPAlphaOver.h"

#include"TradPainterWidgets.h"

using namespace cv;
using namespace std;

std::vector<KeyManager*> KeyManager::_instances;
KeyManager keyBinds;

std::vector<TypeManager*> TypeManager::_instances;

std::vector<MouseManager*> MouseManager::_instances;
MouseManager mouseManager;

class WidgetManager {
public:
	WidgetManager(MouseManager* mm) {
		mouseManager = mm;
	}

	void addWidget(std::string widgetName, Widget* widget) {
		if (widgets.count(widgetName) != 0) {
			return;
		}
		for (std::pair<std::string, Widget*> elem : widgets) {
			if (elem.second == widget) {
				return;
			}
		}
		if (widget->hasClickEvents) {
			widget->clickIndex = mouseManager->addClickListener(widget->getClickCallback());
		}
		else {
			widget->clickIndex = 255;
		}
		if (widget->hasPosEvents) {
			widget->posIndex = mouseManager->addPositionListener(widget->getPosCallback());
		}
		else {
			widget->posIndex = 255;
		}
		std::function<void(std::string)> delFunc = std::bind(&WidgetManager::deleteWidget, this, std::placeholders::_1);
		widget->deleteSelf = delFunc;
		widget->mappedName = widgetName;
		widgets.insert({ widgetName, widget });
		if (prioritySortedWidgets.size() == 0) {
			prioritySortedWidgets.push_back(widget);
		}
		else {
			uint32_t index = 0;
			while (prioritySortedWidgets[index]->priorityLayer > widget->priorityLayer && index < prioritySortedWidgets.size() - 1) {
				index++;
			}
			prioritySortedWidgets.insert(prioritySortedWidgets.begin() + index, widget);
		}
		updateFlag = true;
	}

	Widget* getWidget(std::string widgetName) {
		if (widgets.count(widgetName) == 0) {
			return nullptr;
		}
		return widgets.at(widgetName);
	}

	void drawWidgets(VkCommandBuffer commandBuffer, uint32_t currentFrame, GraphicsPass* currentPass) {
		for (std::pair<std::string, Widget*> widgetElem : widgets) {
			widgetElem.second->drawAll(commandBuffer, currentFrame, currentPass);
		}
	}

	void updateWidgets() {
		for (std::pair<std::string, Widget*> widgetElem : widgets) {
			widgetElem.second->update();
		}
	}

	void cleanup() {
		for (std::pair<std::string, Widget*> widgetElem : widgets) {
			widgetElem.second->cleanup();
		}
	}

	bool showWidgets = true;
	bool updateFlag = false; // A single condition which allows another class to check if it needs to update local pointers which are sources from the manager
	MouseManager* mouseManager{};
private:
	std::map<std::string, Widget*> widgets{};
	std::vector<Widget*> prioritySortedWidgets{};

	void deleteWidget(std::string widgetName) { // Cleans and deletes the selected widget. Widgets are responsible for deleting themselves.
		if (widgets.count(widgetName) == 0) {
			return;
		}

		vkDeviceWaitIdle(Core::getDevice());

		Widget* widget = widgets.at(widgetName);

		auto it = std::find(prioritySortedWidgets.begin(), prioritySortedWidgets.end(), widgets.at(widgetName));
		if (it != prioritySortedWidgets.end()) {
			prioritySortedWidgets.erase(it);
		}

		if (widget->clickIndex != 255) {
			uint32_t clickIndex = widget->clickIndex;
			mouseManager->removeClickListener(widget->clickIndex);
			for (std::pair<std::string, Widget*> widgetElem : widgets) {
				if (widgetElem.second->clickIndex > clickIndex) {
					widgetElem.second->clickIndex -= 1;
				}
			}
		}
		if (widget->posIndex != 255) {
			uint32_t posIndex = widget->posIndex;
			mouseManager->removePositionListener(widget->posIndex);
			for (std::pair<std::string, Widget*> widgetElem : widgets) {
				if (widgetElem.second->posIndex > posIndex) {
					widgetElem.second->posIndex -= 1;
				}
			}
		}

		widgets.at(widgetName)->cleanup();
		delete widgets.at(widgetName);
		widgets.erase(widgetName);

		updateFlag = true;
	}
};

class Application {
public:
	void run() {
		engine->initEngine("BOBERT_TradPainter");
		keyBinds.initCallbacks(engine->window);
		mouseManager.initCallbacks(engine->window);
		glfwSetScrollCallback(engine->window, camera.scrollCallback);
		webcamTexture::get()->setup();
		TextureElements.getPtr(webcamTexture::get(), "Webcam View");
		std::function<void()> colourChange = bind(&Application::colourChangeTest, this);
		std::function<void()> colourChange2 = bind(&Application::colourChangeTest2, this);
		std::function<void()> colourChange3 = bind(&Application::colourChangeTest3, this);
		std::function<void()> FPSTrack = bind(&Application::startFPSTrack, this);
		std::function<void()> widgetToggle = bind(&Application::toggleWidgets, this);
		std::function<void()> resetCamPos = bind(&Application::resetCamera, this);
		std::function<void()> fullResetCamPos = bind(&Application::fullResetCamera, this);
		std::function<void()> setCamPosToProjector = bind(&Application::setCameraToProjector, this);
		std::function<void()> swizzle = bind(&Application::updateSwizzle, this);
		keyBinds.addBinding(GLFW_KEY_1, colourChange2, PRESS_EVENT, std::vector<int>{GLFW_KEY_LEFT_SHIFT});
		keyBinds.addBinding(GLFW_KEY_C, resetCamPos, PRESS_EVENT, std::vector<int>{GLFW_KEY_LEFT_CONTROL});
		keyBinds.addBinding(GLFW_KEY_C, fullResetCamPos, PRESS_EVENT, std::vector<int>{GLFW_KEY_LEFT_CONTROL, GLFW_KEY_LEFT_SHIFT});
		keyBinds.addBinding(GLFW_KEY_1, colourChange, PRESS_EVENT);
		keyBinds.addBinding(GLFW_KEY_1, colourChange3, RELEASE_EVENT);
		keyBinds.addBinding(GLFW_KEY_F, FPSTrack, PRESS_EVENT);
		keyBinds.addBinding(GLFW_KEY_F1, widgetToggle, PRESS_EVENT);
		keyBinds.addBinding(GLFW_KEY_GRAVE_ACCENT, swizzle, PRESS_EVENT);
		keyBinds.addBinding(GLFW_KEY_TAB, setCamPosToProjector, PRESS_EVENT);
		webcamTexture::get()->webCam->shouldUpdate = false;

		Engine::get()->createRenderPass(renderGP.renderPass, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		renderImage = Engine::get()->createDrawImage(Engine::get()->swapChainExtent.width, Engine::get()->swapChainExtent.height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, renderGP.renderPass);
		Engine::get()->createGraphicsPipelines(renderGP);

		currentPass = &renderGP;

		webcamMat = TextureElements.getPtr(new Material(webcamTexture::get(), "BF"), "Webcam Material");
		MaterialTemplate wireMatTemplate(std::string("W"), &renderGP);
		wireMatTemplate.listChannels();
		wireMat = UIElements.getPtr(wireMatTemplate.createMaterial(), "Wire Material");

		createWidgetMaterials();
		createCanvas();
		
		WebcamMenu* webcamMenu = static_cast<WebcamMenu*>(widgetManager.getWidget("WebcamMenu"));
		webcamMenu->canvas->Items[1]->activestate = false;
		webcamMenu->canvas->Items[1]->image->matidx = 1;

		updateColourScheme();
		updateWindowSize();

		updateLightAzimuth(0.0f);
		updateLightPolar(0.0f);
		mainLoop();
		webcamTexture::destruct();
		cleanup();
		Engine::destruct();
	}
private:
	LoadList UIElements{};
	LoadList TextureElements{};

	Engine* engine = Engine::get();

	Camera camera;

	WidgetManager widgetManager = WidgetManager(&mouseManager);
	ObjectMenu* objectMenu = nullptr;
	TextureMenu* textureMenu = nullptr;
	RemapUI* remapMenu = nullptr;
	TextureSettings* textureSettings = nullptr;
	TomographyMenu* tomogUI = nullptr;

	Material* webcamMat = nullptr;
	Material* wireMat = nullptr;

	drawImage renderImage;
	GraphicsPass renderGP;
	GraphicsPass* currentPass = nullptr;

	StaticObject* currentObject = nullptr;

	bool tomogActive = false;
	bool showWireframe = true;
	bool inWebSettings = false;
	bool lit = true;
	bool isTrackingFPS = false;
	uint8_t viewIndex = 1;

	vector<StaticObject> staticObjects = {};
	PlaneObject* tomographyPlane = nullptr;
	std::map<VkPipeline*, std::vector<uint32_t>> objectPipelines{};
	vector<uint32_t> visibleObjects = {};

	uint32_t frameCount = 0;

	glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 5.0f); // Light position
	float polarAngle = 0.0f;
	float azimuthAngle = 0.0f;
	float lightRadius = 10.0f;

	// colours in sRGB format (for krita users these colours match the sRGB-elle-V2-g10.icc profile)
	glm::vec3 primaryColour = glm::vec3(0.42f, 0.06f, 0.11f);
	glm::vec3 secondaryColour = glm::vec3(0.82f, 0.55f, 0.36f);
	glm::vec3 tertiaryColour = glm::vec3(0.812f, 0.2f, 0.2f);
	glm::vec3 backgroundColour = glm::vec3(0.812f, 0.2f, 0.2f);

	// World settings
	glm::vec3 defaultCameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
	float scaleFactor = 1.0f;
	uint8_t swizzle = 0;
	
	// Projector
	bool debugView = false;
	bool inProjectorView = false;
	Projector* projector = nullptr;

	std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

	void createErrorDialog(std::string title = "ERROR", std::string message = "") {
		ErrorDialog* errorDlg = new ErrorDialog(&UIElements);
		errorDlg->setup(title, message);
		widgetManager.addWidget("ErrorDialog", errorDlg);
	}

	void toggleWidgets() {
		widgetManager.showWidgets = !widgetManager.showWidgets;
	}

	void checkForWidgetChanges() {
		remapMenu = (widgetManager.getWidget("remapMenu") != nullptr) ? static_cast<RemapUI*>(widgetManager.getWidget("remapMenu")) : nullptr;
		textureSettings = (widgetManager.getWidget("textureSettings") != nullptr) ? static_cast<TextureSettings*>(widgetManager.getWidget("textureSettings")) : nullptr;;
		tomogUI = (widgetManager.getWidget("tomogUI") != nullptr) ? static_cast<TomographyMenu*>(widgetManager.getWidget("tomogUI")) : nullptr;;
		widgetManager.updateFlag = false;
	}

	void createWidgetMaterials() {
		UIElements.getPtr(new Material(std::vector<Texture*>{}, std::vector <std::pair<std::vector<VkBuffer>, uint32_t>>{std::pair<std::vector<VkBuffer>, uint32_t>{std::vector<VkBuffer>{Engine::get()->colourBuffer}, sizeof(ColourSchemeObject)}, std::pair<std::vector<VkBuffer>, uint32_t>{std::vector<VkBuffer>{Engine::get()->windowSizeBuffer}, sizeof(WindowSizeObject)}}, "UIRoundBox", currentPass), "UIRoundBox");
		imageData cb = CLOSEBUTTON;
		UIElements.getPtr(&cb, "UIGray", "CloseBtn");
		imageData sb = SETTINGSBUTTON;
		UIElements.getPtr(&sb, "UIGray", "SettingsBtn");
		imageData tsb = TANGENTSPACE;
		UIElements.getPtr(&tsb, "UIGray", "TSBtn");
		imageData osb = OSBUTTON;
		UIElements.getPtr(&osb, "UIGray", "OSBtn");
		imageData pb = PLAYBUTTON;
		UIElements.getPtr(&pb, "UIGray", "PlayBtn");
		imageData psb = PAUSEBUTTON;
		UIElements.getPtr(&psb, "UIGray", "PauseBtn");
		imageData bb = BACKBUTTON;
		UIElements.getPtr(&bb, "UIGray", "BackBtn");
		imageData rf = ROTATEFORWARD;
		UIElements.getPtr(&rf, "UIGray", "RotateFWBtn");
		imageData rtb = ROTATEBACKWARD;
		UIElements.getPtr(&rtb, "UIGray", "RotateBWBtn");
		imageData tcb = TESTCHECKBOXBUTTON;
		UIElements.getPtr(&tcb, "UIGray", "TestCheckBtn");
		imageData OpenButton = OPENBUTTON;
		UIElements.getPtr(&OpenButton, "UIGray", "OpenBtn");
		imageData SaveButton = SAVEBUTTON;
		UIElements.getPtr(&SaveButton, "UIGray", "SaveBtn");
		imageData plusButton = PLUSBUTTON;
		UIElements.getPtr(&plusButton, "UIGray", "PlusBtn");
		imageData rb = RENDEREDBUTTON;
		UIElements.getPtr(&rb, "UIGray", "RenderBtn");
		imageData ub = WEBCAMVIEWBUTTON;
		UIElements.getPtr(&ub, "UIGray", "WebcamBtn");
		imageData wb = WIREFRAMEBUTTON;
		UIElements.getPtr(&wb, "UIGray", "WireframeBtn");
		imageData urb = UNRENDEREDBUTTON;
		UIElements.getPtr(&urb, "UIGray", "UnrenderedBtn");
		imageData webcamOn = WEBCAMONBUTTON;
		UIElements.getPtr(&webcamOn, "UIGray", "WebcamOnBtn");
		imageData plb = PLANEBUTTON;
		UIElements.getPtr(&plb, "UIGray", "PlaneBtn");
		imageData crs = CLOSEBUTTON;
		UIElements.getPtr(&crs, "UIGray", "CrossBtn");
	}

	void createProjectorObjPicker(UIItem* owner) {
		if (projector != nullptr) {
			return;
		}
		std::string texName = owner->Name;

		std::vector<std::string> objectNames{};
		for (std::pair<std::string, int> elem : objectMenu->ObjectMap) {
			objectNames.push_back(elem.first);
		}

		std::function<void(UIItem*)> cancelFunc = std::bind(&Application::openTextureSettingsMenu, this, std::placeholders::_1);

		ObjSelector* pob = new ObjSelector(&UIElements, &TextureElements);
		pob->setText("Projecting " + texName + " onto Object : ");
		pob->setErrorFunc(std::bind(&Application::createErrorDialog, this, std::placeholders::_1, std::placeholders::_2));
		pob->setup(texName, objectNames, std::bind(&Application::openProjector, this, std::placeholders::_1), cancelFunc);

		widgetManager.addWidget("pob", pob);
	}

	void openProjector(UIItem* owner) {
		Mesh* selectedMesh = staticObjects[objectMenu->ObjectMap.at(owner->text)].mesh;

		createProjector(owner->Name, selectedMesh);
	}

	void createProjector(std::string texName, Mesh* mesh) {
		if (mesh == nullptr) {
			return;
		}
		projector = new Projector(TextureElements.getTexture(texName), mesh, texName, std::bind(&Application::addTexture, this, std::placeholders::_1, std::placeholders::_2), currentPass);
		projector->setCamera(&camera);
		projector->calculateUVs();
		inProjectorView = true;
	}

	void setProjectorAngle(float angle) {
		if (projector != nullptr) {
			projector->updateMaxAngle(angle);
		}
	}

	void finishProjector(UIItem* owner) {
		if (projector != nullptr) {
			projector->reprojectImage();
			projector->cleanup();
			delete projector;
			projector = nullptr;
		}
	}

	void cancelProjector(UIItem* owner) {
		if (projector != nullptr) {
			projector->cleanup();
			delete projector;
			projector = nullptr;
		}
	}

	void addTexture(std::string name, Texture* tex) {
		TextureElements.getPtr(tex, name);
		UIItem* temp = new spacer();
		temp->Name = name;
		textureMenu->getAddTexCallback()(temp);
		delete temp;
	}

	void setCameraToProjector() {
		if (projector == nullptr) {
			return;
		}
		projector->shareCamera(&camera);
		inProjectorView = true;
	}

	void openSeamObjPicker(UIItem* owner) {
		std::string texName = owner->Name;
		
		std::vector<std::string> objectNames{};
		for (std::pair<std::string, int> elem : objectMenu->ObjectMap) {
			objectNames.push_back(elem.first);
		}

		std::function<void(UIItem*)> cancelFunc = std::bind(&Application::openTextureSettingsMenu, this, std::placeholders::_1);

		ObjSelector* sob = new ObjSelector(&UIElements, &TextureElements);
		sob->setText("Seam-fixing " + texName + " based on Object:");
		sob->setErrorFunc(std::bind(&Application::createErrorDialog, this, std::placeholders::_1, std::placeholders::_2));
		sob->setup(texName, objectNames, std::bind(&Application::openSeamFixer ,this, std::placeholders::_1), cancelFunc);
		
		widgetManager.addWidget("sob", sob);
	}

	void openSeamFixer(UIItem* owner) {
		std::string texName = owner->Name;
		std::string objName = owner->text;
		Mesh* selectedMesh = staticObjects[objectMenu->ObjectMap.at(objName)].mesh;

		SeamFixMenu* seamFixer = new SeamFixMenu(&UIElements, &TextureElements);
		seamFixer->setup(selectedMesh, texName, std::bind(&Application::openTextureSettingsMenu, this, std::placeholders::_1));
		widgetManager.addWidget("seamFixer", seamFixer);
	}

	void colourChangeTest() {
		primaryColour = glm::vec3(0.0f, 0.13f, 0.27f); 
		secondaryColour = glm::vec3(0.0f, 0.55f, 0.32f);
		tertiaryColour = glm::vec3(0.0f, 0.39f, 0.31f);
		backgroundColour = glm::vec3(0.0f, 0.55f, 0.32f);
		updateColourScheme();
	}

	void colourChangeTest2() {
		primaryColour = glm::vec3(0.42f, 0.06f, 0.11f);
		secondaryColour = glm::vec3(0.82f, 0.55f, 0.36f);
		tertiaryColour = glm::vec3(0.812f, 0.2f, 0.2f);
		backgroundColour = glm::vec3(0.812f, 0.2f, 0.2f);
		updateColourScheme();
	}

	void colourChangeTest3() {
		primaryColour = glm::vec3(0.0f, 0.13f, 0.27f);
		tertiaryColour = glm::vec3(0.0f, 0.55f, 0.32f);
		secondaryColour = glm::vec3(0.0f, 0.39f, 0.31f);
		backgroundColour = glm::vec3(0.0f, 0.55f, 0.32f);
		updateColourScheme();
	}

	void updateSwizzle() {
		swizzle += 1;
		swizzle %= 3;
		for (uint32_t i = 0; i != staticObjects.size(); i++) {
			staticObjects[i].mesh->updateSwizzle();
		}
	}

	void updateVisibleObjects() {
		visibleObjects.clear();
		objectPipelines.clear();
		for (uint32_t i = 0; i != staticObjects.size(); i++) {
			if (staticObjects[i].isVisible) {
				visibleObjects.push_back(i);
				VkPipeline* pipeline = currentPass->GraphicsPipelines[currentPass->pipelineMap.at(staticObjects[i].shaderName)];
				if (objectPipelines.count(pipeline) == 0) {
					objectPipelines.insert({ pipeline, std::vector<uint32_t>{i} });
				}
				else {
					objectPipelines.at(pipeline).push_back(i);
				}
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
		std::function<void()> updateTexView = std::bind(&Application::updateTextureSettingsMenu, this);

		WebcamSettings* webSets = new WebcamSettings(&UIElements, &TextureElements);
		webSets->setup(finishSelf, updateTexView);
		if (!webSets->isSetup) {
			return;
		}
		widgetManager.addWidget("webcamSettings", webSets);

		inWebSettings = true;
		webcamTexture::get()->interruptFrameUpdate();
		
		WebcamMenu* webcamMenu = static_cast<WebcamMenu*>(widgetManager.getWidget("WebcamMenu"));
		webcamMenu->canvas->Items[1]->image->matidx = 0;
		webcamMenu->canvas->Items[1]->activestate = true;
	}

	void finishWebSettings(UIItem* owner) {
		widgetManager.getWidget("webcamSettings")->selfDestruct(owner);
		webcamTexture::get()->startFrameUpdate();
		inWebSettings = false;
	}

	void alphaExtract(UIItem* owner) {
		std::string texName = owner->Name;
		Texture* tex = TextureElements.getTexture(texName)->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1);

		filter extractAlpha{ std::vector<Texture*>{tex}, new EXTRACTALPHASHADER };
		extractAlpha.filterImage();
		addTexture(texName + std::string("_alpha"), extractAlpha.filterTarget[0]->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, 1));
		extractAlpha.cleanup();
		tex->cleanup();
		delete tex;
	}

	void createAlphaMixTexSelector(UIItem* owner) {
		std::function<void(std::string, std::string)> continueCallback = std::bind(&Application::alphaMix, this, std::placeholders::_1, std::placeholders::_2);

		TexSelector* amts = new TexSelector(&UIElements, &TextureElements);
		amts->setText(std::string("Baking Alpha ") + owner->Name, std::string("Into texture:"));
		amts->setErrorFunc(std::bind(&Application::createErrorDialog, this, std::placeholders::_1, std::placeholders::_2));
		amts->setup(owner->Name, continueCallback);

		widgetManager.addWidget("amts", amts);
	}

	void alphaMix(std::string texName, std::string alphaName) {
		Texture* tex = TextureElements.getTexture(texName);
		Texture* alpha = TextureElements.getTexture(alphaName);

		filter bakeAlpha{ std::vector<Texture*>{tex, alpha}, new BAKEALPHASHADER};
		bakeAlpha.filterImage();
		bakeAlpha.cleanup();
	}

	void createAlphaOverTexSelector(UIItem* owner) {
		std::function<void(std::string, std::string)> continueCallback = std::bind(&Application::alphaOver, this, std::placeholders::_1, std::placeholders::_2);
		
		TexSelector* aots = new TexSelector(&UIElements, &TextureElements);
		aots->setText(std::string("Mixing ") + owner->Name, std::string("Into texture:"));
		aots->setErrorFunc(std::bind(&Application::createErrorDialog, this, std::placeholders::_1, std::placeholders::_2));
		aots->setup(owner->Name, continueCallback);

		widgetManager.addWidget("aots", aots);
	}

	void alphaOver(std::string texName, std::string overlayName) {
		Texture* tex = TextureElements.getTexture(texName)->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TILING_OPTIMAL, 1);
		Texture* overlay = TextureElements.getTexture(overlayName)->copyTexture(tex->texWidth, tex->texHeight);

		filter alphaOver{ std::vector<Texture*>{tex, overlay}, new SIMPLEIPALPHAOVERSHADER };
		alphaOver.filterImage();
		alphaOver.cleanup();

		overlay->cleanup();
		delete overlay;

		TextureElements.replacePtr(tex, texName);
	}

	void createRemapTexSelector(UIItem* owner) {
		std::function<void(std::string, std::string)> continueCallback = std::bind(&Application::createRemapper, this, std::placeholders::_1, std::placeholders::_2);

		RemapTexSelector* rts = new RemapTexSelector(&UIElements, &TextureElements);
		rts->setErrorFunc(std::bind(&Application::createErrorDialog, this, std::placeholders::_1, std::placeholders::_2));
		rts->setup(owner->Name, continueCallback);
		
		widgetManager.addWidget("rts", rts);
	}

	void createRemapper(std::string refTexture, std::string targetTexture) {
		std::function<void(UIItem*)> destroySelf = std::bind(&Application::destroyRemapper, this, std::placeholders::_1);
		std::function<void(UIItem*)> finishSelf = std::bind(&Application::finishRemapper, this, std::placeholders::_1);

		remapMenu = new RemapUI(&UIElements, &TextureElements);
		remapMenu->setup(refTexture, targetTexture, destroySelf, finishSelf); 
		if (!remapMenu->isSetup) {
			return;
		}

		widgetManager.addWidget("remapMenu", remapMenu);
	}

	void destroyRemapper(UIItem* owner) {
		RemapUI* remapMenu = static_cast<RemapUI*>(widgetManager.getWidget("remapMenu"));
		if (remapMenu == nullptr) {
			return;
		}
		remapMenu->remapper->baseTarget->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		remapMenu->remapper->baseTarget->textureLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		std::string outName = remapMenu->targetTexName;
		TextureElements.replacePtr(remapMenu->remapper->baseTarget->copyTexture(), outName);
		owner->Name = outName;

		remapMenu->selfDestruct(owner);
		openTextureSettingsMenu(owner);
	}

	void finishRemapper(UIItem* owner) {
		RemapUI* remapMenu = static_cast<RemapUI*>(widgetManager.getWidget("remapMenu"));
		if (remapMenu == nullptr) {
			return;
		}
		std::string outName = remapMenu->targetTexName;
		TextureElements.replacePtr(remapMenu->remapper->filteredTarget->copyTexture(), outName);
		owner->Name = outName;

		remapMenu->selfDestruct(owner);
		openTextureSettingsMenu(owner);
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

	void createTexLoadMenu(std::function<void(UIItem*)> updateTexMenu) {		
		TextureLoadMenu* tlm = new TextureLoadMenu(&UIElements, &TextureElements);
		tlm->setErrorFunc(std::bind(&Application::createErrorDialog, this, std::placeholders::_1, std::placeholders::_2));
		tlm->setup(updateTexMenu);
		if (!tlm->isSetup) {
			return;
		}
		widgetManager.addWidget("tlm", tlm);
	}

	void openTextureSettingsMenu(UIItem* owner) {
		if (textureSettings != nullptr) {
			std::cout << "Texture settings already exists!" << std::endl;
			return;
		}

		std::function<void(UIItem*)> seamFixFunc = std::bind(&Application::openSeamObjPicker, this, std::placeholders::_1);
		std::function<void(UIItem*)> alphaBakeFunc = std::bind(&Application::createAlphaMixTexSelector, this, std::placeholders::_1);
		std::function<void(UIItem*)> alphaExtractFunc = std::bind(&Application::alphaExtract, this, std::placeholders::_1);
		std::function<void(UIItem*)> alphaOverFunc = std::bind(&Application::createAlphaOverTexSelector, this, std::placeholders::_1);
		std::function<void(UIItem*)> projectFunc = std::bind(&Application::createProjectorObjPicker, this, std::placeholders::_1);
		std::function<void(UIItem*)> cancelProjectorFunc = std::bind(&Application::cancelProjector, this, std::placeholders::_1);
		std::function<void(UIItem*)> finishProjectorFunc = std::bind(&Application::finishProjector, this, std::placeholders::_1);
		std::function<void(float)> updateProjectorAngle = std::bind(&Application::setProjectorAngle, this, std::placeholders::_1);

		textureSettings = new TextureSettings(&UIElements, &TextureElements);
		textureSettings->setup(owner->Name, std::bind(&Application::createRemapTexSelector, this, std::placeholders::_1), std::bind(&Application::createSpaceTransitionMenu, this, std::placeholders::_1), std::bind(&Application::createNormalMixer, this, std::placeholders::_1), seamFixFunc, projectFunc, cancelProjectorFunc, finishProjectorFunc, alphaBakeFunc, alphaOverFunc, updateProjectorAngle, alphaExtractFunc);
		widgetManager.addWidget("textureSettings", textureSettings);
	}

	void updateTextureSettingsMenu() {
		vkDeviceWaitIdle(Engine::get()->device);
		TextureElements.getMaterial("Webcam Material")->cleanup();
		TextureElements.getMaterial("Webcam Material")->init(webcamTexture::get());
		textureSettings->updateWebcamTex();
		textureSettings->update();
	}

	void openObjectSettingsMenu(UIItem* owner) {
		std::function<void(std::function<void(UIItem*)>, std::string)> openEditMenu = std::bind(&Application::openEditSettingsMenu, this, std::placeholders::_1, std::placeholders::_2);
		
		ObjectSettingsMenu* osm = new ObjectSettingsMenu(&UIElements, &TextureElements);
		osm->setup(&staticObjects[objectMenu->ObjectMap.at(owner->Name)], textureMenu->getAddTexCallback(), std::bind(&Application::openSettingsMenu, this, std::placeholders::_1), std::bind(&Application::closeSettingsMenu, this, std::placeholders::_1), openEditMenu);
		widgetManager.addWidget("OSM", osm);
	}

	void openSettingsMenu(std::function<void(UIItem*)> closeFnc) {
		Widget* osm = widgetManager.getWidget("OSM");
		osm->hide();
		MaterialCreator* mc = new MaterialCreator(&UIElements, &TextureElements);
		mc->setup("BF", currentPass, closeFnc);
		widgetManager.addWidget("mc", mc);
	}

	void openEditSettingsMenu(std::function<void(UIItem*)> closeFnc, std::string matName) {
		Widget* osm = widgetManager.getWidget("OSM");
		osm->hide();
		MaterialCreator* mc = new MaterialCreator(&UIElements, &TextureElements);
		mc->setupEditMode("BF", currentPass, closeFnc, matName);
		widgetManager.addWidget("mc", mc);
	}

	void closeSettingsMenu(UIItem* owner) {
		Widget* osm = widgetManager.getWidget("OSM");
		osm->show();
		osm->update();
		updateVisibleObjects();
	}
	
	void createCanvas() {

		std::function<void(UIItem*)> pipelinefunction = std::bind(&Application::setPipelineIndex, this, placeholders::_1);
		std::function<void(UIItem*)> lightingFunction = std::bind(&Application::toggleLighting, this, placeholders::_1);
		std::function<void(UIItem*)> loadObjectFunct = std::bind(&Application::buttonLoadStaticObject, this, placeholders::_1);
		std::function<void(UIItem*)> webcamSettings = std::bind(&Application::createWebSettings, this, placeholders::_1);
		std::function<void(UIItem*)> texModify = std::bind(&Application::openTextureSettingsMenu, this, placeholders::_1);
		std::function<void(UIItem*)> tomogCallback = std::bind(&Application::createTomogSelector, this, placeholders::_1);

		std::function<void(std::function<void(UIItem*)>)> openTexLoad = std::bind(&Application::createTexLoadMenu, this, placeholders::_1);

		std::function<void(float)> polarFunc = std::bind(&Application::updateLightPolar, this, placeholders::_1);
		std::function<void(float)> azimuthFunc = std::bind(&Application::updateLightAzimuth, this, placeholders::_1);

		objectMenu = new ObjectMenu(&UIElements);
		objectMenu->setup(loadObjectFunct);
		widgetManager.addWidget("ObjectMenu", objectMenu);

		textureMenu = new TextureMenu(&UIElements, &TextureElements);
		textureMenu->setup(texModify, openTexLoad, tomogCallback);
		widgetManager.addWidget("TextureMenu", textureMenu);

		WebcamMenu* webcamMenu = new WebcamMenu(&UIElements);
		webcamMenu->setup(lightingFunction, webcamSettings);
		widgetManager.addWidget("WebcamMenu", webcamMenu);

		RenderMenu* renderMenu = new RenderMenu(&UIElements);
		renderMenu->setup(loadObjectFunct, pipelinefunction, polarFunc, azimuthFunc);
		widgetManager.addWidget("RenderMenu", renderMenu);
	}

	void updateColourScheme() {

		ColourSchemeObject cso{};
		cso.Primary = primaryColour;
		cso.Secondary = secondaryColour;
		cso.Tertiary = tertiaryColour;

		memcpy(engine->colourBufferMapped, &cso, sizeof(cso));
	}

	void updateWindowSize() {

		WindowSizeObject wso{};
		wso.WindowSize = glm::vec2(Core::get()->windowWidth, Core::get()->windowHeight);

		memcpy(engine->windowSizeBufferMapped, &wso, sizeof(wso));
	}

	Mesh* getObj(std::string objName) {
		if (objectMenu->ObjectMap.count(objName) == 0) {
			return nullptr;
		}
		return staticObjects[objectMenu->ObjectMap.at(objName)].mesh;
	}

	void createNormalMixer(UIItem* owner) {
		std::vector<std::string> objects{};
		for (std::pair<std::string, int> elem : objectMenu->ObjectMap) {
			objects.push_back(elem.first);
		}

		std::string texName = owner->Name;
		std::function<Mesh* (std::string)> getMeshFnc = std::bind(&Application::getObj, this, std::placeholders::_1);

		NormalMixer* normalMixer = new NormalMixer(&UIElements, &TextureElements);
		normalMixer->setErrorFunc(std::bind(&Application::createErrorDialog, this, std::placeholders::_1, std::placeholders::_2));
		normalMixer->setup(texName, objects, getMeshFnc, std::bind(&Application::exitNormalMixer, this, std::placeholders::_1), std::bind(&Application::cancelNormalMixer, this, std::placeholders::_1));
		widgetManager.addWidget("normalMixer", normalMixer);
	}

	void cancelNormalMixer(UIItem* owner) {		
		widgetManager.getWidget("normalMixer")->selfDestruct(owner);
		openTextureSettingsMenu(owner);
	}

	void exitNormalMixer(UIItem* owner) {
		openTextureSettingsMenu(owner);
	}

	void createSpaceTransitionMenu(UIItem* owner) {
		std::vector<std::string> objects{};
		for (std::pair<std::string, int> elem : objectMenu->ObjectMap) {
			objects.push_back(elem.first);
		}

		std::string texName = owner->Name;

		std::function<Mesh*(std::string)> getMeshFnc = std::bind(&Application::getObj, this, std::placeholders::_1);
		std::function<void(UIItem*)> exitFnc = std::bind(&Application::exitSpaceTransitionMenu, this, std::placeholders::_1);

		SpaceTransitionMenu* stm = new SpaceTransitionMenu(&UIElements, &TextureElements);
		stm->setup(texName, objects, textureMenu->getAddTexCallback(), getMeshFnc, exitFnc);

		widgetManager.addWidget("stm", stm);
	}

	void exitSpaceTransitionMenu(UIItem* owner) {
		openTextureSettingsMenu(owner);
	}

	void createTomogSelector(UIItem* owner) {
		std::function<void(UIItem*)> cancelFunc = std::bind(&Application::exitTomogSelector, this, std::placeholders::_1);
		std::function<void(UIItem*)> finishFunc = std::bind(&Application::openTomogMenu, this, std::placeholders::_1);
		
		TomogRefPicker* tomogPicker = new TomogRefPicker(&UIElements, &TextureElements);
		tomogPicker->setErrorFunc(std::bind(&Application::createErrorDialog, this, std::placeholders::_1, std::placeholders::_2));
		tomogPicker->setup(finishFunc, cancelFunc);

		WebcamMenu* webcamMenu = static_cast<WebcamMenu*>(widgetManager.getWidget("WebcamMenu"));
		webcamMenu->hide();
		RenderMenu* renderMenu = static_cast<RenderMenu*>(widgetManager.getWidget("RenderMenu"));
		renderMenu->hideViewButtons();

		widgetManager.addWidget("tomogPicker", tomogPicker);
	}

	void exitTomogSelector(UIItem* owner) {
		WebcamMenu* webcamMenu = static_cast<WebcamMenu*>(widgetManager.getWidget("WebcamMenu"));
		webcamMenu->show();
		RenderMenu* renderMenu = static_cast<RenderMenu*>(widgetManager.getWidget("RenderMenu"));
		renderMenu->showViewButtons();

		widgetManager.getWidget("tomogPicker")->selfDestruct(owner);
	}

	void openTomogMenu(UIItem* owner) {
		std::function<void(UIItem*)> toggleFunct = std::bind(&Application::toggleTomogMeshes, this, std::placeholders::_1);
		std::function<void(UIItem*)> tomogExit = std::bind(&Application::exitTomogMenu, this, std::placeholders::_1);

		tomogUI = new TomographyMenu(&UIElements, &TextureElements);
		tomogUI->setErrorFunc(std::bind(&Application::createErrorDialog, this, std::placeholders::_1, std::placeholders::_2));
		tomogUI->setup(owner->Name, toggleFunct, tomogExit, &mouseManager);

		Texture* refTex = TextureElements.getTexture(owner->Name);

		tomographyPlane = new PlaneObject(refTex->texWidth, refTex->texHeight);
		tomographyPlane->isVisible = true;
		for (size_t i = 0; i != staticObjects.size(); i++) {
			staticObjects[i].isVisible = false;
		}
		updateVisibleObjects();

		exitTomogSelector(owner);

		widgetManager.addWidget("tomogUI", tomogUI);

		tomogActive = true;
	}

	void exitTomogMenu(UIItem* owner) {
		if (!tomogActive) {
			return;
		}
		vkQueueWaitIdle(engine->graphicsQueueObj->queue);
		
		tomographyPlane->mesh->cleanup();
		delete tomographyPlane;
		tomographyPlane = nullptr;
		
		UIItem* tempItem = new spacer;
		std::function<void(UIItem*)> addTex = textureMenu->getAddTexCallback();

		if (UIElements.checkForTexture("TomogDiffTex")) {
			Texture* tomogDiff = UIElements.findTexPtr("TomogDiffTex");
			
			TextureElements.getPtr(tomogDiff->copyTexture(), "TomogDiffTex");
			UIElements.deleteTexture("TomogDiffTex");

			tempItem->Name = "TomogDiffTex";
			addTex(tempItem);
		}
		if (UIElements.checkForTexture("TomogNormTex")) {
			Texture* tomogNorm = UIElements.findTexPtr("TomogNormTex");
			tomogNorm->isNormal = true;
			tomogNorm->normalType = true;

			TextureElements.getPtr(tomogNorm->copyTexture(), "TomogNormTex");
			UIElements.deleteTexture("TomogNormTex");

			tempItem->Name = "TomogNormTex";
			addTex(tempItem);
		}
		tempItem->cleanup();
		delete tempItem;

		tomogActive = false;

		for (size_t i = 0; i != staticObjects.size(); i++) {
			staticObjects[i].isVisible = true;
		}
		updateVisibleObjects();

		widgetManager.getWidget("tomogUI")->selfDestruct(owner);
		tomogUI = nullptr;

		WebcamMenu* webcamMenu = static_cast<WebcamMenu*>(widgetManager.getWidget("WebcamMenu"));
		webcamMenu->show();
		RenderMenu* renderMenu = static_cast<RenderMenu*>(widgetManager.getWidget("RenderMenu"));
		renderMenu->showViewButtons();
	}
	
	void toggleTomogMeshes(UIItem* owner) {
		if (owner->activestate) {
			tomographyPlane->isVisible = true;
			for (size_t i = 0; i != staticObjects.size(); i++) {
				staticObjects[i].isVisible = false;
			}
			updateVisibleObjects();
		}
		else {
			tomographyPlane->isVisible = false;
			for (size_t i = 0; i != staticObjects.size(); i++) {
				staticObjects[i].isVisible = true;
			}
			updateVisibleObjects();
		}
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(engine->window)) {
			try {
				if (isTrackingFPS) {
					updateFPSTrack();
				}
				glfwPollEvents();
				keyBinds.pollRepeatEvents();
				mouseManager.checkPositionEvents();
				if (!inWebSettings) {
					webcamTexture::get()->asyncUpdate();
				}
				else {
					webcamTexture::get()->updateWebcam();
				}
				drawFrame();
			}
			catch (const exception& e) {
				std::cout << e.what() << std::endl;
				createErrorDialog("YOU... SHOULDN'T BE HERE", "An uncategorized error has been encountered. Let me know how you got to here!");
			}
		}
		webcamTexture::get()->interruptFrameUpdate();
		vkDeviceWaitIdle(engine->device);
	}

	void buttonLoadStaticObject(UIItem* owner) {
		loadStaticObject();
	}

	void setObjectVisibility(UIItem* owner) {
		staticObjects[objectMenu->ObjectMap.at(owner->Name)].isVisible = owner->activestate;
		updateVisibleObjects();
	}

	void setObjectWireframe(UIItem* owner) {
		staticObjects[objectMenu->ObjectMap.at(owner->Name)].isWireframeVisible = owner->activestate;
	}

	void setPipelineIndex(UIItem* owner) {
		if (owner->Name == string("WebcamMat")) {
			viewIndex = 0;
			objectMenu->hide();
			textureMenu->hide();
			
			widgetManager.getWidget("textureSettings")->destruct();
			widgetManager.updateFlag = true;

			std::function<void(UIItem*)> projectFunc = std::bind(&Application::createProjectorObjPicker, this, std::placeholders::_1);
			std::function<void(UIItem*)> cancelProjectorFunc = std::bind(&Application::cancelProjector, this, std::placeholders::_1);
			std::function<void(UIItem*)> finishProjectorFunc = std::bind(&Application::finishProjector, this, std::placeholders::_1);
			std::function<void(float)> updateProjectorAngle = std::bind(&Application::setProjectorAngle, this, std::placeholders::_1);

			textureSettings = new TextureSettings(&UIElements, &TextureElements);
			textureSettings->setup("Webcam View", std::bind(&Application::createRemapTexSelector, this, std::placeholders::_1), nullptr, nullptr, nullptr, projectFunc, cancelProjectorFunc, finishProjectorFunc, nullptr, nullptr, updateProjectorAngle, nullptr);

			widgetManager.addWidget("textureSettings", textureSettings);
		}
		else if (owner->Name == string("SurfaceMat")) {
			viewIndex = 1;
			objectMenu->show();
			textureMenu->show();

			widgetManager.getWidget("textureSettings")->destruct();
			widgetManager.updateFlag = true;
		}
		else if (owner->Name == string("Wireframe")) {
			viewIndex = 2;
			objectMenu->show();
			textureMenu->show();

			widgetManager.getWidget("textureSettings")->destruct();
			widgetManager.updateFlag = true;
		}
	}

	void toggleLighting(UIItem* owner) {
		lit = owner->activestate;
	}

	void loadStaticObject() {
		string modelPath;
		modelPath = winFile::OpenFileDialog();
		if (modelPath == "fail") {
			return;
		}
		std::string objectName = winFile::getBaseName(modelPath);
		std::string extension = winFile::getExtension(objectName);
		winFile::stripExtension(objectName);
		if (extension != std::string("obj")) {
			createErrorDialog("FILE ERROR:", std::string("File extension \".") + extension + std::string("\" is not currently supported for models"));
			return;
		}
		StaticObject newObject(modelPath, swizzle, scaleFactor);

		newObject.objectName = objectName;

		std::function<void(UIItem*)> visibleFunction = bind(&Application::setObjectVisibility, this, placeholders::_1);
		std::function<void(UIItem*)> wireFunction = bind(&Application::setObjectWireframe, this, placeholders::_1);
		std::function<void(UIItem*)> optionsFunction = std::bind(&Application::openObjectSettingsMenu, this, placeholders::_1);

		objectMenu->addObject(visibleFunction, wireFunction, optionsFunction, objectName);
		newObject.isVisible = true;
		newObject.setMat(TextureElements.getMaterial("Webcam Material"), "BF");

		staticObjects.push_back(newObject);
		if (staticObjects.size() == 1) {
			resetCamera();
		}

		updateVisibleObjects();
	}

	void updateCentralLocation() {
		uint32_t objectCount = static_cast<uint32_t>(staticObjects.size());
		glm::vec3 avgPos{ 0.0f, 0.0f, 0.0f };
		for (uint32_t i = 0; i != objectCount; i++) {
			avgPos += staticObjects[i].mesh->bbox.position;
		}
		if (objectCount > 0) {
			defaultCameraPos = avgPos / static_cast<float>(objectCount);
		}
	}

	void resetCamera() {
		updateCentralLocation();
		camera.center = defaultCameraPos;
	}

	void fullResetCamera() {
		resetCamera();
		camera.getInstance().distance = 5.0f;
		camera.initxrot = 45.0f;
		camera.inityrot = 10.0f;
		camera.xrot = 45.0f;
		camera.yrot = 10.0f;
	}

	void cleanup() {
		for (uint32_t i = 0; i != staticObjects.size(); i++) {
			staticObjects[i].mesh->cleanup();
		}

		if (tomographyPlane != nullptr) {
			tomographyPlane->mesh->cleanup();
		}

		UIElements.empty();
		TextureElements.empty();
		widgetManager.cleanup();

		if (projector != nullptr) {
			projector->cleanup();
			delete projector;
		}

		renderImage.cleanup(Core::getDevice());
		renderGP.cleanup(Core::getDevice());

		engine->cleanup(); 
	}

	void drawFrame() {
		uint32_t imageIndex = engine->getRenderTarget();
		uint32_t currentFrame = engine->currentFrame;
		
		updateUniformBuffer(currentFrame);
		recordCommandBuffer(engine->commandBuffers[currentFrame], currentPass, imageIndex);

		VkResult result = engine->submitAndPresentFrame(imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || Core::get()->framebufferResized) {
			Core::get()->framebufferResized = false;

			if (!glfwGetWindowAttrib(engine->window, GLFW_ICONIFIED)) {
				widgetManager.updateWidgets();
			}	

			engine->recreateSwapChain();
			engine->recreateDrawImage(&renderImage);
			
			updateWindowSize();

			return;
		}
		else if (result != VK_SUCCESS) {
			throw runtime_error("failed to acquire swap chain image!");
		}

		engine->currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void updateUniformBuffer(uint32_t currentImage) {
		// We probably don't need to do this for every frame

		if (widgetManager.updateFlag) {
			checkForWidgetChanges();
		}

		camera.updateCamera(engine->window);

		UniformBufferObject ubo{};
		ubo.model = glm::mat4(1.0f);
		ubo.view = camera.view;
		ubo.proj = glm::perspective(glm::radians(camera.fov), engine->swapChainExtent.width / (float)engine->swapChainExtent.height, 0.1f, 25.0f);
		ubo.proj[1][1] *= -1;

		if (remapMenu != nullptr && remapMenu->isVisible && remapMenu->isSetup && (!debugView || projector == nullptr)) {
			ubo.UVdistort[0] = 2 * remapMenu->outMap->extentx;
			ubo.UVdistort[1] = (remapMenu->outMap->posx) - remapMenu->outMap->extentx;
			ubo.UVdistort[2] = 2 * remapMenu->outMap->extenty;
			ubo.UVdistort[3] = (remapMenu->outMap->posy) - remapMenu->outMap->extenty;
		}
		else if (textureSettings != nullptr && textureSettings->isVisible && textureSettings->isSetup && (!debugView || projector == nullptr)) {
			textureSettings->getUVPos(ubo.UVdistort);
		}
		else if (projector != nullptr && debugView) {
			float projectedWidth = static_cast<float>(static_cast<float>(Core::get()->windowHeight) / static_cast<float>(projector->tex->texHeight) * projector->tex->texWidth) / static_cast<float>(Core::get()->windowWidth);
			ubo.UVdistort[0] = 2.0f * projectedWidth;
			ubo.UVdistort[1] = -1.0f * projectedWidth;
			ubo.UVdistort[2] = 2.0f;
			ubo.UVdistort[3] = -1.0f;
		}
		else {
			ubo.UVdistort[0] = 0;
			ubo.UVdistort[1] = 0;
			ubo.UVdistort[2] = 0;
			ubo.UVdistort[3] = 0;
		}

		ubo.backgroundColour = backgroundColour;

		ubo.lightPosition = lightPos;
		ubo.viewPosition = camera.pos;
		if (inProjectorView && projector != nullptr) {
			if (camera.pos != projector->pos) {
				inProjectorView = false;
			}
		}
		else if (inProjectorView) {
			inProjectorView = false;
		}

		memcpy(engine->uniformBuffersMapped[currentImage], &ubo, sizeof(ubo)); 
	} 

	void recordCommandBuffer(VkCommandBuffer commandBuffer, GraphicsPass* currentPass, uint32_t imageIndex) {
		uint32_t currentFrame = engine->currentFrame;

		engine->beginRenderPass(commandBuffer, currentPass, &renderImage, imageIndex, backgroundColour);

		if (widgetManager.showWidgets) {
			if (showWireframe) {
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *currentPass->GraphicsPipelines[currentPass->pipelineMap.at("UV")]);

				for (uint32_t i = 0; i != staticObjects.size(); i++) {
					if (staticObjects[i].isWireframeVisible) {
						engine->drawObject(commandBuffer, staticObjects[i].mesh->vertexBuffer, staticObjects[i].mesh->indexBuffer, currentPass->pipelineLayouts[currentPass->layoutMap.at("1_")], wireMat->descriptorSets[currentFrame], static_cast<uint32_t>(staticObjects[i].mesh->indices.size()));
					}
				}
			}
			widgetManager.drawWidgets(commandBuffer, currentFrame, currentPass);
		}
		
		if (tomogUI != nullptr && tomographyPlane != nullptr && tomographyPlane->isVisible) {
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *currentPass->GraphicsPipelines[currentPass->pipelineMap.at(tomogUI->renderPipeline)]);

			engine->drawObject(commandBuffer, tomographyPlane->mesh->vertexBuffer, tomographyPlane->mesh->indexBuffer, tomogUI->scannedMaterial.pipelineLayout, tomogUI->scannedMaterial.descriptorSets[currentFrame], static_cast<uint32_t>(tomographyPlane->mesh->indices.size()));
		}

		if (projector != nullptr && projector->shellMesh != nullptr) {
			Material* mat = nullptr;
			if (viewIndex == 1 && lit) {
				mat = projector->shadedProjection;
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *currentPass->GraphicsPipelines[currentPass->pipelineMap.at("Projector_BF")]);
			}
			else {
				mat = projector->flatProjection;
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *currentPass->GraphicsPipelines[currentPass->pipelineMap.at("Projector_Flat")]);
			}
			engine->drawObject(commandBuffer, projector->shellMesh->vertexBuffer, projector->shellMesh->indexBuffer, mat->pipelineLayout, mat->descriptorSets[currentFrame], static_cast<uint32_t>(projector->shellMesh->indices.size()));
			if (inProjectorView && projector->backgroundRect != nullptr) {
				mat = projector->backgroundMat;
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *currentPass->GraphicsPipelines[currentPass->pipelineMap.at("Projector_Background")]);
				engine->drawObject(commandBuffer, projector->backgroundRect->vertexBuffer, projector->backgroundRect->indexBuffer, mat->pipelineLayout, mat->descriptorSets[currentFrame], static_cast<uint32_t>(projector->backgroundRect->indices.size()));
			}
			if (projector->projectorMarker != nullptr) {
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *currentPass->GraphicsPipelines[currentPass->pipelineMap.at("W")]);
				engine->drawObject(commandBuffer, projector->projectorMarker->vertexBuffer, projector->projectorMarker->indexBuffer, wireMat->pipelineLayout, wireMat->descriptorSets[currentFrame], static_cast<uint32_t>(projector->projectorMarker->indices.size()));
			}
		}

		if (viewIndex == 1 && lit) {
			for (std::pair<VkPipeline*, std::vector<uint32_t>> elem : objectPipelines) {
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *elem.first);
				for (uint32_t i : elem.second) {
					Material* mat = staticObjects[i].mat;
					engine->drawObject(commandBuffer, staticObjects[i].mesh->vertexBuffer, staticObjects[i].mesh->indexBuffer, mat->pipelineLayout, mat->descriptorSets[currentFrame], static_cast<uint32_t>(staticObjects[i].mesh->indices.size()));
				}
			}
		}
		else {
			std::string shaderName = "Flat";
			if (viewIndex == 0) {
				shaderName = "Flat";
			}
			else if (viewIndex == 2) {
				shaderName = "W";
			}
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *currentPass->GraphicsPipelines[currentPass->pipelineMap.at(shaderName)]);
			for (uint32_t i : visibleObjects) {
				Material* mat = nullptr;
				
				switch (viewIndex) {
				case (0):
					mat = webcamMat;
					break;
				case (2):
					mat = wireMat;
					break;
				default:
					mat = staticObjects[i].unlitMat;
					break;
				}
				
				engine->drawObject(commandBuffer, staticObjects[i].mesh->vertexBuffer, staticObjects[i].mesh->indexBuffer, mat->pipelineLayout, mat->descriptorSets[currentFrame], static_cast<uint32_t>(staticObjects[i].mesh->indices.size()));
			}
		}

		vkCmdEndRenderPass(commandBuffer);

		// Post-processing can be put here

		Engine::get()->copyImageToSwapchain(commandBuffer, &renderImage, imageIndex);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw runtime_error("failed to record command buffer!");
		}
	}
};

Core* Core::coreinstance = nullptr;
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