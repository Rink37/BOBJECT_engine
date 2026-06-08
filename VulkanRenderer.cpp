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
#include"SeamFixer.h"

#include<chrono>

#include"include/OS_EdgeFill.h"
#include"include/BakedImages.h"
#include"include/CombineNorms.h"

using namespace cv;
using namespace std;

std::vector<KeyManager*> KeyManager::_instances;
KeyManager keyBinds;

std::vector<TypeManager*> TypeManager::_instances;

std::vector<MouseManager*> MouseManager::_instances;
MouseManager mouseManager;

class TextureSettings : public Widget {
public: 
	TextureSettings(LoadList* assets, LoadList* textureAssets) {
		loadList = assets;
		textureLL = textureAssets;
	}

	void setup(std::string texName, std::function<void(UIItem*)> exitFnc, std::function<void(UIItem*)> remapFnc, std::function<void(UIItem*)> transitionTypeFnc, std::function<void(UIItem*)> mixNormalsFnc, std::function<void(UIItem*)> fixSeamsFnc) {
		if (isSetup) {
			return;
		}
		if (!textureLL->checkForTexture(texName)) {
			std::cout << "Texture " << texName << " does not exist in this load list" << std::endl;
			return;
		}

		Texture* tex = textureLL->getTexture(texName);

		imageName = texName;

		Material* closeMat = loadList->getMaterial("CloseBtnMat");
		Material* settingsMat = loadList->getMaterial("SettingsBtnMat");

		Arrangement* mainArrangement = new Arrangement(ORIENT_VERTICAL, 1.0f, 0.0f, 0.25f, 0.8f, 0.01f);
		Arrangement* exitArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.2f, 0.01f);
		Arrangement* remapArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.2f, 0.01f);
		exitArrangement->addItem(getPtr(new spacer()));
		exitArrangement->addItem(getPtr(new Button(closeMat, exitFnc)));

		remapArrangement->addItem(getPtr(new spacer()));
		Button* remapBtn = new Button(settingsMat, remapFnc);
		remapBtn->Name = texName;
		if (tex->isNormal) {
			Button* mapMixer = new Button(settingsMat, mixNormalsFnc);
			mapMixer->Name = texName;
			remapArrangement->addItem(getPtr(mapMixer));

			if (tex->normalType) {
				Material* tsMat = loadList->getMaterial("TSBtnMat");

				Button* normalType = new Button(tsMat, transitionTypeFnc);
				normalType->Name = texName;

				remapArrangement->addItem(getPtr(normalType));
			}
			else {
				Material* osMat = loadList->getMaterial("OSBtnMat");

				Button* normalType = new Button(osMat, transitionTypeFnc);
				normalType->Name = texName;

				remapArrangement->addItem(getPtr(normalType));
			}
		}
		if (texName != "Webcam View") {
			Button* seamFixBtn = new Button(settingsMat, fixSeamsFnc);
			seamFixBtn->Name = texName;

			remapArrangement->addItem(getPtr(seamFixBtn));
		}
		remapArrangement->addItem(getPtr(remapBtn));

		Material* panelMat = loadList->replacePtr(new Material(tex), "Image view");
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
		
		//addBackground(loadList->getMaterial("UIRoundBox"));

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

		Material* settingsMat = loadList->getMaterial("SettingsBtnMat");
		Material* finishmat = loadList->getMaterial("FinishBtnMat");
		Material* forwardMat = loadList->getMaterial("PlayBtnMat");
		Material* backMat = loadList->getMaterial("BackBtnMat");
		Material* rotForward = loadList->getMaterial("RotateFWBtnMat");
		Material* rotBackward = loadList->getMaterial("RotateBWBtnMat");
		Material* visibleMat = loadList->getMaterial("TestCheckBtnMat");

		idButtons->addItem(getPtr(new Button(backMat, idDown)));
		idButtons->addItem(getPtr(new spacer));
		idButtons->addItem(getPtr(new Button(forwardMat, idUp)));
		
		rotationButtons->addItem(getPtr(new Button(rotBackward, subtractRot)));
		rotationButtons->addItem(getPtr(new spacer));
		rotationButtons->addItem(getPtr(new Button(rotForward, addRot)));

		endButtons->addItem(getPtr(new Button(settingsMat, webcamCalib)));
		endButtons->addItem(getPtr(new spacer));
		endButtons->addItem(getPtr(new Button(finishmat, finishCallback)));

		ratioSlider = new Slider(ORIENT_HORIZONTAL, visibleMat, 0.0f, 0.0f, 1.0f, 0.1f);
		ratioSlider->setFloatCallback(std::bind(&WebcamSettings::updateAspectRatio, this, placeholders::_1), true);
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
		cv::Mat testFrame = webcamTexture::get()->webCam->getTestFrame();
		float aspectRatio = static_cast<float>(testFrame.size().width) / static_cast<float>(testFrame.size().height);
		webcamTexture::get()->webCam->updateAspectRatio(aspectRatio);
		webcamTexture::get()->recreateWebcamImage();
		webcamView->image->mat[0]->cleanupDescriptor();
		webcamView->image->mat[0] = new Material(webcamTexture::get());
		ratioSlider->setSlideValues(0.5f, 2.0f, webcamTexture::get()->webCam->sizeRatio);
		reload();
		update();

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

		Material* openMat = loadList->getMaterial("OpenBtnMat");
		Material* saveMat = loadList->getMaterial("SaveBtnMat");
		Material* plusMat = loadList->getMaterial("PlusBtnMat");

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

		Material* renderedMat = loadList->getMaterial("RenderBtnMat");
		Material* webcamViewMat = loadList->getMaterial("WebcamBtnMat");
		Material* wireframeViewMat = loadList->getMaterial("WireframeBtnMat");
		Material* visibleMat = loadList->getMaterial("TestCheckBtnMat");

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

		Slider* polarSlider = new Slider(loadList->getMaterial("UIRoundBox"), 0.0f, 0.0f, 1.0f, 0.25f);
		polarSlider->updateDisplay();
		polarSlider->setSlideValues(0.0f, 3.14159265f, 0.0f);
		polarSlider->setFloatCallback(polarCallback, true);

		Slider* azimuthSlider = new Slider(loadList->getMaterial("UIRoundBox"), 0.0f, 0.0f, 1.0f, 0.25f);
		azimuthSlider->updateDisplay();
		azimuthSlider->setSlideValues(0.0f, 6.283185307f, 0.0f);
		azimuthSlider->setFloatCallback(azimuthCallback, true);

		Arrangement* buttons = new Arrangement(ORIENT_VERTICAL, -1.0f, 1.0f, 0.1f, 0.25f, 0.0f, ARRANGE_START, SCALE_BY_DIMENSIONS);
		buttons->addItem(getPtr(Renderbuttons));
		buttons->addItem(getPtr(polarSlider));
		buttons->addItem(getPtr(azimuthSlider));

		buttons->arrangeItems();
		
		canvas.push_back(getPtr(buttons));

		isSetup = true;
	}
};

class MaterialCreator : public Widget {
public:
	MaterialCreator(LoadList* assets, LoadList* textureAssets) {
		loadList = assets;
		textureLL = textureAssets;
	}

