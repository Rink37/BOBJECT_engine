#include"Bobject_Engine.h"
#include"InputManager.h"
#include"WindowsFileManager.h"
#include"CameraController.h"
#include"UIelements.h"
#include"Webcam_feeder.h"
#include"Textures.h"
#include"Materials.h"
#include"Meshes.h"
#include"StudioSession.h"
#include"Tomography.h"
#include"LoadLists.h"
#include"Remapper.h"
#include"ImageProcessor.h"
#include"GenerateNormalMap.h"

#include<chrono>

#include"include/OS_EdgeFill.h"
#include"include/BakedImages.h"

using namespace cv;
using namespace std;

std::vector<KeyManager*> KeyManager::_instances;
KeyManager keyBinds;

std::vector<MouseManager*> MouseManager::_instances;
MouseManager mouseManager;

class TextureSettings : public Widget {
public: 
	TextureSettings(LoadList* assets, LoadList* textureAssets) {
		loadList = assets;
		textureLL = textureAssets;
	}

	void setup(std::string texName, std::function<void(UIItem*)> exitFnc, std::function<void(UIItem*)> remapFnc) {
		if (isSetup) {
			return;
		}
		if (!textureLL->checkForTexture(texName)) {
			std::cout << "Texture " << texName << " does not exist in this load list" << std::endl;
			return;
		}

		imageName = texName;

		imageData cb = CLOSEBUTTON;
		Material* closeMat = newMaterial(&cb, "CloseBtn");

		imageData sb = SETTINGSBUTTON;
		Material* settingsMat = newMaterial(&sb, "SettingsBtn");

		Arrangement* mainArrangement = new Arrangement(ORIENT_VERTICAL, 1.0f, 0.0f, 0.25f, 0.8f, 0.01f);
		Arrangement* exitArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.2f, 0.01f);
		Arrangement* remapArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.2f, 0.01f);
		exitArrangement->addItem(getPtr(new spacer()));
		exitArrangement->addItem(getPtr(new Button(closeMat, exitFnc)));

		remapArrangement->addItem(getPtr(new spacer()));
		remapArrangement->addItem(getPtr(new Button(settingsMat, remapFnc)));

		Material* panelMat = loadList->replacePtr(new Material(textureLL->getTexture(texName)), "Image view");
		bool isWebcam = (texName == std::string("Webcam View"));
		imagePanel = new ImagePanel(panelMat, isWebcam);

		mainArrangement->addItem(getPtr(exitArrangement));
		mainArrangement->addItem(getPtr(imagePanel));
		if (texName != std::string("Webcam View")) {
			mainArrangement->addItem(getPtr(remapArrangement));
		}
		mainArrangement->addItem(getPtr(new spacer()));
		mainArrangement->arrangeItems();

		canvas.push_back(getPtr(mainArrangement));
		isSetup = true;
	}

	void getUVPos(glm::vec4& positions) {
		positions[0] = 2 * imagePanel->UVextentx;
		positions[1] = (imagePanel->posx) - imagePanel->UVextentx;
		positions[2] = 2 * imagePanel->extenty;
		positions[3] = (imagePanel->posy) - imagePanel->extenty;
	}

	void updateWebcamTex() {
		if (imageName == std::string("Webcam View")) {
			imagePanel->image->mat[0] = loadList->replacePtr(new Material(textureLL->getTexture("Webcam View")), "Image View");
		}
	}

	int clickIndex = 0;
private:
	LoadList* textureLL = nullptr;

	ImagePanel* imagePanel = nullptr;

	std::string imageName = "";
};

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

		std::function<void(UIItem*)> revertAR = bind(&WebcamSettings::revertAspectRatio, this, placeholders::_1);
		
		Arrangement* mainArrangement = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.4f, 0.6f, 0.01f, ARRANGE_START);

		Arrangement* endButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.05f, 0.01f);
		Arrangement* idButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.05f, 0.01f);
		Arrangement* rotationButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.05f, 0.01f);
		Arrangement* ARSettings = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.1f, 0.05f);

		webcamView = new ImagePanel(new Material(webcamTexture::get()), true);
		
		imageData finishBtnImage = FINISHBUTTON;
		Material* finishmat = newMaterial(&finishBtnImage, "FinishBtn");

		imageData sb = SETTINGSBUTTON;
		Material* settingsMat = newMaterial(&sb, "SettingsBtn");

		imageData pb = PLAYBUTTON;
		Material* forwardMat = newMaterial(&pb, "PlayBtn");

		imageData bb = BACKBUTTON;
		Material* backMat = newMaterial(&bb, "BackBtn");

		imageData rf = ROTATEFORWARD;
		Material* rotForward = newMaterial(&rf, "RotateFWBtn");

		imageData rb = ROTATEBACKWARD;
		Material* rotBackward = newMaterial(&rb, "RotateBWBtn");

		idButtons->addItem(getPtr(new Button(backMat, idDown)));
		idButtons->addItem(getPtr(new spacer));
		idButtons->addItem(getPtr(new Button(forwardMat, idUp)));
		
		rotationButtons->addItem(getPtr(new Button(rotBackward, subtractRot)));
		rotationButtons->addItem(getPtr(new spacer));
		rotationButtons->addItem(getPtr(new Button(rotForward, addRot)));

		endButtons->addItem(getPtr(new Button(settingsMat, webcamCalib)));
		endButtons->addItem(getPtr(new spacer));
		endButtons->addItem(getPtr(new Button(finishmat, finishCallback)));

		imageData tcb = TESTCHECKBOXBUTTON;
		Material* visibleMat = newMaterial(&tcb, "TestCheckBtn");

		ratioSlider = new Slider(ORIENT_HORIZONTAL, visibleMat, 0.0f, 0.0f, 1.0f, 0.1f);
		ratioSlider->setFloatCallback(bind(&WebcamSettings::updateAspectRatio, this, placeholders::_1), true);
		ratioSlider->setSlideValues(0.5f, 2.0f, webcamTexture::get()->webCam->sizeRatio);

		ARSettings->addItem(getPtr(ratioSlider));
		ARSettings->addItem(getPtr(new Button(rotBackward, revertAR)));

		ARSettings->arrangeItems();

		mainArrangement->addItem(webcamView);
		mainArrangement->addItem(getPtr(ARSettings));
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

		reload();
		update();
	}

	void indexUp(UIItem* owner) {
		webcamTexture::get()->webCam->switchWebcam(true);
		webcamTexture::get()->recreateWebcamImage();
		webcamView->image->mat[0]->cleanupDescriptor();
		webcamView->image->mat[0] = new Material(webcamTexture::get());
		ratioSlider->setSlideValues(0.5f, 2.0f, webcamTexture::get()->webCam->sizeRatio);
		reload();
		update();

		reload();
		update();
	}

	void indexDown(UIItem* owner) {
		webcamTexture::get()->webCam->switchWebcam(false);
		webcamTexture::get()->recreateWebcamImage();
		webcamView->image->mat[0]->cleanupDescriptor();
		webcamView->image->mat[0] = new Material(webcamTexture::get());
		ratioSlider->setSlideValues(0.5f, 2.0f, webcamTexture::get()->webCam->sizeRatio);
		reload();
		update();

		reload();
		update();
	}

	void revertAspectRatio(UIItem* owner) {
		std::cout << "Revert aspect ratio function called" << std::endl;
	}

	void updateAspectRatio(float newRatio) {
		webcamTexture::get()->webCam->updateAspectRatio(newRatio);
		webcamTexture::get()->recreateWebcamImage();
		webcamView->image->mat[0]->cleanupDescriptor();
		webcamView->image->mat[0] = new Material(webcamTexture::get());
		reload();
		update();

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
		session::get()->currentStudio.packWebcamSettings(webcamTexture::get()->webCam->rotationState, webcamTexture::get()->webCam->camIndex);
		session::get()->currentStudio.webcamAspectRatio = webcamTexture::get()->webCam->sizeRatio;
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
		buttons->addItem(getPtr(Renderbuttons));
		buttons->addItem(getPtr(polarSlider));
		buttons->addItem(getPtr(azimuthSlider));

		buttons->arrangeItems();
		
		canvas.push_back(getPtr(buttons));

		//font* testFont = new font();
		//TextBox* testTextBox = new TextBox(testFont, 0.0f, 0.0f, 0.3f, 0.2f, 24, ARRANGE_FILL, ARRANGE_FILL);
		//testTextBox->addText("\tThis line should be separate!\n\n\tLorem ipsum dolor sit amet, consectetur adipiscing elit. Pellentesque vestibulum aliquet ligula vel dictum. Praesent scelerisque orci at tincidunt placerat. Aliquam et blandit nulla. Nullam consequat ligula vitae massa luctus, et tincidunt felis dictum. Morbi mattis dapibus ante, vitae eleifend ipsum rutrum vitae. Proin in mauris eget metus mattis interdum vel eget nisi. Nulla porta sapien id eros malesuada laoreet. Integer et rhoncus magna, sed ullamcorper elit. Quisque ut massa ut nibh venenatis ultrices ac id tortor. Sed mattis, massa at vestibulum tincidunt, arcu diam vestibulum libero, vel lacinia tortor sapien quis sem. Proin scelerisque pharetra odio, quis congue turpis. Proin arcu leo, blandit quis ex vitae, posuere sollicitudin turpis. Duis ullamcorper sodales dui ac posuere. Pellentesque nibh felis, finibus in elit sed, iaculis fringilla est.");
		//canvas.push_back(getPtr(testTextBox));

		//DropdownMenu* testMenu = new DropdownMenu(0.0f, 0.0f, 0.3f, 0.05f, renderedMat, visibleMat, testFont);
		//testMenu->addOptions(std::vector<std::string>{"Hello", "World!", "This", "Is", "A", "Test"});
		//testMenu->setOptionIndex(1);
		
		//canvas.push_back(getPtr(testMenu));

		isSetup = true;
	}
};

class MaterialCreator : public Widget {
public:
	MaterialCreator(std::string sName, GraphicsPass* bPass, LoadList* assets, LoadList* textureAssets) {
		loadList = assets;
		textureLL = textureAssets;

		shaderName = sName;
		boundPass = bPass;
		matTemplate = new MaterialTemplate(shaderName, boundPass);
	}

	void setup(std::function<void(UIItem*)> callback) {
		if (isSetup) {
			return;
		}
		inFont = loadList->getFont();
		
		finishedCallback = callback;

		Arrangement* totalArrangement = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.25f, 0.8f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		imageData rb = RENDEREDBUTTON;
		Material* renderedMat = newMaterial(&rb, "RenderBtn");

		imageData tcb = TESTCHECKBOXBUTTON;
		Material* visibleMat = newMaterial(&tcb, "TestCheckBtn");

		int index = 0;
		int optionIndex = 0;

		std::vector<std::string> materialOptions{};
		std::vector<std::string> invalidMatOptions = {"UI", "UIGray", "UIText", "UV", "W"};
		for (auto elem : boundPass->pipelineMap) {
			if (find(invalidMatOptions.begin(), invalidMatOptions.end(), elem.first) != invalidMatOptions.end()) {
				continue;
			}
			if (elem.first == shaderName) {
				optionIndex = index;
			}
			materialOptions.push_back(elem.first);
			index++;
		}

		std::vector<std::string> existingMaterials{};
		textureLL->listMaterials(existingMaterials);
		newMaterialName = "Material" + std::to_string(existingMaterials.size() - 1);

		DropdownMenu* materialSelect = new DropdownMenu(0.0f, 0.0f, 4.0f, 1.0f, renderedMat, visibleMat, inFont);
		materialSelect->addOptions(materialOptions);
		materialSelect->setSelectCallback(std::bind(&MaterialCreator::createMatOptionsMenu, this, std::placeholders::_1));
		materialSelect->setOptionIndex(optionIndex);

		totalArrangement->addItem(getPtr(materialSelect));
		totalArrangement->arrangeItems();

		canvas.push_back(getPtr(totalArrangement));
		isSetup = true;
	}

	void setupEditMode(std::function<void(UIItem*)> callback, std::string MatName) {
		if (isSetup) {
			return;
		}
		finishedCallback = callback;

		Arrangement* totalArrangement = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.25f, 0.8f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		newMaterialName = MatName;
		std::string shaderName = textureLL->getMaterial(newMaterialName)->shaderName;

		UIItem* empty = getPtr(new spacer());
		empty->text = shaderName;

		totalArrangement->arrangeItems();

		canvas.push_back(getPtr(totalArrangement));

		std::vector<std::string> textureNames{};
		textureLL->listTexturesInMat(newMaterialName, textureNames);

		std::vector<std::string> texOptions{};
		textureLL->listTextures(texOptions);

		for (std::string texName : textureNames) {
			auto it = find(texOptions.begin(), texOptions.end(), texName);
			if (it != texOptions.end()) {
				int optionIndex = it - texOptions.begin();
				autoIndices.push_back(optionIndex);
			}
			else {
				autoIndices.push_back(-1);
			}
		}

		createMatOptionsMenu(empty);

		isSetup = true;
	}

	int clickIndex = 0;

private:
	MaterialTemplate* matTemplate = nullptr;

	std::string shaderName = "";
	GraphicsPass* boundPass = nullptr;

	std::string newMaterialName = "";

	LoadList* textureLL = nullptr;

	font* inFont = nullptr;

	std::function<void(UIItem*)> finishedCallback = nullptr;

	std::vector<int> autoIndices{};

	void createMatOptionsMenu(UIItem* owner) {
		if (canvas[0]->Items.size() > 1) {
			for (int i = 1; i != canvas[0]->Items.size(); i++) {
				canvas[0]->Items[i]->cleanup();
			}
			canvas[0]->Items.erase(canvas[0]->Items.begin() + 1, canvas[0]->Items.end());
		}

		shaderName = owner->text;
		if (matTemplate != nullptr) {
			delete matTemplate;
		}
		matTemplate = new MaterialTemplate(shaderName, boundPass);
		std::vector<std::string> availableTextures{};
		textureLL->listTextures(availableTextures);

		auto it = find(availableTextures.begin(), availableTextures.end(), std::string("Webcam View"));
		int defaultIndex = 0;
		if (it != availableTextures.end()) {
			defaultIndex = it - availableTextures.begin();
		}

		std::vector<std::string> texChannels = matTemplate->listChannels();
		it = find(texChannels.begin(), texChannels.end(), std::string("UniformBufferObject"));
		if (it != texChannels.end()) {
			texChannels.erase(it);
		}

		for (std::string channel : texChannels) {
			std::cout << channel << " ";
		}
		std::cout << std::endl;

		imageData rb = RENDEREDBUTTON;
		Material* renderedMat = newMaterial(&rb, "RenderBtn");

		imageData tcb = TESTCHECKBOXBUTTON;
		Material* visibleMat = newMaterial(&tcb, "TestCheckBtn");

		imageData fb = FINISHBUTTON;
		Material* finishedMat = newMaterial(&fb, "FinishBtn");

		std::function<void(UIItem*)> updateMatTemplate = std::bind(&MaterialCreator::setTexCallback, this, std::placeholders::_1);
		std::function<void(UIItem*)> exitCallback = std::bind(&MaterialCreator::exit, this, std::placeholders::_1);

		int i = 0;
		for (std::string channel : texChannels) {
			int index = defaultIndex;

			if (autoIndices.size() == texChannels.size()) {
				index = (autoIndices[i] > -1) ? autoIndices[i] : defaultIndex;
			}

			TextBox* channelTextBox = new TextBox(inFont, 0.0f, 0.0f, 4.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
			channelTextBox->addText(channel);
			canvas[0]->addItem(getPtr(channelTextBox));

			DropdownMenu* materialSelect = new DropdownMenu(0.0f, 0.0f, 4.0f, 1.0f, renderedMat, visibleMat, inFont);
			materialSelect->addOptions(availableTextures);
			materialSelect->setSelectCallback(updateMatTemplate);
			materialSelect->setOptionIndex(index);
			materialSelect->Name = channel;
			materialSelect->execCallback();

			canvas[0]->addItem(getPtr(materialSelect));
			i++;
		}
		canvas[0]->addItem(getPtr(new Button(finishedMat, exitCallback)));
		canvas[0]->arrangeItems();
	}

	void setTexCallback(UIItem* owner) {
		std::string channel = owner->Name;
		std::string selectedTexture = owner->text;

		matTemplate->setTexture(channel, textureLL->getTexture(selectedTexture));
	}

	void exit(UIItem* owner) {
		Material* mat = matTemplate->createMaterial();
		Material* flatMat = matTemplate->createFlatMaterial();
		textureLL->replacePtr(mat, newMaterialName);
		textureLL->replacePtr(mat, newMaterialName + "_flat");
		owner->Name = newMaterialName;
		owner->text = shaderName;
		finishedCallback(owner);
	}
};

class ObjectSettingsMenu : public Widget {
public:
	ObjectSettingsMenu(LoadList* assets, LoadList* textureAssets) {
		loadList = assets;
		textureLL = textureAssets;
	}