	void setup(std::string sName, GraphicsPass* bPass, std::function<void(UIItem*)> callback) {
		if (isSetup) {
			return;
		}
		shaderName = sName;
		boundPass = bPass;
		matTemplate = new MaterialTemplate(shaderName, boundPass);

		font* inFont = loadList->getFont();
		
		finishedCallback = callback;

		Arrangement* totalArrangement = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.25f, 0.8f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		Material* renderedMat = loadList->getMaterial("RenderBtnMat");
		Material* visibleMat = loadList->getMaterial("TestCheckBtnMat");

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

		TextBox* matNameTB = new TextBox(inFont, 0.0f, 0.0f, 4.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER, true);
		matNameTB->addText(newMaterialName);
		matNameTB->setClickFunction(std::bind(&MaterialCreator::setMaterialName, this, std::placeholders::_1));

		DropdownMenu* materialSelect = new DropdownMenu(0.0f, 0.0f, 5.0f, 1.0f, renderedMat, visibleMat, loadList->getMaterial("UIRoundBox"), inFont);
		materialSelect->addOptions(materialOptions);
		materialSelect->setBlankText("Select material");
		materialSelect->setSelectCallback(std::bind(&MaterialCreator::createMatOptionsMenu, this, std::placeholders::_1));

		totalArrangement->addItem(getPtr(matNameTB));
		totalArrangement->addItem(getPtr(materialSelect));
		totalArrangement->arrangeItems();

		canvas.push_back(getPtr(totalArrangement));
		isSetup = true;
	}

	void setupEditMode(std::string sName, GraphicsPass* bPass, std::function<void(UIItem*)> callback, std::string MatName) {
		if (isSetup) {
			return;
		}
		shaderName = sName;
		boundPass = bPass;
		matTemplate = new MaterialTemplate(shaderName, boundPass);

		font* inFont = loadList->getFont();

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

		empty->cleanup();
		delete empty;

		isSetup = true;
	}

private:
	MaterialTemplate* matTemplate = nullptr;

	std::string shaderName = "";
	GraphicsPass* boundPass = nullptr;

	std::string newMaterialName = "";

	LoadList* textureLL = nullptr;

	std::function<void(UIItem*)> finishedCallback = nullptr;

	std::vector<int> autoIndices{};

	void setMaterialName(UIItem* owner) {
		std::cout << newMaterialName << std::endl;
		newMaterialName = owner->text;
	}

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

		Material* renderedMat = loadList->getMaterial("RenderBtnMat");
		Material* visibleMat = loadList->getMaterial("TestCheckBtnMat");
		Material* finishedMat = loadList->getMaterial("FinishBtnMat");

		std::function<void(UIItem*)> updateMatTemplate = std::bind(&MaterialCreator::setTexCallback, this, std::placeholders::_1);
		std::function<void(UIItem*)> exitCallback = std::bind(&MaterialCreator::exit, this, std::placeholders::_1);

		font* inFont = loadList->getFont();

		int i = 0;
		for (std::string channel : texChannels) {
			int index = defaultIndex;

			if (autoIndices.size() == texChannels.size()) {
				index = (autoIndices[i] > -1) ? autoIndices[i] : defaultIndex;
			}

			TextBox* channelTextBox = new TextBox(inFont, 0.0f, 0.0f, 4.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
			channelTextBox->addText(channel);
			canvas[0]->addItem(getPtr(channelTextBox));

			DropdownMenu* materialSelect = new DropdownMenu(0.0f, 0.0f, 4.0f, 1.0f, renderedMat, visibleMat, loadList->getMaterial("UIRoundBox"), inFont);
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
		textureLL->replacePtr(flatMat, newMaterialName + "_flat");
		owner->Name = newMaterialName;
		owner->text = shaderName;
		if (finishedCallback != nullptr) {
			finishedCallback(owner);
		}
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
		
		font* inFont = loadList->getFont();

		addTextureFunc = addTex;

		Arrangement* totalArrangement = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.25f, 0.8f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		Arrangement* materialSettingsArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 4.0f, 1.0f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		TextBox* matLabel = new TextBox(inFont, 0.0f, 0.0f, 3.0f, 1.0f, 24, ARRANGE_START, ARRANGE_CENTER);
		matLabel->addText("MATERIAL:");

		Material* settingsMat = loadList->getMaterial("SettingsBtnMat");
		Material* plusMat = loadList->getMaterial("PlusBtnMat");

		std::function<void(UIItem*)> newMatBtn = std::bind(&ObjectSettingsMenu::newMaterialMenu, this, std::placeholders::_1);
		std::function<void(UIItem*)> editMatBtn = std::bind(&ObjectSettingsMenu::editMaterialMenu, this, std::placeholders::_1);

		settingsButton = getPtr(new Button(settingsMat, editMatBtn));
		settingsButton->Name = obj->materialName;

		materialSettingsArrangement->addItem(getPtr(matLabel));
		materialSettingsArrangement->addItem(getPtr(new spacer()));
		materialSettingsArrangement->addItem(settingsButton);
		materialSettingsArrangement->addItem(getPtr(new Button(plusMat, newMatBtn)));

		totalArrangement->addItem(getPtr(materialSettingsArrangement));

		Material* renderedMat = loadList->getMaterial("RenderBtnMat");
		Material* visibleMat = loadList->getMaterial("TestCheckBtnMat");
		Material* finishMat = loadList->getMaterial("FinishBtnMat");

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

		DropdownMenu* materialSelect = new DropdownMenu(0.0f, 0.0f, 4.0f, 1.0f, renderedMat, visibleMat, loadList->getMaterial("UIRoundBox"), inFont);
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

		totalArrangement->addItem(getPtr(genNormArrangement));

		totalArrangement->addItem(getPtr(new Button(finishMat, finishedCallback)));
		totalArrangement->arrangeItems();

		canvas.push_back(getPtr(totalArrangement));
		isSetup = true;
	}

private:
	StaticObject* obj = nullptr;

	LoadList* textureLL = nullptr;

	UIItem* matSelPtr = nullptr;

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

		std::vector<std::string> options = static_cast<DropdownMenu*>(matSelPtr)->options;
		if (find(options.begin(), options.end(), obj->materialName) == options.end()) {
			matSelPtr->addOption(obj->materialName);
			matSelPtr->addOption(obj->materialName + "_flat");
		}

		std::vector<std::string> existingMaterials{};
		textureLL->listMaterials(existingMaterials);
		matSelPtr->setOptionIndex(existingMaterials.size() - 2);
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

		OSNormTex->isNormal = true;
		OSNormTex->normalType = false;

		uint8_t existsIndex = 0;
		std::string modifiedOSNormName = OSNormName;

		while (textureLL->checkForTexture(modifiedOSNormName)) {
			modifiedOSNormName = OSNormName + '_' + to_string(existsIndex);
			existsIndex++;
		}
		textureLL->getPtr(OSNormTex, modifiedOSNormName);

		if (addTextureFunc != nullptr) {
			owner->Name = modifiedOSNormName;
			addTextureFunc(owner);
		}

		generator.cleanupGenOS();
		EdgeFillImg->cleanup();
		OS_EdgeFill.cleanup();
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

		invisibleMat = loadList->getMaterial("UnrenderedBtnMat");
		visibleMat = loadList->getMaterial("TestCheckBtnMat");
		wireframeMat = loadList->getMaterial("WireframeBtnMat");
		settingsMat = loadList->getMaterial("SettingsBtnMat");
		Material* loadMat = loadList->getMaterial("OpenBtnMat");

		Arrangement* textArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.2f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

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
		objectButton->Name = nameString;

		Checkbox* objWireframeButton = new Checkbox(wireframeMat, invisibleMat, wireframeToggle);
		objWireframeButton->Name = objectButton->Name;

		ObjectMap.insert({ objectButton->Name, ObjectButtons->Items.size() - 1 });

		font* objectFont = loadList->getFont();
		TextBox* objectName = new TextBox(objectFont, 0.0f, 0.0f, 3.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
		objectName->addText(nameString);

		Button* optionsButton = new Button(settingsMat, optionsMenu);
		optionsButton->Name = nameString;

		objButtons->addItem(getPtr(objectName));
		objButtons->addItem(getPtr(new spacer()));
		objButtons->addItem(getPtr(optionsButton));
		objButtons->addItem(getPtr(objectButton));
		objButtons->addItem(getPtr(objWireframeButton));
		objButtons->arrangeItems();

		ObjectButtons->addItem(getPtr(objButtons));
		ObjectButtons->arrangeItems();

		measureWindowPositions();
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

class NormalMixer : public Widget {
public:
	NormalMixer(LoadList* assets, LoadList* texLL) {
		loadList = assets;
		textureLL = texLL;
	}

	void setup(std::string tName, std::vector<std::string> objects, std::function<Mesh*(std::string)> getMeshFunc, std::function<void(UIItem*)> exitFunc) {
		if (isSetup) {
			return;
		}

		textureName = tName;

		getMesh = getMeshFunc;
		exitCallback = exitFunc;

		std::vector<std::string> textures{};
		textureLL->listTextures(textures);

		auto it = find(textures.begin(), textures.end(), textureName);
		if (it != textures.end()) {
			textures.erase(it);
		}

		Material* invisibleMat = loadList->getMaterial("UnrenderedBtnMat");
		Material* visibleMat = loadList->getMaterial("TestCheckBtnMat");
		Material* finishMat = loadList->getMaterial("FinishBtnMat");

		Arrangement* totalArrangement = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.25f, 0.8f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		TextBox* topText = new TextBox(loadList->getFont(), 0.0f, 0.0f, 5.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
		topText->addText("Mixing " + textureName + " with:");

		totalArrangement->addItem(getPtr(topText));

		DropdownMenu* textureSelect = new DropdownMenu(0.0f, 0.0f, 5.0f, 1.0f, invisibleMat, visibleMat, loadList->getMaterial("UIRoundBox"), loadList->getFont());
		textureSelect->addOptions(textures);
		textureSelect->setOptionIndex(0);
		textureSelect->setSelectCallback(std::bind(&NormalMixer::setMixTex, this, std::placeholders::_1));
		mixTexName = textures[0];

		totalArrangement->addItem(getPtr(textureSelect));

		TextBox* midText = new TextBox(loadList->getFont(), 0.0f, 0.0f, 5.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
		midText->addText("Based on object:");

		totalArrangement->addItem(getPtr(midText));

		DropdownMenu* objSelect = new DropdownMenu(0.0f, 0.0f, 5.0f, 1.0f, invisibleMat, visibleMat, loadList->getMaterial("UIRoundBox"), loadList->getFont());
		objSelect->addOptions(objects);
		objSelect->setOptionIndex(0);
		objSelect->setSelectCallback(std::bind(&NormalMixer::setObject, this, std::placeholders::_1));
		objectName = objects[0];

		totalArrangement->addItem(getPtr(objSelect));

		Arrangement* finishArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 5.0f, 1.0f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);
		finishArrangement->addItem(getPtr(new spacer()));
		finishArrangement->addItem(getPtr(new Button(finishMat, std::bind(&NormalMixer::mixNorms, this, std::placeholders::_1))));

		totalArrangement->addItem(getPtr(finishArrangement));
		totalArrangement->arrangeItems();

		canvas.push_back(getPtr(totalArrangement));

		isSetup = true;
	}

private:
	LoadList* textureLL = nullptr;

	std::string textureName = "";
	std::string mixTexName = "";

	std::string objectName = "";

	std::vector<std::string> createdTextures{};

	std::function<Mesh* (std::string)> getMesh = nullptr;
	std::function<void(UIItem*)> exitCallback = nullptr;

	void createRefNormal(std::string& texName) {
		Mesh* mesh = getMesh(objectName);

		NormalGen generator(textureLL);
		generator.setupOSExtractor();
		VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
		commandBuffer = generator.drawOSMap(commandBuffer, mesh);
		Engine::get()->endSingleTimeCommands(commandBuffer);

		Texture* OSNormTex = generator.objectSpaceMap.colour->copyImage(VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_TILING_LINEAR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1);

		OSNormTex->textureImageView = OSNormTex->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

		OSNormTex->isNormal = true;
		OSNormTex->normalType = false;

		uint8_t existsIndex = 0;
		std::string modifiedOSNormName = "TempOSNorm";

		while (textureLL->checkForTexture(modifiedOSNormName)) {
			modifiedOSNormName = "TempOSNorm_" + to_string(existsIndex);
			existsIndex++;
		}
		textureLL->getPtr(OSNormTex, modifiedOSNormName);

		createdTextures.push_back(modifiedOSNormName);

		generator.cleanupGenOS();

		texName = modifiedOSNormName;
	}

	void changeNormalMapType(std::string tName) {
		if (getMesh == nullptr) {
			return;
		}

		Texture* norm = textureLL->getTexture(tName);
		std::string outNormalName = tName + "_OS";

		Texture* res = nullptr;

		Mesh* mesh = getMesh(objectName);

		VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
		NormalGen generator(loadList);

		if (norm->normalType) {
			generator.copyTSImage(norm);
			generator.setupOSConverter();
			commandBuffer = generator.convertTStoOS(commandBuffer, mesh);
			res = generator.objectSpaceMap.colour;
		}
		else {
			generator.copyOSImage(norm);
			generator.setupTSConverter();
			commandBuffer = generator.convertOStoTS(commandBuffer, mesh);
			res = generator.tangentSpaceMap.colour;
		}

		Engine::get()->endSingleTimeCommands(commandBuffer);

		Texture* outNorm = nullptr;

		if (!norm->normalType) {
			outNorm = res->copyImage(VK_FORMAT_R8G8B8A8_UNORM,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_IMAGE_TILING_LINEAR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1);

			outNorm->textureImageView = outNorm->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
		} else {
			Texture* EdgeFillImg = res->copyTexture(res->textureFormat, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1);
			filter OS_EdgeFill(std::vector<Texture*>({ EdgeFillImg }), new OS_EDGEFILLSHADER, VK_FORMAT_R8G8B8A8_UNORM);
			OS_EdgeFill.filterImage();
			OS_EdgeFill.filterTarget[0]->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			OS_EdgeFill.filterTarget[0]->textureLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

			outNorm = res->copyImage(VK_FORMAT_R8G8B8A8_UNORM,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_IMAGE_TILING_LINEAR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1);

			outNorm->textureImageView = outNorm->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

			OS_EdgeFill.cleanup();
		}

		outNorm->isNormal = true;
		outNorm->normalType = !norm->normalType;

		textureLL->replacePtr(outNorm, outNormalName);

		createdTextures.push_back(outNormalName);

		if (norm->normalType) {
			generator.cleanupOS();
		}
		else {
			generator.cleanupTS();
		}
	}

	void setMixTex(UIItem* owner) {
		mixTexName = owner->text;
	}

	void setObject(UIItem* owner) {
		objectName = owner->text;
	}

	void mixNorms(UIItem* owner) {
		std::cout << textureName << " " << mixTexName << std::endl;

		Texture* baseNorm = textureLL->getTexture(textureName);
		Texture* layerNorm = textureLL->getTexture(mixTexName);

		bool resultNormalType = baseNorm->normalType;

		if (!baseNorm->isNormal || !layerNorm->isNormal) {
			return;
		}
		if (baseNorm->normalType) {
			changeNormalMapType(textureName);
			baseNorm = textureLL->getTexture(textureName + "_OS");
		}
		if (layerNorm->normalType) {
			changeNormalMapType(mixTexName);
			layerNorm = textureLL->getTexture(mixTexName + "_OS");
		}

		std::string refName = "";
		createRefNormal(refName);

		uint32_t width = baseNorm->texWidth;
		uint32_t height = baseNorm->texHeight;

		Texture* refNorm = textureLL->getTexture(refName);

		filter normMixer(std::vector<Texture*>{refNorm->copyTexture(refNorm->textureFormat, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height),
											   baseNorm->copyTexture(baseNorm->textureFormat, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height),
											   layerNorm->copyTexture(layerNorm->textureFormat, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height)},
											   new COMBINENORMSSHADER, VK_FORMAT_R8G8B8A8_UNORM);
		normMixer.filterImage();

		Texture* EdgeFillImg = normMixer.filterTarget[0]->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1);
		filter OS_EdgeFill(std::vector<Texture*>({ EdgeFillImg }), new OS_EDGEFILLSHADER, VK_FORMAT_R8G8B8A8_UNORM);
		OS_EdgeFill.filterImage();
		OS_EdgeFill.filterTarget[0]->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		OS_EdgeFill.filterTarget[0]->textureLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		Texture* mixedNorm = OS_EdgeFill.filterTarget[0]->copyImage(VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_TILING_LINEAR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1);

		mixedNorm->isNormal = true;
		mixedNorm->normalType = resultNormalType;

		mixedNorm->textureImageView = mixedNorm->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

		textureLL->replacePtr(mixedNorm, textureName);

		OS_EdgeFill.cleanup();
		normMixer.cleanup();
		for (std::string tName : createdTextures) {
			textureLL->deleteTexture(tName);
		}

		if (resultNormalType) {
			std::cout << "Attempting to change normal map type" << std::endl;
			changeNormalMapType(textureName);
		}

		owner->Name = textureName;
		if (exitCallback != nullptr) {
			exitCallback(owner);
		}
	}
};

class SpaceTransitionMenu : public Widget {
public:
	SpaceTransitionMenu(LoadList* assets, LoadList* texLL) {
		loadList = assets;
		textureLL = texLL;
	}

	void setup(std::string tName, std::vector<std::string> objects, std::function<void(UIItem*)> addTexFunc, std::function<Mesh*(std::string)> getMeshFunc, std::function<void(UIItem*)> exitFunc) {
		if (isSetup || objects.size() == 0) {
			return;
		}
		texName = tName;

		addTextureFunc = addTexFunc;
		getMesh = getMeshFunc;
		exitCallback = exitFunc;

		for (std::string objName : objects) {
			std::cout << objName << std::endl;
		}

		Arrangement* totalArrangement = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.25f, 0.8f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);
		
		Material* invisibleMat = loadList->getMaterial("UnrenderedBtnMat");
		Material* visibleMat = loadList->getMaterial("TestCheckBtnMat");
		Material* renderedMat = loadList->getMaterial("RenderBtnMat");
		Material* finishMat = loadList->getMaterial("FinishBtnMat");

		DropdownMenu* objectSelector = new DropdownMenu(0.0f, 0.0f, 5.0f, 1.0f, invisibleMat, visibleMat, loadList->getMaterial("UIRoundBox"), loadList->getFont());
		objectSelector->addOptions(objects);
		objectSelector->setSelectCallback(std::bind(&SpaceTransitionMenu::setObjName, this, std::placeholders::_1));
		objectSelector->setOptionIndex(0);

		objectName = objects[0];

		Arrangement* replaceArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 5.0f, 1.0f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);
		TextBox* replaceText = new TextBox(loadList->getFont(), 0.0f, 0.0f, 4.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
		replaceText->addText("In place?");
		replaceArrangement->addItem(getPtr(replaceText));

		overwriteToggle = getPtr(new Checkbox(visibleMat, invisibleMat));
		replaceArrangement->addItem(overwriteToggle);
		
		Arrangement* finishArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 5.0f, 1.0f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);
		finishArrangement->addItem(getPtr(new spacer));
		finishArrangement->addItem(getPtr(new Button(finishMat, std::bind(&SpaceTransitionMenu::exit, this, std::placeholders::_1))));

		totalArrangement->addItem(getPtr(objectSelector));
		totalArrangement->addItem(getPtr(replaceArrangement));
		totalArrangement->addItem(getPtr(finishArrangement));

		totalArrangement->arrangeItems();

		canvas.push_back(getPtr(totalArrangement));

		isSetup = true;
	}

private:
	LoadList* textureLL = nullptr;

	UIItem* overwriteToggle = nullptr;

	std::string texName = "";
	std::string objectName = "";
	std::string outNormalName = "";

	std::function<void(UIItem*)> addTextureFunc = nullptr;
	std::function<void(UIItem*)> exitCallback = nullptr;
	std::function<Mesh*(std::string)> getMesh = nullptr;

	bool overwrite = true;

	void setObjName(UIItem* owner) {
		objectName = owner->text;
	}

	void exit(UIItem* owner) {
		changeNormalMapType(owner);
		owner->Name = outNormalName;
		if (exitCallback != nullptr) {
			exitCallback(owner);
		}
	}

	void changeNormalMapType(UIItem* owner) {
		if (getMesh == nullptr) {
			return;
		}

		overwrite = overwriteToggle->activestate;

		Texture* norm = textureLL->getTexture(texName);
		outNormalName = "";

		if (overwrite) {
			outNormalName = texName;
		}
		else {
			if (norm->normalType) {
				outNormalName = texName + "_OS";
			}
			else {
				outNormalName = texName + "_TS";
			}
		}
		
		Texture* res = nullptr;

		Mesh* mesh = getMesh(objectName);

		VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
		NormalGen generator(loadList);
		
		if (norm->normalType) {
			generator.copyTSImage(norm);
			generator.setupOSConverter();
			commandBuffer = generator.convertTStoOS(commandBuffer, mesh);
			res = generator.objectSpaceMap.colour;
		}
		else {
			generator.copyOSImage(norm);
			generator.setupTSConverter();
			commandBuffer = generator.convertOStoTS(commandBuffer, mesh);
			res = generator.tangentSpaceMap.colour;
		}

		Engine::get()->endSingleTimeCommands(commandBuffer);

		Texture* outNorm = nullptr;

		if (!norm->normalType) {
			outNorm = res->copyImage(VK_FORMAT_R8G8B8A8_UNORM,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_IMAGE_TILING_LINEAR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1);

			outNorm->textureImageView = outNorm->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
		}
		else {
			Texture* EdgeFillImg = res->copyTexture(res->textureFormat, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1);
			filter OS_EdgeFill(std::vector<Texture*>({ EdgeFillImg }), new OS_EDGEFILLSHADER, VK_FORMAT_R8G8B8A8_UNORM);
			OS_EdgeFill.filterImage();
			OS_EdgeFill.filterTarget[0]->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			OS_EdgeFill.filterTarget[0]->textureLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

			outNorm = OS_EdgeFill.filterTarget[0]->copyImage(VK_FORMAT_R8G8B8A8_UNORM,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_IMAGE_TILING_LINEAR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1);

			outNorm->textureImageView = outNorm->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

			OS_EdgeFill.cleanup();
		}

		outNorm->isNormal = true;
		outNorm->normalType = !norm->normalType;

		if (addTextureFunc != nullptr && !textureLL->checkForTexture(outNormalName)) {
			owner->Name = outNormalName;
			addTextureFunc(owner);
		}

		textureLL->replacePtr(outNorm, outNormalName);

		if (norm->normalType) {
			generator.cleanupOS();
		}
		else {
			generator.cleanupTS();
		}
	}
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

		Material* renderedMat = loadList->getMaterial("RenderBtnMat");
		Material* visibleMat = loadList->getMaterial("TestCheckBtnMat");
		Material* finishedMat = loadList->getMaterial("FinishBtnMat");
		Material* osMat = loadList->getMaterial("OSBtnMat");
		Material* tsMat = loadList->getMaterial("TSBtnMat");

		normalTypeToggle = getPtr(new Checkbox(osMat, tsMat));
		normalTypeToggle->setVisibility(false);

		DropdownMenu* loadOption = new DropdownMenu(0.0f, 0.0f, 5.0f, 1.0f, renderedMat, visibleMat, loadList->getMaterial("UIRoundBox"), newFont);
		loadOption->addOptions(loadOptions);
		loadOption->setSelectCallback(std::bind(&TextureLoadMenu::setOptionCallback, this, std::placeholders::_1));
		loadOption->setOptionIndex(0);
		loadOption->execCallback();

		mainArrangement->addItem(getPtr(loadOption));

		Arrangement* finishButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 5.0f, 1.0f, 0.0f, ARRANGE_START, SCALE_BY_DIMENSIONS);
		finishButtons->addItem(getPtr(new spacer()));
		finishButtons->addItem(normalTypeToggle);
		finishButtons->addItem(getPtr(new Button(finishedMat, std::bind(&TextureLoadMenu::finishCallback, this, std::placeholders::_1))));