	void setup(StaticObject* object, std::function<void(UIItem*)> addTex, std::function<void(std::function<void(UIItem*)>)> newMatFunc, std::function<void(UIItem*)> closeMatFunc, std::function<void(UIItem*)> closeFunc, std::function<void(std::function<void(UIItem*)>, std::string)> editMatFunc) {
		if (isSetup) {
			return;
		}

		obj = object;

		OSNormName = obj->objectName + "_OSNorm";
		TSNormName = obj->objectName + "_TSNorm";

		openEditMaterialMenu = editMatFunc;
		openMaterialMenu = newMatFunc;
		closeMaterialMenu = closeMatFunc;

		finishedCallback = closeFunc;
		
		inFont = loadList->getFont();

		addTextureFunc = addTex;

		Arrangement* totalArrangement = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.25f, 0.8f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		Arrangement* materialSettingsArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 4.0f, 1.0f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		TextBox* matLabel = new TextBox(inFont, 0.0f, 0.0f, 3.0f, 1.0f, 24, ARRANGE_START, ARRANGE_CENTER);
		matLabel->addText("MATERIAL:");

		imageData sb = SETTINGSBUTTON;
		Material* settingsMat = newMaterial(&sb, "SettingsBtn");

		imageData pb = PLUSBUTTON;
		Material* plusMat = newMaterial(&pb, "PlusBtn");

		std::function<void(UIItem*)> newMatBtn = std::bind(&ObjectSettingsMenu::newMaterialMenu, this, std::placeholders::_1);
		std::function<void(UIItem*)> editMatBtn = std::bind(&ObjectSettingsMenu::editMaterialMenu, this, std::placeholders::_1);

		settingsButton = getPtr(new Button(settingsMat, editMatBtn));
		settingsButton->Name = obj->materialName;

		materialSettingsArrangement->addItem(getPtr(matLabel));
		materialSettingsArrangement->addItem(getPtr(new spacer()));
		materialSettingsArrangement->addItem(settingsButton);
		materialSettingsArrangement->addItem(getPtr(new Button(plusMat, newMatBtn)));

		totalArrangement->addItem(getPtr(materialSettingsArrangement));

		imageData rb = RENDEREDBUTTON;
		Material* renderedMat = newMaterial(&rb, "RenderBtn");
		
		imageData tcb = TESTCHECKBOXBUTTON;
		Material* visibleMat = newMaterial(&tcb, "TestCheckBtn");

		imageData fb = FINISHBUTTON;
		Material* finishMat = newMaterial(&fb, "FinishBtn");

		int index = 0;
		int optionIndex = 0;

		std::vector<std::string> existingMaterials{};
		textureLL->listMaterials(existingMaterials);

		for (std::string elem : existingMaterials) {
			if (elem == obj->materialName) {
				optionIndex = index;
				break;
			}
			index++;
		}

		DropdownMenu* materialSelect = new DropdownMenu(0.0f, 0.0f, 4.0f, 1.0f, renderedMat, visibleMat, inFont);
		materialSelect->addOptions(existingMaterials);
		materialSelect->setOptionIndex(optionIndex);
		materialSelect->setSelectCallback(std::bind(&ObjectSettingsMenu::selectMaterialCallback, this, std::placeholders::_1));

		matSelPtr = getPtr(materialSelect);

		totalArrangement->addItem(matSelPtr);
		
		Arrangement* genNormArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 5.0f, 1.0f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);
		TextBox* genNormText = new TextBox(inFont, 0.0f, 0.0f, 3.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
		genNormText->addText("Generate OS normal:");

		genNormArrangement->addItem(getPtr(genNormText));
		genNormArrangement->addItem(getPtr(new spacer()));
		genNormArrangement->addItem(getPtr(new Button(plusMat, std::bind(&ObjectSettingsMenu::createNormalMap, this, std::placeholders::_1))));
		
		if (textureLL->findTexPtr(OSNormName) || textureLL->findTexPtr(TSNormName)) {
			Arrangement* switchNormArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 5.0f, 1.0f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);
			DropdownMenu* normSelect = new DropdownMenu(0.0f, 0.0f, 4.0f, 1.0f, renderedMat, visibleMat, inFont);
			std::vector<std::string> normNames{};
			if (textureLL->findTexPtr(OSNormName)) {
				normNames.push_back(OSNormName);
			}
			if (textureLL->findTexPtr(TSNormName)) {
				normNames.push_back(TSNormName);
			}
			normSelect->addOptions(normNames);
			normSelect->setOptionIndex(0);
			normSelect->setSelectCallback(std::bind(&ObjectSettingsMenu::selectNormType, this, std::placeholders::_1));

			switchNormArrangement->addItem(getPtr(normSelect));
			switchNormArrangement->addItem(getPtr(new Button(settingsMat, std::bind(&ObjectSettingsMenu::changeNormalMapType, this, std::placeholders::_1))));

			totalArrangement->addItem(getPtr(switchNormArrangement));
		}

		totalArrangement->addItem(getPtr(genNormArrangement));

		totalArrangement->addItem(getPtr(new Button(finishMat, finishedCallback)));
		totalArrangement->arrangeItems();

		canvas.push_back(getPtr(totalArrangement));
		isSetup = true;
	}

	int clickIndex = 0;

private:
	StaticObject* obj = nullptr;

	LoadList* textureLL = nullptr;

	UIItem* matSelPtr = nullptr;

	font* inFont = nullptr;

	UIItem* settingsButton = nullptr;

	std::function<void(UIItem*)> finishedCallback = nullptr;

	std::function<void(std::function<void(UIItem*)>)> openMaterialMenu = nullptr;
	std::function<void(std::function<void(UIItem*)>, std::string)> openEditMaterialMenu = nullptr;
	std::function<void(UIItem*)> closeMaterialMenu = nullptr;

	std::function<void(UIItem*)> addTextureFunc = nullptr;

	std::string OSNormName = "";
	std::string TSNormName = "";

	int normalType = 0;

	void selectNormType(UIItem* owner) {
		if (owner->text == OSNormName) {
			normalType = 0;
		}
		else if (owner->text == TSNormName) {
			normalType = 1;
		}
	}
	
	void newMaterialMenu(UIItem* owner) {
		std::function<void(UIItem*)> closeFnc = std::bind(&ObjectSettingsMenu::exitMaterialMenu, this, std::placeholders::_1);
		if (openMaterialMenu != nullptr) {
			openMaterialMenu(closeFnc);
		}
	}

	void editMaterialMenu(UIItem* owner) {
		if (owner->Name == std::string("Webcam Material")) {
			return;
		}
		std::function<void(UIItem*)> closeFnc  = std::bind(&ObjectSettingsMenu::exitMaterialMenu, this, std::placeholders::_1);
		if (openEditMaterialMenu != nullptr) {
			openEditMaterialMenu(closeFnc, owner->Name);
		}
	}

	void exitMaterialMenu(UIItem* owner) {
		obj->mat = textureLL->getMaterial(owner->Name);
		obj->unlitMat = textureLL->getMaterial(owner->Name + "_flat");
		obj->shaderName = owner->text;
		obj->materialName = owner->Name;
		matSelPtr->addOption(obj->materialName);
		if (closeMaterialMenu != nullptr) {
			closeMaterialMenu(owner);
		}
	}

	void selectMaterialCallback(UIItem* owner) {
		obj->mat = textureLL->getMaterial(owner->text);
		obj->shaderName = obj->mat->shaderName;
		obj->materialName = owner->text;
		settingsButton->Name = obj->materialName;
	}

	void createNormalMap(UIItem* owner) {

		NormalGen generator(textureLL);
		generator.setupOSExtractor();
		VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
		commandBuffer = generator.drawOSMap(commandBuffer, obj->mesh);
		Engine::get()->endSingleTimeCommands(commandBuffer);

		Texture* EdgeFillImg = generator.objectSpaceMap.colour->copyTexture(generator.objectSpaceMap.colour->textureFormat, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1);
		filter OS_EdgeFill(std::vector<Texture*>({ EdgeFillImg }), new OS_EDGEFILLSHADER, VK_FORMAT_R8G8B8A8_UNORM);
		OS_EdgeFill.filterImage();
		OS_EdgeFill.filterTarget[0]->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		OS_EdgeFill.filterTarget[0]->textureLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		Texture* OSNormTex = OS_EdgeFill.filterTarget[0]->copyImage(VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_TILING_LINEAR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1);
		
		OSNormTex->textureImageView = OSNormTex->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

		textureLL->replacePtr(OSNormTex, OSNormName);

		if (addTextureFunc != nullptr) {
			owner->Name = OSNormName;
			addTextureFunc(owner);
		}

		generator.cleanupGenOS();
		EdgeFillImg->cleanup();
		OS_EdgeFill.cleanup();
	}

	void changeNormalMapType(UIItem* owner) {
		std::string normalName = "";
		std::string outNormalName = "";

		switch (normalType) {
		case (0):
			normalName = OSNormName;
			outNormalName = TSNormName;
			break;
		case (1):
			normalName = TSNormName;
			outNormalName = OSNormName;
			break;
		default:
			break;
		}

		Texture* norm = textureLL->getTexture(normalName);
		Texture* res = nullptr;

		VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
		NormalGen generator(loadList);
		switch (normalType) {
		case (0):
			generator.copyOSImage(norm);
			generator.setupTSConverter();
			commandBuffer = generator.convertOStoTS(commandBuffer, obj->mesh);
			res = generator.tangentSpaceMap.colour;
			break;
		case (1):
			generator.copyTSImage(norm);
			generator.setupOSConverter();
			commandBuffer = generator.convertTStoOS(commandBuffer, obj->mesh);
			res = generator.objectSpaceMap.colour;
			break;
		default:
			Engine::get()->endSingleTimeCommands(commandBuffer);
			return;
		}
		Engine::get()->endSingleTimeCommands(commandBuffer);

		Texture* outNorm = res->copyImage(VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_TILING_LINEAR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1);

		outNorm->textureImageView = outNorm->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

		if (addTextureFunc != nullptr && !textureLL->checkForTexture(outNormalName)) {
			owner->Name = outNormalName;
			addTextureFunc(owner);
		}
		
		textureLL->replacePtr(outNorm, outNormalName);

		switch (normalType) {
		case (0):
			generator.cleanupTS();
			break;
		case (1):
			generator.cleanupOS();
			break;
		default:
			return;
		}
	}
};

class ObjectMenu : public Widget {
public:
	ObjectMenu(LoadList* assets) {
		loadList = assets;
	}

	void setup(std::function<void(UIItem*)> loadObjectFunct) {
		if (isSetup) {
			return;
		}

		imageData ub = UNRENDEREDBUTTON;
		invisibleMat = newMaterial(&ub, "UnrenderedBtn");

		imageData tcb = TESTCHECKBOXBUTTON;
		visibleMat = newMaterial(&tcb, "CheckboxBtn");

		imageData wb = WIREFRAMEBUTTON;
		wireframeMat = newMaterial(&wb, "WireframeBtn");

		imageData sb = SETTINGSBUTTON;
		settingsMat = newMaterial(&sb, "SettingsBtn");

		Arrangement* textArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.2f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		imageData lb = OPENBUTTON;
		Material* loadMat = newMaterial(&lb, "OpenBtn");

		font* newFont = loadList->getFont();

		TextBox* menuText = new TextBox(newFont, 0.0f, 0.0f, 3.0f, 1.0f, 24, ARRANGE_START, ARRANGE_CENTER);
		menuText->addText("MESHES");
		textArrangement->addItem(getPtr(menuText));
		textArrangement->addItem(getPtr(new spacer()));
		textArrangement->addItem(getPtr(new Button(loadMat, loadObjectFunct)));

		ObjectButtons = getPtr(new Arrangement(ORIENT_VERTICAL, -0.9f, 0.3125f, 0.2f, 0.4375f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS));

		ObjectButtons->addItem(getPtr(textArrangement));
		ObjectButtons->arrangeItems();
		
		canvas.push_back(ObjectButtons);
		
		isSetup = true;
	}

	void addObject(std::function<void(UIItem*)> toggleFunction, std::function<void(UIItem*)> wireframeToggle, std::function<void(UIItem*)> optionsMenu, std::string nameString = "Object Name") {
		ObjectButtons->arrangeItems();

		Arrangement* objButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.15f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		Checkbox* objectButton = new Checkbox(visibleMat, invisibleMat, toggleFunction);
		objectButton->Name = "Object button " + std::to_string(ObjectButtons->Items.size() - 1);

		Checkbox* objWireframeButton = new Checkbox(wireframeMat, invisibleMat, wireframeToggle);
		objWireframeButton->Name = objectButton->Name;

		ObjectMap.insert({ objectButton->Name, ObjectButtons->Items.size() - 1 });

		font* objectFont = loadList->getFont();
		TextBox* objectName = new TextBox(objectFont, 0.0f, 0.0f, 3.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
		objectName->addText(nameString);

		Button* newButton = new Button(settingsMat, optionsMenu);
		newButton->Name = to_string(ObjectButtons->Items.size() - 1);

		objButtons->addItem(getPtr(objectName));
		objButtons->addItem(getPtr(new spacer()));
		objButtons->addItem(getPtr(newButton));
		objButtons->addItem(getPtr(objectButton));
		objButtons->addItem(getPtr(objWireframeButton));
		objButtons->arrangeItems();

		ObjectButtons->addItem(getPtr(objButtons));
		ObjectButtons->arrangeItems();
	}

	void clearObjects() {
		bool isFirstItem = true;
		for (UIItem* item : ObjectButtons->Items) {
			if (!isFirstItem) {
				isFirstItem = true;
				continue;
			}
			item->image->cleanup();
		}
		ObjectButtons->Items.erase(ObjectButtons->Items.begin()+1, ObjectButtons->Items.end());
		ObjectMap.clear();
		ObjectButtons->arrangeItems();
	}

	map<string, int> ObjectMap = {};
private:
	UIItem* ObjectButtons = nullptr;

	Material* visibleMat = nullptr;
	Material* invisibleMat = nullptr;
	Material* wireframeMat = nullptr;
	Material* settingsMat = nullptr;

};

class TextureLoadMenu : public Widget {
public:
	TextureLoadMenu(LoadList* assets, LoadList* texLL) {
		loadList = assets;
		textureLL = texLL;
	}

	void setup(std::function<void(UIItem*)> update, std::function<void(UIItem*)> exit) {
		if (isSetup) {
			return;
		}

		updateTextureMenu = update;
		exitCallback = exit;

		fileName = winFile::OpenFileDialog();
		if (fileName == string("fail")) {
			return; // We will need to check if this menu has been setup after the setup function is called otherwise we will have some draw errors
		}
		textureName = fileName;
		string del = "\\";
		auto pos = textureName.find(del);
		while (pos != string::npos) {
			textureName.erase(0, pos + del.length());
			pos = textureName.find(del);
		}
		del = ".";

		Arrangement* mainArrangement = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.25f, 0.8f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		font* newFont = loadList->getFont();
		TextBox* TexLoadLabel = new TextBox(newFont, 0.0f, 0.0f, 5.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
		TexLoadLabel->addText(std::string("Loading ") + textureName);

		mainArrangement->addItem(getPtr(TexLoadLabel));

		std::vector<std::string> loadOptions{ "Colour", "Normalized vector" };

		imageData rb = RENDEREDBUTTON;
		Material* renderedMat = newMaterial(&rb, "RenderBtn");

		imageData tcb = TESTCHECKBOXBUTTON;
		Material* visibleMat = newMaterial(&tcb, "TestCheckBtn");

		imageData fb = FINISHBUTTON;
		Material* finishedMat = newMaterial(&fb, "FinishBtn");

		DropdownMenu* loadOption = new DropdownMenu(0.0f, 0.0f, 5.0f, 1.0f, renderedMat, visibleMat, newFont);
		loadOption->addOptions(loadOptions);
		loadOption->setSelectCallback(std::bind(&TextureLoadMenu::setOptionCallback, this, std::placeholders::_1));
		loadOption->setOptionIndex(0);
		loadOption->execCallback();

		mainArrangement->addItem(getPtr(loadOption));

		Arrangement* finishButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 5.0f, 1.0f, 0.0f, ARRANGE_START, SCALE_BY_DIMENSIONS);
		finishButtons->addItem(getPtr(new spacer()));
		finishButtons->addItem(getPtr(new Button(finishedMat, std::bind(&TextureLoadMenu::finishCallback, this, std::placeholders::_1))));

		mainArrangement->addItem(getPtr(finishButtons));

		mainArrangement->arrangeItems();

		pos = textureName.find(del);
		textureName = textureName.substr(0, pos);

		canvas.push_back(getPtr(mainArrangement));

		isSetup = true;
	}

	int clickIndex = 0;
private:
	LoadList* textureLL = nullptr;

	std::string textureName = "";
	std::string fileName = "";

	bool isNormalized = false;