		mainArrangement->addItem(getPtr(finishButtons));

		mainArrangement->arrangeItems();

		pos = textureName.find(del);
		textureName = textureName.substr(0, pos);

		canvas.push_back(getPtr(mainArrangement));

		isSetup = true;
	}

private:
	LoadList* textureLL = nullptr;

	std::string textureName = "";
	std::string fileName = "";

	UIItem* normalTypeToggle = nullptr;

	bool isNormalized = false;
	bool normalType = false;

	std::function<void(UIItem*)> updateTextureMenu = nullptr;
	std::function<void(UIItem*)> exitCallback = nullptr; 

	void setOptionCallback(UIItem* owner) {
		if (owner->text == std::string("Colour")) {
			isNormalized = false;
			normalTypeToggle->setVisibility(false);
		}
		else {
			isNormalized = true;
			normalTypeToggle->setVisibility(true);
		}
	}

	void finishCallback(UIItem* owner) {
		imageTexture* loadedTexture = nullptr;
		try {
			if (isNormalized) {
				cv::Mat initMat = cv::imread(fileName, cv::IMREAD_UNCHANGED);
				loadedTexture = new imageTexture(initMat, VK_FORMAT_R8G8B8A8_UNORM);
				loadedTexture->isNormal = true;
				normalType = !normalTypeToggle->activestate;
				if (normalType) {
					std::cout << "TS" << std::endl;
				}
				else {
					std::cout << "OS" << std::endl;
				}
				loadedTexture->normalType = normalType;
			}
			else {
				cv::Mat initMat = cv::imread(fileName, cv::IMREAD_UNCHANGED);
				loadedTexture = new imageTexture(initMat, VK_FORMAT_R8G8B8A8_SRGB);
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
		catch (...) {
			std::cout << "Invalid texture name or extension" << std::endl;
			if (exitCallback != nullptr) {
				exitCallback(nullptr);
			}
		}
	}
};

class TextureMenu : public Widget {
public:
	TextureMenu(LoadList* assets, LoadList* textureLL) {
		loadList = assets;
		textureLoadList = textureLL;
	}

	void setup(std::function<void(UIItem*)> modCallback, std::function<void(std::function<void(UIItem*)>)> loadCallback, std::function<void(UIItem*)> tomogCallback) {
		if (isSetup) {
			return;
		}

		objectFont = loadList->getFont();

		modifyCallback = modCallback;
		loadTextureCallback = loadCallback;

		std::function<void(UIItem*)> loadTexFunct = std::bind(&TextureMenu::loadTexture, this, std::placeholders::_1);

		Material* invisibleMat = loadList->getMaterial("UnrenderedBtnMat");
		Material* visibleMat = loadList->getMaterial("TestCheckBtnMat");
		Material* wireframeMat = loadList->getMaterial("WireframeBtnMat");
		settingsMat = loadList->getMaterial("SettingsBtnMat");
		saveMat = loadList->getMaterial("SaveBtnMat");

		saveCallback = std::bind(&TextureMenu::saveTexture, this, std::placeholders::_1);

		Arrangement* textArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.2f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		//imageData lb = OPENBUTTON;
		Material* loadMat = loadList->getMaterial("OpenBtnMat");

		TextBox* menuText = new TextBox(objectFont, 0.0f, 0.0f, 4.0f, 1.0f, 24, ARRANGE_START, ARRANGE_CENTER);
		menuText->addText("TEXTURES");
		textArrangement->addItem(getPtr(menuText));
		textArrangement->addItem(getPtr(new spacer()));
		textArrangement->addItem(getPtr(new Button(loadMat, loadTexFunct)));

		Arrangement* tomogArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.2f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);
		TextBox* tomogMenuText = new TextBox(objectFont, 0.0f, 0.0f, 6.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
		tomogMenuText->addText("Extract from painting:");
		tomogArrangement->addItem(getPtr(tomogMenuText));
		tomogArrangement->addItem(getPtr(new spacer()));
		tomogArrangement->addItem(getPtr(new Button(settingsMat, tomogCallback)));

		TextureButtons = getPtr(new Arrangement(ORIENT_VERTICAL, -0.9f, -0.5625f, 0.2f, 0.4375f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS));

		TextureButtons->addItem(getPtr(textArrangement));
		TextureButtons->addItem(getPtr(tomogArrangement));
		
		Arrangement* objButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.15f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		std::string textureName = "Webcam View";
		
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

		sortImages();
		measureWindowPositions();
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
		Material* renderedMat = loadList->getMaterial("RenderBtnMat");
		Material* unrenderedMat = loadList->getMaterial("UnrenderedBtnMat");
		Material* playMat = loadList->getMaterial("PlayBtnMat");
		Material* pauseMat = loadList->getMaterial("PauseBtnMat");
		Material* settingsMat = loadList->getMaterial("SettingsBtnMat");
		Material* webcamMat = loadList->getMaterial("WebcamOnBtnMat");

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
		if (webcamTexture::get()->webCam != nullptr) {
			webcamTexture::get()->webCam->loadFilter();
		}
		TextureElements.getPtr(webcamTexture::get(), "Webcam View");
		std::function<void()> colourChange = bind(&Application::colourChangeTest, this);
		std::function<void()> FPSTrack = bind(&Application::startFPSTrack, this);
		std::function<void()> widgetToggle = bind(&Application::toggleWidgets, this);
		keyBinds.addBinding(GLFW_KEY_1, colourChange, PRESS_EVENT);
		keyBinds.addBinding(GLFW_KEY_F, FPSTrack, PRESS_EVENT);
		keyBinds.addBinding(GLFW_KEY_F1, widgetToggle, PRESS_EVENT);
		webcamTexture::get()->webCam->shouldUpdate = false;

		Engine::get()->createRenderPass(renderGP.renderPass, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		renderImage = Engine::get()->createDrawImage(Engine::get()->swapChainExtent.width, Engine::get()->swapChainExtent.height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, renderGP.renderPass);
		Engine::get()->createGraphicsPipelines(renderGP);

		currentPass = &renderGP;

		webcamMat = TextureElements.getPtr(new Material(webcamTexture::get()) , "Webcam Material");
		MaterialTemplate wireMatTemplate(std::string("W"), &renderGP);
		wireMatTemplate.listChannels();
		wireMat = UIElements.getPtr(wireMatTemplate.createMaterial(), "Wire Material");

		createWidgetMaterials();
		createCanvas();
		
		webcamMenu.canvas[0]->Items[1]->activestate = false;
		webcamMenu.canvas[0]->Items[1]->image->matidx = 1;

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

	TomographyMenu tomogUI = TomographyMenu(&UIElements, &TextureElements);
	SaveMenu saveMenu = SaveMenu(&UIElements);
	WebcamMenu webcamMenu = WebcamMenu(&UIElements);
	RenderMenu renderMenu = RenderMenu(&UIElements);
	ObjectMenu objectMenu = ObjectMenu(&UIElements);
	TextureMenu textureMenu = TextureMenu(&UIElements, &TextureElements);
	TextureSettings textureSettings = TextureSettings(&UIElements, &TextureElements);
	RemapUI remapMenu = RemapUI(&UIElements, &TextureElements);
	WebcamSettings webSets = WebcamSettings(&UIElements);
	MaterialCreator mc = MaterialCreator(&UIElements, &TextureElements); 
	ObjectSettingsMenu osm = ObjectSettingsMenu(&UIElements, &TextureElements);
	RemapTexSelector rts = RemapTexSelector(&UIElements, &TextureElements);
	TextureLoadMenu tlm = TextureLoadMenu(&UIElements, &TextureElements);
	TomogRefPicker tomogPicker = TomogRefPicker(&UIElements, &TextureElements);
	SpaceTransitionMenu stm = SpaceTransitionMenu(&UIElements, &TextureElements);
	NormalMixer normalMixer = NormalMixer(&UIElements, &TextureElements);
	SeamObjPicker sob = SeamObjPicker(&UIElements, &TextureElements);
	SeamFixMenu seamFixer = SeamFixMenu(&UIElements, &TextureElements);

	bool showWidgets = true;

	Material* webcamMat = nullptr;
	Material* wireMat = nullptr;

	vector<Widget*> widgets;
	vector<Widget*> allWidgets = { &tomogUI, &saveMenu, &webcamMenu, &renderMenu, &objectMenu, &textureMenu, &textureSettings, &remapMenu, &webSets, &mc, &osm, &rts, &tlm, &tomogPicker, &stm, &normalMixer, &sob, &seamFixer};

	drawImage renderImage;
	GraphicsPass renderGP;
	GraphicsPass* currentPass = nullptr;

	StaticObject* currentObject = nullptr;

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

	void toggleWidgets() {
		showWidgets = !showWidgets;
	}

	void createWidgetMaterials() {
		UIElements.getPtr(new Material(std::vector<Texture*>{}, "UIRoundBox", currentPass, true), "UIRoundBox");
		imageData cb = CLOSEBUTTON;
		UIElements.getPtr(&cb, "CloseBtn");
		imageData sb = SETTINGSBUTTON;
		UIElements.getPtr(&sb, "SettingsBtn");
		imageData tsb = TANGENTSPACE;
		UIElements.getPtr(&tsb, "TSBtn");
		imageData osb = OSBUTTON;
		UIElements.getPtr(&osb, "OSBtn");
		imageData finishBtnImage = FINISHBUTTON;
		UIElements.getPtr(&finishBtnImage, "FinishBtn");
		imageData cancelBtnImage = CANCELBUTTON;
		UIElements.getPtr(&cancelBtnImage, "CancelBtn");
		imageData pb = PLAYBUTTON;
		UIElements.getPtr(&pb, "PlayBtn");
		imageData psb = PAUSEBUTTON;
		UIElements.getPtr(&psb, "PauseBtn");
		imageData bb = BACKBUTTON;
		UIElements.getPtr(&bb, "BackBtn");
		imageData rf = ROTATEFORWARD;
		UIElements.getPtr(&rf, "RotateFWBtn");
		imageData rtb = ROTATEBACKWARD;
		UIElements.getPtr(&rtb, "RotateBWBtn");
		imageData tcb = TESTCHECKBOXBUTTON;
		UIElements.getPtr(&tcb, "TestCheckBtn");
		imageData OpenButton = OPENBUTTON;
		UIElements.getPtr(&OpenButton, "OpenBtn");
		imageData SaveButton = SAVEBUTTON;
		UIElements.getPtr(&SaveButton, "SaveBtn");
		imageData plusButton = PLUSBUTTON;
		UIElements.getPtr(&plusButton, "PlusBtn");
		imageData rb = RENDEREDBUTTON;
		UIElements.getPtr(&rb, "RenderBtn");
		imageData ub = WEBCAMVIEWBUTTON;
		UIElements.getPtr(&ub, "WebcamBtn");
		imageData wb = WIREFRAMEBUTTON;
		UIElements.getPtr(&wb, "WireframeBtn");
		imageData urb = UNRENDEREDBUTTON;
		UIElements.getPtr(&urb, "UnrenderedBtn");
		imageData webcamOn = WEBCAMONBUTTON;
		UIElements.getPtr(&webcamOn, "WebcamOnBtn");
		imageData update = UPDATEBUTTON;
		UIElements.getPtr(&update, "UpdateBtn");
		imageData plb = PLANEBUTTON;
		UIElements.getPtr(&plb, "PlaneBtn");
		imageData crs = CLOSEBUTTON;
		UIElements.getPtr(&crs, "CrossBtn");
	}

	void addWidget(Widget* widget, bool hasClick = true, bool hasPos = false) {
		// The widget must be set up beforehand since setup functions don't have consistent arguments
		if (!widget->isSetup) {
			return;
		}

		if (hasClick) {
			widget->clickIndex = mouseManager.addClickListener(widget->getClickCallback());
		}
		else {
			widget->clickIndex = INT_MAX;
		}
		if (hasPos) {
			widget->posIndex = mouseManager.addPositionListener(widget->getPosCallback());
		}
		else {
			widget->posIndex = INT_MAX;
		}
		widgets.push_back(widget);

		sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });
	}

	void removeWidget(Widget* widget) {
		vkDeviceWaitIdle(Engine::get()->device);

		if (widget->clickIndex != INT_MAX) {
			mouseManager.removeClickListener(widget->clickIndex);
		}
		if (widget->posIndex != INT_MAX) {
			mouseManager.removePositionListener(widget->posIndex);
		}
		widget->cleanup();
		widgets.erase(find(widgets.begin(), widgets.end(), widget));

		sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });
	}

	void openSeamObjPicker(UIItem* owner) {
		std::string texName = owner->Name;
		
		std::vector<std::string> objectNames{};
		for (auto elem : objectMenu.ObjectMap) {
			objectNames.push_back(elem.first);
		}
		closeTextureSettingsMenu(owner);

		sob.setup(texName, objectNames, std::bind(&Application::openSeamFixer ,this, std::placeholders::_1));
		
		addWidget(&sob);
	}

	void openSeamFixer(UIItem* owner) {
		std::string texName = owner->Name;
		std::string objName = owner->text;
		Mesh* selectedMesh = staticObjects[objectMenu.ObjectMap.at(objName)].mesh;

		removeWidget(&sob);

		seamFixer.setup(selectedMesh, texName, std::bind(&Application::cancelSeamFixer, this, std::placeholders::_1), std::bind(&Application::finishSeamFixer, this, std::placeholders::_1));
		addWidget(&seamFixer, true, true);
	}

	void cancelSeamFixer(UIItem* owner) {
		std::string texName = owner->Name;
		
		removeWidget(&seamFixer);

		UIItem* temp = new spacer;
		temp->Name = texName;
		openTextureSettingsMenu(temp);
		temp->cleanup();
		delete temp;
	}

	void finishSeamFixer(UIItem* owner) {
		std::string texName = owner->Name;

		removeWidget(&seamFixer);

		UIItem* temp = new spacer;
		temp->Name = texName;
		openTextureSettingsMenu(temp);
		temp->cleanup();
		delete temp;
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
		addWidget(&webSets);

		inWebSettings = true;
		webcamMenu.canvas[0]->Items[1]->image->matidx = 0;
		webcamMenu.canvas[0]->Items[1]->activestate = true;
	}

	void finishWebSettings(UIItem* owner) {
		removeWidget(&webSets);

		inWebSettings = false;
	}

	void createRemapTexSelector(UIItem* owner) {
		std::function<void(std::string, std::string)> continueCallback = std::bind(&Application::createRemapper, this, std::placeholders::_1, std::placeholders::_2);
		
		rts.setup(owner->Name, continueCallback);
		
		addWidget(&rts);
	}

	void createRemapper(std::string refTexture, std::string targetTexture) {
		std::function<void(UIItem*)> destroySelf = std::bind(&Application::destroyRemapper, this, std::placeholders::_1);
		std::function<void(UIItem*)> finishSelf = std::bind(&Application::finishRemapper, this, std::placeholders::_1);

		removeWidget(&rts);

		UIItem* newItem = new spacer();
		closeTextureSettingsMenu(newItem);
		newItem->cleanup();
		delete newItem;

		remapMenu.setup(refTexture, targetTexture, destroySelf, finishSelf); 
		if (!remapMenu.isSetup) {
			return;
		}

		addWidget(&remapMenu);
	}

	void destroyRemapper(UIItem* owner) {
		vkDeviceWaitIdle(Engine::get()->device);

		remapMenu.remapper->baseTarget->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		remapMenu.remapper->baseTarget->textureLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		
		std::string outName = remapMenu.targetTexName;
		TextureElements.replacePtr(remapMenu.remapper->baseTarget->copyTexture(), outName);
		owner->Name = outName;
		
		mouseManager.removeClickListener(remapMenu.clickIndex);
		mouseManager.removePositionListener(remapMenu.posIndex);
		
		openTextureSettingsMenu(owner);

		remapMenu.cleanup();

		widgets.erase(find(widgets.begin(), widgets.end(), &remapMenu));

		sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });
	}

	void finishRemapper(UIItem* owner) {
		vkDeviceWaitIdle(Engine::get()->device);
		
		std::string outName = remapMenu.targetTexName;
		TextureElements.replacePtr(remapMenu.remapper->filteredTarget->copyTexture(), remapMenu.targetTexName);
		owner->Name = outName;
		
		mouseManager.removeClickListener(remapMenu.clickIndex);
		mouseManager.removePositionListener(remapMenu.posIndex);
		
		openTextureSettingsMenu(owner);

		remapMenu.cleanup();

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
	}

	void createTexLoadMenu(std::function<void(UIItem*)> updateTexMenu) {
		std::function<void(UIItem*)> exitCallback = std::bind(&Application::exitTexLoadMenu, this, std::placeholders::_1);
		
		tlm.setup(updateTexMenu, exitCallback);
		if (!tlm.isSetup) {
			return;
		}
		addWidget(&tlm);

	}

	void exitTexLoadMenu(UIItem* owner) {
		removeWidget(&tlm);
	}

	void openTextureSettingsMenu(UIItem* owner) {
		std::function<void(UIItem*)> seamFixFunc = std::bind(&Application::openSeamObjPicker, this, std::placeholders::_1);

		textureSettings.setup(owner->Name, std::bind(&Application::closeTextureSettingsMenu, this, std::placeholders::_1), std::bind(&Application::createRemapTexSelector, this, std::placeholders::_1), std::bind(&Application::createSpaceTransitionMenu, this, std::placeholders::_1), std::bind(&Application::createNormalMixer, this, std::placeholders::_1), seamFixFunc);

		addWidget(&textureSettings);
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
		removeWidget(&textureSettings);
	}

	void openObjectSettingsMenu(UIItem* owner) {
		std::function<void(std::function<void(UIItem*)>, std::string)> openEditMenu = std::bind(&Application::openEditSettingsMenu, this, std::placeholders::_1, std::placeholders::_2);
		
		osm.setup(&staticObjects[objectMenu.ObjectMap.at(owner->Name)], textureMenu.getAddTexCallback(), std::bind(&Application::openSettingsMenu, this, std::placeholders::_1), std::bind(&Application::closeSettingsMenu, this, std::placeholders::_1), std::bind(&Application::closeObjectSettingsMenu, this, std::placeholders::_1), openEditMenu);
		
		addWidget(&osm);
	}

	void closeObjectSettingsMenu(UIItem* owner) {
		removeWidget(&osm);
	}

	void openSettingsMenu(std::function<void(UIItem*)> closeFnc) {
		osm.hide();
		mc.setup("BF", currentPass, closeFnc);
		addWidget(&mc);
	}

	void openEditSettingsMenu(std::function<void(UIItem*)> closeFnc, std::string matName) {
		osm.hide();
		mc.setupEditMode("BF", currentPass, closeFnc, matName);
		addWidget(&mc);
	}

	void closeSettingsMenu(UIItem* owner) {

		removeWidget(&mc);

		osm.show();
		osm.update();

		updateVisibleObjects();
	}
	
	void loadSave(UIItem* owner) {

		//string saveLocation;
		//saveLocation = winFile::OpenFileDialog();
		//if (saveLocation == "fail") {
		//	return;
		//}
		//newSession(owner);
		//session::get()->loadStudio(saveLocation);

		//uint8_t webcamRot = 0;
		//uint8_t webcamIndex = 0;

		//session::get()->currentStudio.unpackWebcamSettings(webcamRot, webcamIndex);

		//for (string path : session::get()->currentStudio.modelPaths) {
		//	StaticObject newObject(path);
		//	string objectName = path;
		//	string del = "\\";
		//	auto pos = objectName.find(del);
		//	while (pos != string::npos) {
		//		objectName.erase(0, pos + del.length());
		//		pos = objectName.find(del);
		//	}
		//	del = ".";
		//	pos = objectName.find(del);
		//	objectName = objectName.substr(0, pos);
		//	newObject.objectName = objectName;

		//	std::function<void(UIItem*)> visibleFunction = std::bind(&Application::setObjectVisibility, this, placeholders::_1);
		//	std::function<void(UIItem*)> wireFunction = std::bind(&Application::setObjectWireframe, this, placeholders::_1);
		//	std::function<void(UIItem*)> optionsFunction = std::bind(&Application::openObjectSettingsMenu, this, placeholders::_1);

		//	objectMenu.addObject(visibleFunction, wireFunction, optionsFunction, objectName);

		//	newObject.isVisible = true;
		//	newObject.setMat(TextureElements.getMaterial("Webcam Material"), "BF");

			//staticObjects.push_back(newObject);
		//}
		//if (session::get()->currentStudio.diffusePath != "None") {
		//	imageTexture* loadedTexture = new imageTexture(session::get()->currentStudio.diffusePath, VK_FORMAT_R8G8B8A8_SRGB);
		//}
		//if (session::get()->currentStudio.OSPath != "None") {
		//	imageTexture* loadedTexture = new imageTexture(session::get()->currentStudio.OSPath, VK_FORMAT_R8G8B8A8_UNORM);
		//}
		//if (session::get()->currentStudio.TSPath != "None") {
		//	imageTexture* loadedTexture = new imageTexture(session::get()->currentStudio.TSPath, VK_FORMAT_R8G8B8A8_UNORM);
		//}
		//if (webcamIndex != webcamTexture::get()->webCam->camIndex) {
		//	webcamTexture::get()->webCam->switchWebcam(webcamIndex);
		//}
		//webcamTexture::get()->webCam->setRotation(webcamRot);
		//webcamTexture::get()->webCam->updateAspectRatio(session::get()->currentStudio.webcamAspectRatio);
		//webcamTexture::get()->recreateWebcamImage();
		//webcamTexture::get()->interruptFrameUpdate();
		//webcamTexture::get()->webCam->loadFilter();
		//webcamTexture::get()->startFrameUpdate();

		//updateVisibleObjects();
	}
	
	void createCanvas() {

		std::function<void(UIItem*)> pipelinefunction = std::bind(&Application::setPipelineIndex, this, placeholders::_1);
		std::function<void(UIItem*)> lightingFunction = std::bind(&Application::toggleLighting, this, placeholders::_1);
		std::function<void(UIItem*)> loadObjectFunct = std::bind(&Application::buttonLoadStaticObject, this, placeholders::_1);
		std::function<void(UIItem*)> loadSessionFunc = std::bind(&Application::loadSave, this, placeholders::_1);
		std::function<void(UIItem*)> newSessionFunc = std::bind(&Application::newSession, this, placeholders::_1);
		std::function<void(UIItem*)> webcamSettings = std::bind(&Application::createWebSettings, this, placeholders::_1);
		std::function<void(UIItem*)> texModify = std::bind(&Application::openTextureSettingsMenu, this, placeholders::_1);
		std::function<void(UIItem*)> tomogCallback = std::bind(&Application::createTomogSelector, this, placeholders::_1);

		std::function<void(std::function<void(UIItem*)>)> openTexLoad = std::bind(&Application::createTexLoadMenu, this, placeholders::_1);

		std::function<void(float)> polarFunc = std::bind(&Application::updateLightPolar, this, placeholders::_1);
		std::function<void(float)> azimuthFunc = std::bind(&Application::updateLightAzimuth, this, placeholders::_1);

		objectMenu.setup(loadObjectFunct);
		addWidget(&objectMenu);

		textureMenu.setup(texModify, openTexLoad, tomogCallback);
		addWidget(&textureMenu);

		//saveMenu.setup(loadSessionFunc, newSessionFunc);
		//mouseManager.addClickListener(saveMenu.getClickCallback());
		//widgets.push_back(&saveMenu);

		webcamMenu.setup(lightingFunction, webcamSettings);
		addWidget(&webcamMenu);

		renderMenu.setup(loadObjectFunct, pipelinefunction, polarFunc, azimuthFunc);
		addWidget(&renderMenu, true, true);
	}

	void updateColourScheme() {

		ColourSchemeObject cso{};
		cso.Primary = primaryColour;
		cso.Secondary = secondaryColour;
		cso.Tertiary = tertiaryColour;

		memcpy(engine->colourBufferMapped, &cso, sizeof(cso));
	}

	Mesh* getObj(std::string objName) {
		if (objectMenu.ObjectMap.count(objName) == 0) {
			return nullptr;
		}
		return staticObjects[objectMenu.ObjectMap.at(objName)].mesh;
	}

	void createNormalMixer(UIItem* owner) {
		std::vector<std::string> objects{};
		for (auto elem : objectMenu.ObjectMap) {
			objects.push_back(elem.first);
		}

		std::string texName = owner->Name;
		closeTextureSettingsMenu(owner);

		std::function<Mesh* (std::string)> getMeshFnc = std::bind(&Application::getObj, this, std::placeholders::_1);

		normalMixer.setup(texName, objects, getMeshFnc, std::bind(&Application::exitNormalMixer, this, std::placeholders::_1));
		addWidget(&normalMixer);
	}

	void exitNormalMixer(UIItem* owner) {
		vkDeviceWaitIdle(Engine::get()->device);

		UIItem* temp = new spacer;
		temp->Name = owner->Name;
		mouseManager.removeClickListener(normalMixer.clickIndex);
		normalMixer.cleanup();

		openTextureSettingsMenu(temp);
		temp->cleanup();
		delete temp;

		widgets.erase(find(widgets.begin(), widgets.end(), &normalMixer));
	}

	void createSpaceTransitionMenu(UIItem* owner) {
		std::vector<std::string> objects{};
		for (auto elem : objectMenu.ObjectMap) {
			objects.push_back(elem.first);
		}

		std::string texName = owner->Name;
		closeTextureSettingsMenu(owner);

		std::function<Mesh*(std::string)> getMeshFnc = std::bind(&Application::getObj, this, std::placeholders::_1);
		std::function<void(UIItem*)> exitFnc = std::bind(&Application::exitSpaceTransitionMenu, this, std::placeholders::_1);

		stm.setup(texName, objects, textureMenu.getAddTexCallback(), getMeshFnc, exitFnc);

		addWidget(&stm);
	}

	void exitSpaceTransitionMenu(UIItem* owner) {
		vkDeviceWaitIdle(Engine::get()->device);
		
		UIItem* temp = new spacer;
		temp->Name = owner->Name;
		mouseManager.removeClickListener(stm.clickIndex);
		stm.cleanup();

		openTextureSettingsMenu(temp);
		temp->cleanup();
		delete temp;

		widgets.erase(find(widgets.begin(), widgets.end(), &stm));

		sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });
	}

	void createTomogSelector(UIItem* owner) {
		std::function<void(UIItem*)> cancelFunc = std::bind(&Application::exitTomogSelector, this, std::placeholders::_1);
		std::function<void(UIItem*)> finishFunc = std::bind(&Application::openTomogMenu, this, std::placeholders::_1);
		
		tomogPicker.setup(finishFunc, cancelFunc);

		addWidget(&tomogPicker);
	}

	void exitTomogSelector(UIItem* owner) {
		removeWidget(&tomogPicker);
	}

	void openTomogMenu(UIItem* owner) {
		std::cout << owner->Name << std::endl;

		std::function<void(UIItem*)> toggleFunct = std::bind(&Application::toggleTomogMeshes, this, std::placeholders::_1);
		std::function<void(UIItem*)> tomogExit = std::bind(&Application::exitTomogMenu, this, std::placeholders::_1);

		tomogUI.setup(owner->Name, toggleFunct, tomogExit, &mouseManager);

		Texture* refTex = TextureElements.getTexture(owner->Name);

		tomographyPlane = new PlaneObject(refTex->texWidth, refTex->texHeight);
		tomographyPlane->isVisible = true;
		for (size_t i = 0; i != staticObjects.size(); i++) {
			staticObjects[i].isVisible = false;
		}
		updateVisibleObjects();

		exitTomogSelector(owner);

		addWidget(&tomogUI);

		tomogActive = true;
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
		tomogNorm->isNormal = true;
		tomogNorm->normalType = true;

		TextureElements.getPtr(tomogDiff, "TomogDiffTex");
		TextureElements.getPtr(tomogNorm, "TomogNormTex");


		std::function<void(UIItem*)> addTex = textureMenu.getAddTexCallback();

		std::cout << "Adding items to texture menu" << std::endl;

		UIItem* tempItem = new spacer;
		tempItem->Name = "TomogDiffTex";
		addTex(tempItem);
		tempItem->Name = "TomogNormTex";
		addTex(tempItem);
		tempItem->cleanup();
		delete tempItem;
	//	
	// 
	//  if (tomogDiff != nullptr) {
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
		tomogActive = false;

		std::cout << "Showing visible objects" << std::endl;
		for (size_t i = 0; i != staticObjects.size(); i++) {
			staticObjects[i].isVisible = true;
		}
		updateVisibleObjects();

		std::cout << "Removing click listener" << std::endl;
		mouseManager.removeClickListener(tomogUI.clickIdx);

		std::cout << "Cleaning up tomograpy UI" << std::endl;
		tomogUI.cleanup();
		if (find(widgets.begin(), widgets.end(), &tomogUI) != widgets.end()) {
			widgets.erase(find(widgets.begin(), widgets.end(), &tomogUI));

			sort(widgets.begin(), widgets.end(), [](Widget* a, Widget* b) {return a->priorityLayer > b->priorityLayer; });
		}
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
			
			textureSettings.setup("Webcam View", std::bind(&Application::closeTextureSettingsMenu, this, std::placeholders::_1), std::bind(&Application::createRemapTexSelector, this, std::placeholders::_1), nullptr, nullptr, nullptr);

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
		std::string extension = objectName.substr(pos+1, objectName.size());
		std::cout << extension << std::endl;
		if (extension != std::string("obj")) {
			std::cout << "File extension " << extension << " is not currently supported for models" << std::endl;
			return;
		}
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

		if (showWidgets) {
			if (showWireframe) {
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *currentPass->GraphicsPipelines[currentPass->pipelineMap.at("UV")]);

				for (uint32_t i = 0; i != staticObjects.size(); i++) {
					if (staticObjects[i].isWireframeVisible) {
						engine->drawObject(commandBuffer, staticObjects[i].mesh->vertexBuffer, staticObjects[i].mesh->indexBuffer, currentPass->pipelineLayouts[currentPass->layoutMap.at("1_")], wireMat->descriptorSets[currentFrame], static_cast<uint32_t>(staticObjects[i].mesh->indices.size()));
					}
				}
			}
			for (size_t i = 0; i != widgets.size(); i++) {
				widgets[i]->drawAll(commandBuffer, currentFrame, currentPass);
			}
		}
		
		if (tomographyPlane != nullptr && tomographyPlane->isVisible) {
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *currentPass->GraphicsPipelines[currentPass->pipelineMap.at(tomogUI.renderPipeline)]);

			engine->drawObject(commandBuffer, tomographyPlane->mesh->vertexBuffer, tomographyPlane->mesh->indexBuffer, tomogUI.scannedMaterial.pipelineLayout, tomogUI.scannedMaterial.descriptorSets[currentFrame], static_cast<uint32_t>(tomographyPlane->mesh->indices.size()));
		}

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