	std::function<void(UIItem*)> updateTextureMenu = nullptr;
	std::function<void(UIItem*)> exitCallback = nullptr; 

	void setOptionCallback(UIItem* owner) {
		if (owner->text == std::string("Colour")) {
			isNormalized = false;
		}
		else {
			isNormalized = true;
		}
	}

	void finishCallback(UIItem* owner) {
		imageTexture* loadedTexture = nullptr;
		if (isNormalized) {
			loadedTexture = new imageTexture(fileName, VK_FORMAT_R8G8B8A8_UNORM);
		}
		else {
			loadedTexture = new imageTexture(fileName, VK_FORMAT_R8G8B8A8_SRGB);
		}
		textureLL->getPtr(loadedTexture, textureName);
		owner->Name = textureName;
		if (updateTextureMenu != nullptr) {
			updateTextureMenu(owner);
		}
		if (exitCallback != nullptr) {
			exitCallback(nullptr);
		}
	}
};

class TextureMenu : public Widget {
public:
	TextureMenu(LoadList* assets, LoadList* textureLL) {
		loadList = assets;
		textureLoadList = textureLL;
	}

	void setup(std::function<void(UIItem*)> modCallback, std::function<void(std::function<void(UIItem*)>)> loadCallback) {
		if (isSetup) {
			return;
		}

		objectFont = loadList->getFont();

		modifyCallback = modCallback;
		loadTextureCallback = loadCallback;

		std::function<void(UIItem*)> loadTexFunct = std::bind(&TextureMenu::loadTexture, this, std::placeholders::_1);

		imageData ub = UNRENDEREDBUTTON;
		Material* invisibleMat = newMaterial(&ub, "UnrenderedBtn");

		imageData tcb = TESTCHECKBOXBUTTON;
		Material* visibleMat = newMaterial(&tcb, "CheckboxBtn");

		imageData wb = WIREFRAMEBUTTON;
		Material* wireframeMat = newMaterial(&wb, "WireframeBtn");

		imageData sb = SETTINGSBUTTON;
		settingsMat = newMaterial(&sb, "SettingsBtn");

		imageData svm = SAVEBUTTON;
		saveMat = newMaterial(&svm, "SaveBtn");

		saveCallback = std::bind(&TextureMenu::saveTexture, this, std::placeholders::_1);

		Arrangement* textArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.2f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		imageData lb = OPENBUTTON;
		Material* loadMat = newMaterial(&lb, "OpenBtn");

		TextBox* menuText = new TextBox(objectFont, 0.0f, 0.0f, 4.0f, 1.0f, 24, ARRANGE_START, ARRANGE_CENTER);
		menuText->addText("TEXTURES");
		textArrangement->addItem(getPtr(menuText));
		textArrangement->addItem(getPtr(new spacer()));
		textArrangement->addItem(getPtr(new Button(loadMat, loadTexFunct)));

		TextureButtons = getPtr(new Arrangement(ORIENT_VERTICAL, -0.9f, -0.5625f, 0.2f, 0.4375f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS));

		TextureButtons->addItem(getPtr(textArrangement));
		
		Arrangement* objButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.15f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		std::string textureName = "Webcam View";

		//textureLoadList->getPtr(webcamTexture::get(), textureName);
		
		TextBox* objectName = new TextBox(objectFont, 0.0f, 0.0f, 5.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
		objectName->addText(textureName);

		Button* saveButton = new Button(saveMat, saveCallback);
		saveButton->Name = textureName;

		Button* settingsButton = new Button(settingsMat, modifyCallback);
		settingsButton->Name = textureName;

		objButtons->addItem(getPtr(objectName));
		objButtons->addItem(getPtr(new spacer()));
		objButtons->addItem(getPtr(saveButton));
		objButtons->addItem(getPtr(settingsButton));
		objButtons->arrangeItems();

		TextureButtons->addItem(getPtr(objButtons));
		
		TextureButtons->arrangeItems();

		canvas.push_back(TextureButtons);

		isSetup = true;
	}

	std::function<void(UIItem*)> getAddTexCallback() {
		return std::bind(&TextureMenu::addTexture, this, std::placeholders::_1);
	}

private:
	std::function<void(std::function<void(UIItem*)>)> loadTextureCallback = nullptr;

	void loadTexture(UIItem* owner) {
		if (loadTextureCallback != nullptr) {
			loadTextureCallback(std::bind(&TextureMenu::addTexture, this, std::placeholders::_1));
		}
	}

	void addTexture(UIItem* owner) {
		std::string textureName = owner->Name;
		
		Arrangement* objButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.15f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		TextBox* objectName = new TextBox(objectFont, 0.0f, 0.0f, 5.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
		objectName->addText(textureName);

		Button* settingsButton = new Button(settingsMat, modifyCallback);
		settingsButton->Name = textureName;

		Button* saveButton = new Button(saveMat, saveCallback);
		saveButton->Name = textureName;

		objButtons->addItem(getPtr(objectName));
		objButtons->addItem(getPtr(new spacer()));
		objButtons->addItem(getPtr(saveButton));
		objButtons->addItem(getPtr(settingsButton));
		objButtons->arrangeItems();

		TextureButtons->addItem(getPtr(objButtons));
		TextureButtons->arrangeItems();
	}

	void saveTexture(UIItem* owner) {
		std::string textureName = owner->Name;

		Texture* texToSave = textureLoadList->getTexture(textureName);
		texToSave->getCVMat();
		string saveName = winFile::SaveFileDialog();
		if (saveName != string("fail")) {
			imwrite(saveName, texToSave->texMat);
		}
		texToSave->destroyCVMat();
	}

	UIItem* TextureButtons = nullptr;
	LoadList* textureLoadList = nullptr;

	font* objectFont = nullptr;

	Material* settingsMat = nullptr;
	Material* saveMat = nullptr;

	std::function<void(UIItem*)> modifyCallback = nullptr;
	std::function<void(UIItem*)> saveCallback = nullptr;
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

		Arrangement* Videobuttons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 1.0f, 0.2f, 0.05f, 0.01f, ARRANGE_CENTER);

		Videobuttons->addItem(getPtr(new Button(webcamMat)));
		Videobuttons->addItem(getPtr(new Checkbox(playMat, pauseMat, toggleWebcamFunct)));
		Videobuttons->addItem(getPtr(new Button(settingsMat, openSettings)));
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
};

class Application {
public:
	void run() {
		engine->initWindow("BOBERT_TradPainter");
		engine->initVulkan();
		keyBinds.initCallbacks(engine->window);
		mouseManager.initCallbacks(engine->window);
		glfwSetScrollCallback(engine->window, camera.scrollCallback);
		webcamTexture::get()->setup();
		createCanvas();
		if (webcamTexture::get()->webCam != nullptr) {
			webcamTexture::get()->webCam->loadFilter();
		}
		TextureElements.getPtr(webcamTexture::get(), "Webcam View");
		//std::function<void()> tomogFunct = bind(&Application::toggleTomogMenu, this);
		std::function<void()> colourChange = bind(&Application::colourChangeTest, this);
		std::function<void()> FPSTrack = bind(&Application::startFPSTrack, this);
		//std::function<void()> drawUpdate = bind(&Application::updateDrawVariables, this);
		keyBinds.addBinding(GLFW_KEY_1, colourChange, PRESS_EVENT);
		//keyBinds.addBinding(GLFW_KEY_T, tomogFunct, PRESS_EVENT);
		keyBinds.addBinding(GLFW_KEY_F, FPSTrack, PRESS_EVENT);
		webcamTexture::get()->webCam->shouldUpdate = false;
		webcamMenu.canvas[0]->Items[1]->activestate = false;
		webcamMenu.canvas[0]->Items[1]->image->matidx = 1;

		Engine::get()->createRenderPass(renderGP.renderPass, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		renderImage = Engine::get()->createDrawImage(Engine::get()->swapChainExtent.width, Engine::get()->swapChainExtent.height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, renderGP.renderPass);
		Engine::get()->createGraphicsPipelines(renderGP);

		webcamMat = TextureElements.getPtr(new Material(webcamTexture::get()) , "Webcam Material");
		MaterialTemplate wireMatTemplate(std::string("W"), &renderGP);
		wireMatTemplate.listChannels();
		wireMat = UIElements.getPtr(wireMatTemplate.createMaterial(), "Wire Material");

		currentPass = &renderGP;

		updateColourScheme();
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
	Tomographer tomographer;

	TomographyMenu tomogUI = TomographyMenu(&UIElements);
	SaveMenu saveMenu = SaveMenu(&UIElements);
	WebcamMenu webcamMenu = WebcamMenu(&UIElements);
	RenderMenu renderMenu = RenderMenu(&UIElements);
	ObjectMenu objectMenu = ObjectMenu(&UIElements);
	TextureMenu textureMenu = TextureMenu(&UIElements, &TextureElements);
	TextureSettings textureSettings = TextureSettings(&UIElements, &TextureElements);
	RemapUI remapMenu = RemapUI(&UIElements, &TextureElements);
	WebcamSettings webSets = WebcamSettings(&UIElements);
	MaterialCreator* mc = nullptr; 
	ObjectSettingsMenu osm = ObjectSettingsMenu(&UIElements, &TextureElements);
	RemapTexSelector rts = RemapTexSelector(&UIElements, &TextureElements);
	TextureLoadMenu tlm = TextureLoadMenu(&UIElements, &TextureElements);

	Material* webcamMat = nullptr;
	Material* wireMat = nullptr;

	vector<Widget*> widgets;
	vector<Widget*> allWidgets = { &tomogUI, &saveMenu, &webcamMenu, &renderMenu, &objectMenu, &textureMenu, &textureSettings, &remapMenu, &webSets, mc, &osm, &rts, &tlm };

	drawImage renderImage;
	GraphicsPass renderGP;
	GraphicsPass* currentPass = nullptr;

	StaticObject* currentObject = nullptr;

	bool use_sConst = false;

	bool mouseDown = false;
	bool tomogActive = false;

	bool showWireframe = true;

	bool inWebSettings = false;

	vector<StaticObject> staticObjects = {};
	PlaneObject* tomographyPlane = nullptr;
	std::map<std::string, std::vector<uint32_t>> objectPipelines{};
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

	void colourChangeTest() {
		primaryColour = glm::vec3(0.0f, 0.13f, 0.27f); 
		secondaryColour = glm::vec3(0.0f, 0.55f, 0.32f);
		tertiaryColour = glm::vec3(0.0f, 0.39f, 0.31f);
		backgroundColour = glm::vec3(0.0f, 0.55f, 0.32f);
		updateColourScheme();
	}

	void updateVisibleObjects() {
		visibleObjects.clear();
		objectPipelines.clear();
		for (uint32_t i = 0; i != staticObjects.size(); i++) {
			if (staticObjects[i].isVisible) {
				visibleObjects.push_back(i);
				if (objectPipelines.count(staticObjects[i].shaderName) == 0) {
					objectPipelines.insert({ staticObjects[i].shaderName, std::vector<uint32_t>{i} });
				}
				else {
					objectPipelines.at(staticObjects[i].shaderName).push_back(i);
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

		webSets.setup(finishSelf, updateTexView);
		if (!webSets.isSetup) {
			return;
		}
		webSets.clickIndex = mouseManager.addClickListener(webSets.getClickCallback());
		webSets.posIndex = mouseManager.addPositionListener(webSets.getPosCallback());

		widgets.push_back(&webSets);

		sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });

		inWebSettings = true;
		webcamMenu.canvas[0]->Items[1]->image->matidx = 0;
		webcamMenu.canvas[0]->Items[1]->activestate = true;
	}

	void finishWebSettings(UIItem* owner) {
		vkDeviceWaitIdle(Engine::get()->device);
		webSets.cleanup();

		mouseManager.removeClickListener(webSets.clickIndex);
		mouseManager.removePositionListener(webSets.posIndex);

		widgets.erase(find(widgets.begin(), widgets.end(), &webSets));

		sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });
		
		inWebSettings = false;
	}

	void createRemapTexSelector(UIItem* owner) {
		std::function<void(std::string, std::string)> continueCallback = std::bind(&Application::createRemapper, this, std::placeholders::_1, std::placeholders::_2);
		
		rts.setup(owner->Name, continueCallback);
		rts.clickIndex = mouseManager.addClickListener(rts.getClickCallback());

		widgets.push_back(&rts);
		sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });
	}

	void createRemapper(std::string refTexture, std::string targetTexture) {
		std::function<void(UIItem*)> destroySelf = std::bind(&Application::destroyRemapper, this, std::placeholders::_1);
		std::function<void(UIItem*)> finishSelf = std::bind(&Application::finishRemapper, this, std::placeholders::_1);

		vkDeviceWaitIdle(Engine::get()->device);
		rts.cleanup();
		widgets.erase(find(widgets.begin(), widgets.end(), &rts));

		remapMenu.setup(refTexture, targetTexture, destroySelf, finishSelf); 
		if (!remapMenu.isSetup) {
			return;
		}
		remapMenu.clickIndex = mouseManager.addClickListener(remapMenu.getClickCallback());
		remapMenu.posIndex = mouseManager.addPositionListener(remapMenu.getPosCallback());
		
		widgets.push_back(&remapMenu);

		sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });
	}

	void destroyRemapper(UIItem* owner) {
		vkDeviceWaitIdle(Engine::get()->device);

		remapMenu.remapper->baseTarget->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		remapMenu.remapper->baseTarget->textureLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		TextureElements.replacePtr(remapMenu.remapper->baseTarget->copyTexture(), remapMenu.targetTexName);

		remapMenu.cleanup();

		mouseManager.removeClickListener(remapMenu.clickIndex);
		mouseManager.removePositionListener(remapMenu.posIndex);

		widgets.erase(find(widgets.begin(), widgets.end(), &remapMenu));

		sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });
	}

	void finishRemapper(UIItem* owner) {
		TextureElements.replacePtr(remapMenu.remapper->filteredTarget->copyTexture(), remapMenu.targetTexName);

		remapMenu.cleanup();
		mouseManager.removeClickListener(remapMenu.clickIndex);
		mouseManager.removePositionListener(remapMenu.posIndex);

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
		//surfaceMenu.removeNormalMenu(owner);
		//sConst->clearSurface();
		//surfaceMenu.resetDiffuseTog(false);
		//if (sConst->normalAvailable) {
		//	surfaceMenu.resetNormalTog(false);
		//}
		//sConst->normalAvailable = false;
		//surfaceMenu.setDiffuse(sConst->currentDiffuse());

		//if (sConst->alphaClipEnabled) {
		//	sConst->renderPipeline = "AC_BF";
		//}
		//else {
		//	sConst->renderPipeline = "BF";
		//}
		//sConst->updateSurfaceMat();
	}

	void createTexLoadMenu(std::function<void(UIItem*)> updateTexMenu) {
		std::function<void(UIItem*)> exitCallback = std::bind(&Application::exitTexLoadMenu, this, std::placeholders::_1);
		
		tlm.setup(updateTexMenu, exitCallback);
		if (!tlm.isSetup) {
			return;
		}
		tlm.clickIndex = mouseManager.addClickListener(tlm.getClickCallback());
		widgets.push_back(&tlm);
	}

	void exitTexLoadMenu(UIItem* owner) {
		vkDeviceWaitIdle(Engine::get()->device);
		tlm.cleanup();
		mouseManager.removeClickListener(tlm.clickIndex);

		widgets.erase(find(widgets.begin(), widgets.end(), &tlm));
	}

	void openTextureSettingsMenu(UIItem* owner) {
		textureSettings.setup(owner->Name, std::bind(&Application::closeTextureSettingsMenu, this, std::placeholders::_1), std::bind(&Application::createRemapTexSelector, this, std::placeholders::_1));

		textureSettings.clickIndex = mouseManager.addClickListener(textureSettings.getClickCallback());

		widgets.push_back(&textureSettings);
	}

	void updateTextureSettingsMenu() {
		vkDeviceWaitIdle(Engine::get()->device);
		TextureElements.getMaterial("Webcam Material")->cleanupDescriptor();
		TextureElements.getMaterial("Webcam Material")->init(webcamTexture::get());
		textureSettings.updateWebcamTex();
		textureSettings.update();
	}

	void closeTextureSettingsMenu(UIItem* owner) {
		if (!textureSettings.isSetup){
			return;
		}
		vkDeviceWaitIdle(Engine::get()->device);
		textureSettings.cleanup();
		mouseManager.removeClickListener(textureSettings.clickIndex);

		widgets.erase(find(widgets.begin(), widgets.end(), &textureSettings));
	}

	void openObjectSettingsMenu(UIItem* owner) {
		//StaticObject* activeObject = &staticObjects[stoi(owner->Name)];
		std::function<void(std::function<void(UIItem*)>, std::string)> openEditMenu = std::bind(&Application::openEditSettingsMenu, this, std::placeholders::_1, std::placeholders::_2);
		//if (osm == nullptr) {
		//	osm.setup(&staticObjects[stoi(owner->Name)], &UIElements, &TextureElements, std::bind(&Application::openSettingsMenu, this, std::placeholders::_1), std::bind(&Application::closeSettingsMenu, this, std::placeholders::_1), std::bind(&Application::closeObjectSettingsMenu, this, std::placeholders::_1), openEditMenu);
		//}
		
		osm.setup(&staticObjects[stoi(owner->Name)], textureMenu.getAddTexCallback(), std::bind(&Application::openSettingsMenu, this, std::placeholders::_1), std::bind(&Application::closeSettingsMenu, this, std::placeholders::_1), std::bind(&Application::closeObjectSettingsMenu, this, std::placeholders::_1), openEditMenu);
		osm.clickIndex = mouseManager.addClickListener(osm.getClickCallback());
		widgets.push_back(&osm);
	}

	void closeObjectSettingsMenu(UIItem* owner) {
		vkDeviceWaitIdle(Engine::get()->device);
		osm.cleanup();
		mouseManager.removeClickListener(osm.clickIndex);

		widgets.erase(find(widgets.begin(), widgets.end(), &osm));

		//sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });
	}

	void openSettingsMenu(std::function<void(UIItem*)> closeFnc) {
		if (mc == nullptr) {
			mc = new MaterialCreator("BF", currentPass, &UIElements, &TextureElements);
		}
		osm.hide();
		mc->setup(closeFnc);
		mc->clickIndex = mouseManager.addClickListener(mc->getClickCallback());
		widgets.push_back(mc);
	}

	void openEditSettingsMenu(std::function<void(UIItem*)> closeFnc, std::string matName) {
		if (mc == nullptr) {
			mc = new MaterialCreator("BF", currentPass, &UIElements, &TextureElements);
		}
		osm.hide();
		mc->setupEditMode(closeFnc, matName);
		mc->clickIndex = mouseManager.addClickListener(mc->getClickCallback());
		widgets.push_back(mc);
	}

	void closeSettingsMenu(UIItem* owner) {

		vkDeviceWaitIdle(Engine::get()->device);
		mc->cleanup();
		mouseManager.removeClickListener(mc->clickIndex);

		osm.show();

		updateVisibleObjects();

		widgets.erase(find(widgets.begin(), widgets.end(), mc));

		sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });

		mc = nullptr;
		use_sConst = false;
	}
	
	void loadSave(UIItem* owner) {

		string saveLocation;
		saveLocation = winFile::OpenFileDialog();
		if (saveLocation == "fail") {
			return;
		}
		newSession(owner);
		session::get()->loadStudio(saveLocation);

		uint8_t webcamRot = 0;
		uint8_t webcamIndex = 0;

		session::get()->currentStudio.unpackWebcamSettings(webcamRot, webcamIndex);

		for (string path : session::get()->currentStudio.modelPaths) {
			StaticObject newObject(path);
			string objectName = path;
			string del = "\\";
			auto pos = objectName.find(del);
			while (pos != string::npos) {
				objectName.erase(0, pos + del.length());
				pos = objectName.find(del);
			}
			del = ".";
			pos = objectName.find(del);
			objectName = objectName.substr(0, pos);
			newObject.objectName = objectName;

			std::function<void(UIItem*)> visibleFunction = std::bind(&Application::setObjectVisibility, this, placeholders::_1);
			std::function<void(UIItem*)> wireFunction = std::bind(&Application::setObjectWireframe, this, placeholders::_1);
			std::function<void(UIItem*)> optionsFunction = std::bind(&Application::openObjectSettingsMenu, this, placeholders::_1);

			objectMenu.addObject(visibleFunction, wireFunction, optionsFunction, objectName);

			newObject.isVisible = true;
			newObject.setMat(TextureElements.getMaterial("Webcam Material"), "BF");

			staticObjects.push_back(newObject);
		}
		if (session::get()->currentStudio.diffusePath != "None") {
			imageTexture* loadedTexture = new imageTexture(session::get()->currentStudio.diffusePath, VK_FORMAT_R8G8B8A8_SRGB);

			//sConst->diffuseIdx = 1;
			//surfaceMenu.setDiffuse(sConst->currentDiffuse());
			//surfaceMenu.resetDiffuseTog(true);
		}
		if (session::get()->currentStudio.OSPath != "None") {
			imageTexture* loadedTexture = new imageTexture(session::get()->currentStudio.OSPath, VK_FORMAT_R8G8B8A8_UNORM);
			//if (!sConst->normalAvailable) {
			//	surfaceMenu.createNormalMenu(new UIItem);
			//}
			//sConst->normalType = 0;
			//sConst->loadNormal(loadedTexture);
			//surfaceMenu.setNormal(sConst->currentNormal());
			//surfaceMenu.resetNormalTog(true);
			//surfaceMenu.toggleNormalState(true);
		}
		if (session::get()->currentStudio.TSPath != "None") {
			imageTexture* loadedTexture = new imageTexture(session::get()->currentStudio.TSPath, VK_FORMAT_R8G8B8A8_UNORM);
			//if (!sConst->normalAvailable) {
			//	surfaceMenu.createNormalMenu(new UIItem);
			//}
			//sConst->normalType = 1;
			//sConst->loadNormal(loadedTexture);
			//sConst->TSmatching = true;
			//surfaceMenu.setNormal(sConst->currentNormal());
			//surfaceMenu.resetNormalTog(true);
			//surfaceMenu.toggleNormalState(false);
		}
		if (webcamIndex != webcamTexture::get()->webCam->camIndex) {
			webcamTexture::get()->webCam->switchWebcam(webcamIndex);
		}
		webcamTexture::get()->webCam->setRotation(webcamRot);
		webcamTexture::get()->webCam->updateAspectRatio(session::get()->currentStudio.webcamAspectRatio);
		webcamTexture::get()->recreateWebcamImage();
		webcamTexture::get()->interruptFrameUpdate();
		webcamTexture::get()->webCam->loadFilter();
		webcamTexture::get()->startFrameUpdate();

		//reloadWebcamTex();

		//reloadWebcamTex();

		//sConst->updateSurfaceMat();

		updateVisibleObjects();
	}
	
	void createCanvas() {

		std::function<void(UIItem*)> pipelinefunction = std::bind(&Application::setPipelineIndex, this, placeholders::_1);
		std::function<void(UIItem*)> lightingFunction = std::bind(&Application::toggleLighting, this, placeholders::_1);
		std::function<void(UIItem*)> loadObjectFunct = std::bind(&Application::buttonLoadStaticObject, this, placeholders::_1);
		std::function<void(UIItem*)> loadSessionFunc = std::bind(&Application::loadSave, this, placeholders::_1);
		std::function<void(UIItem*)> newSessionFunc = std::bind(&Application::newSession, this, placeholders::_1);
		std::function<void(UIItem*)> webcamSettings = std::bind(&Application::createWebSettings, this, placeholders::_1);
		std::function<void(UIItem*)> texModify = std::bind(&Application::openTextureSettingsMenu, this, placeholders::_1);

		std::function<void(std::function<void(UIItem*)>)> openTexLoad = std::bind(&Application::createTexLoadMenu, this, placeholders::_1);

		std::function<void(float)> polarFunc = std::bind(&Application::updateLightPolar, this, placeholders::_1);
		std::function<void(float)> azimuthFunc = std::bind(&Application::updateLightAzimuth, this, placeholders::_1);

		objectMenu.setup(loadObjectFunct);
		mouseManager.addClickListener(objectMenu.getClickCallback());
		widgets.push_back(&objectMenu);

		textureMenu.setup(texModify, openTexLoad);
		mouseManager.addClickListener(textureMenu.getClickCallback());
		widgets.push_back(&textureMenu);

		//saveMenu.setup(loadSessionFunc, newSessionFunc);
		//mouseManager.addClickListener(saveMenu.getClickCallback());
		//widgets.push_back(&saveMenu);

		webcamMenu.setup(lightingFunction, webcamSettings);
		mouseManager.addClickListener(webcamMenu.getClickCallback());
		widgets.push_back(&webcamMenu);

		renderMenu.setup(loadObjectFunct, pipelinefunction, polarFunc, azimuthFunc);
		mouseManager.addClickListener(renderMenu.getClickCallback());
		mouseManager.addPositionListener(renderMenu.getPosCallback());
		widgets.push_back(&renderMenu);

		sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });
	}

	void updateColourScheme() {

		ColourSchemeObject cso{};
		cso.Primary = primaryColour;
		cso.Secondary = secondaryColour;
		cso.Tertiary = tertiaryColour;

		memcpy(engine->colourBufferMapped, &cso, sizeof(cso));
	}

	//void toggleTomogMenu() {
	//	if (!tomogActive && sConst->diffTex != nullptr) {
	//		std::function<void(UIItem*)> toggleFunct = std::bind(&Application::toggleTomogMeshes, this, std::placeholders::_1);
	//		std::function<void(UIItem*)> tomogExit = std::bind(&Application::exitTomogMenu, this, std::placeholders::_1);
	//		
	//		if (!tomogUI.isSetup) {
	//			tomogUI.setup(sConst, toggleFunct, &mouseManager, tomogExit);
	//		}
	//		else {
	//			tomogUI.show();
	//		}
			
	//		tomographyPlane = new PlaneObject(sConst->diffTex->texWidth, sConst->diffTex->texHeight);
	//		tomographyPlane->isVisible = true;
	//		for (size_t i = 0; i != staticObjects.size(); i++) {
	//			staticObjects[i].isVisible = false;
	//		}
	//		updateVisibleObjects();
	//		objectMenu.hide();
	//		surfaceMenu.hide();
			
	//		tomogUI.clickIdx = mouseManager.addClickListener(tomogUI.getClickCallback());
	//		widgets.push_back(&tomogUI);

	//		sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });

	//		tomogActive = true;
	//		updateDrawVariables();
	//	}
	//}

	//void exitTomogMenu(UIItem* owner) {
	//	if (!tomogActive) {
	//		return;
	//	}
	//	vkQueueWaitIdle(engine->graphicsQueue);
		
	//	tomographyPlane->mesh->cleanup();
	//	delete tomographyPlane;
	//	tomographyPlane = nullptr;
		
	//	Texture* tomogDiff = UIElements.findTexPtr("TomogDiffTex");
	//	Texture* tomogNorm = UIElements.findTexPtr("TomogNormTex");
	//	if (tomogDiff != nullptr) {
	//		sConst->loadDiffuse(tomogDiff->copyTexture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, 0));
	//		surfaceMenu.setDiffuse(sConst->currentDiffuse());
	//	}
	//	if (tomogNorm != nullptr) {
	//		sConst->normalType = 1;
	//		sConst->loadNormal(tomogNorm->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, 0));
	//		if (!sConst->normalAvailable) {
	//			surfaceMenu.createNormalMenu(owner);
	//		}
	//		sConst->normalType = 1;
	//		surfaceMenu.setNormal(sConst->currentNormal());
	//	}
	//	tomogActive = false;

	//	for (size_t i = 0; i != staticObjects.size(); i++) {
	//		staticObjects[i].isVisible = true;
	//	}
	//	updateVisibleObjects();
		
	//	objectMenu.show();
	//	surfaceMenu.show();

	//	mouseManager.removeClickListener(tomogUI.clickIdx);

	//	if (find(widgets.begin(), widgets.end(), &tomogUI) != widgets.end()) {
	//		widgets.erase(find(widgets.begin(), widgets.end(), &tomogUI));

	//		sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });
	//	}
		
	//	tomogUI.hide();
	//	updateDrawVariables();

	//}
	
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
			if (!inWebSettings) {
				webcamTexture::get()->asyncUpdate();
			}
			else {
				webcamTexture::get()->updateWebcam();
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
		updateVisibleObjects();
	}

	void setObjectWireframe(UIItem* owner) {
		staticObjects[objectMenu.ObjectMap.at(owner->Name)].isWireframeVisible = owner->activestate;
	}

	void setPipelineIndex(UIItem* owner) {
		if (owner->Name == string("WebcamMat")) {
			viewIndex = 0;
			objectMenu.hide();
			textureMenu.hide();
			
			closeTextureSettingsMenu(owner);
			
			textureSettings.setup("Webcam View", std::bind(&Application::closeTextureSettingsMenu, this, std::placeholders::_1), std::bind(&Application::createRemapTexSelector, this, std::placeholders::_1));

			textureSettings.clickIndex = mouseManager.addClickListener(textureSettings.getClickCallback());

			widgets.push_back(&textureSettings);
		}
		else if (owner->Name == string("SurfaceMat")) {
			viewIndex = 1;
			objectMenu.show();
			textureMenu.show();

			closeTextureSettingsMenu(owner);
		}
		else if (owner->Name == string("Wireframe")) {
			viewIndex = 2;
			objectMenu.show();
			textureMenu.show();

			closeTextureSettingsMenu(owner);
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
		string objectName = modelPath;
		string del = "\\";
		auto pos = objectName.find(del);
		while (pos != string::npos) {
			objectName.erase(0, pos + del.length());
			pos = objectName.find(del);
		}
		del = ".";
		pos = objectName.find(del);
		objectName = objectName.substr(0, pos);
		StaticObject newObject(modelPath);
		newObject.objectName = objectName;

		std::function<void(UIItem*)> visibleFunction = bind(&Application::setObjectVisibility, this, placeholders::_1);
		std::function<void(UIItem*)> wireFunction = bind(&Application::setObjectWireframe, this, placeholders::_1);
		std::function<void(UIItem*)> optionsFunction = std::bind(&Application::openObjectSettingsMenu, this, placeholders::_1);

		objectMenu.addObject(visibleFunction, wireFunction, optionsFunction, objectName);
		newObject.isVisible = true;
		newObject.setMat(TextureElements.getMaterial("Webcam Material"), "BF");

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
		TextureElements.empty();
		
		for (size_t i = 0; i != allWidgets.size(); i++) {
			if (allWidgets[i] != nullptr) {
				allWidgets[i]->cleanup();
			}
		}

		renderImage.cleanup(Engine::get()->device);
		renderGP.cleanup(Engine::get()->device);

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
			engine->recreateDrawImage(&renderImage);
			
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

		if (remapMenu.isVisible && remapMenu.isSetup) {
			ubo.UVdistort[0] = 2 * remapMenu.outMap->extentx;
			ubo.UVdistort[1] = (remapMenu.outMap->posx) - remapMenu.outMap->extentx;
			ubo.UVdistort[2] = 2 * remapMenu.outMap->extenty;
			ubo.UVdistort[3] = (remapMenu.outMap->posy) - remapMenu.outMap->extenty;
		}
		else if (textureSettings.isVisible && textureSettings.isSetup) {
			textureSettings.getUVPos(ubo.UVdistort);
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

		memcpy(engine->uniformBuffersMapped[currentImage], &ubo, sizeof(ubo)); 
	} 

	void recordCommandBuffer(VkCommandBuffer commandBuffer, GraphicsPass* currentPass, uint32_t imageIndex) {
		uint32_t currentFrame = engine->currentFrame;

		engine->beginRenderPass(commandBuffer, currentPass, &renderImage, imageIndex, backgroundColour);

		if (showWireframe) {
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *currentPass->GraphicsPipelines[currentPass->pipelineMap.at("UV")]);

			for (uint32_t i = 0; i != staticObjects.size(); i++) {
				if (staticObjects[i].isWireframeVisible) {
					engine->drawObject(commandBuffer, staticObjects[i].mesh->vertexBuffer, staticObjects[i].mesh->indexBuffer, currentPass->pipelineLayouts[currentPass->layoutMap.at("1_")], wireMat->descriptorSets[currentFrame], static_cast<uint32_t>(staticObjects[i].mesh->indices.size()));
				}
			}
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *currentPass->GraphicsPipelines[currentPass->pipelineMap.at("UIGray")]);

		for (size_t i = 0; i != widgets.size(); i++) {
			widgets[i]->drawUI(commandBuffer, currentFrame);
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *currentPass->GraphicsPipelines[currentPass->pipelineMap.at("UI")]);

		for (size_t i = 0; i != widgets.size(); i++) {
			widgets[i]->drawImages(commandBuffer, currentFrame);
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *currentPass->GraphicsPipelines[currentPass->pipelineMap.at("UIText")]);

		for (size_t i = 0; i != widgets.size(); i++) {
			widgets[i]->drawText(commandBuffer, currentFrame);
		}


		//if (tomographyPlane != nullptr && tomographyPlane->isVisible) {
		//	engine->drawObject(commandBuffer, tomographyPlane->mesh->vertexBuffer, tomographyPlane->mesh->indexBuffer, tomogUI.scannedMaterial.pipelineLayout, tomogUI.scannedMaterial.descriptorSets[currentFrame], static_cast<uint32_t>(tomographyPlane->mesh->indices.size()));
		//}

		if (viewIndex == 1 && lit) {
			for (auto elem : objectPipelines) {
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *currentPass->GraphicsPipelines[currentPass->pipelineMap.at(elem.first)]);
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

session* session::sessionInstance = nullptr;
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