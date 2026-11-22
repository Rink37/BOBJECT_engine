#include"TradPainterWidgets.h"

void TextureSettings::setup(std::string texName, std::function<void(UIItem*)> remapFnc, std::function<void(UIItem*)> transitionTypeFnc, std::function<void(UIItem*)> mixNormalsFnc, std::function<void(UIItem*)> fixSeamsFnc, std::function<void(UIItem*)> projectorFunc, std::function<void(UIItem*)> cancelProjectorFnc, std::function<void(UIItem*)> finishProjectorFnc, std::function<void(UIItem*)> bakeAlphaFunc, std::function<void(UIItem*)> alphaOverFunc, std::function<void(float)> updateProjectorAngleFunc, std::function<void(UIItem*)> extractAlphaMap) {
	if (isSetup) {
		return;
	}
	if (!textureLL->checkForTexture(texName)) {
		return;
	}

	functions.clear();
	functions.insert({ "Convert norm", transitionTypeFnc });
	functions.insert({ "Combine norms", mixNormalsFnc });
	functions.insert({ "Remap", remapFnc });
	functions.insert({ "Fix seams", fixSeamsFnc });
	functions.insert({ "Bake alpha", bakeAlphaFunc });
	functions.insert({ "Alpha over", alphaOverFunc });
	functions.insert({ "Extract alpha", extractAlphaMap });

	Texture* tex = textureLL->getTexture(texName);

	imageName = texName;

	Material* closeMat = loadList->getMaterial("CloseBtnMat");
	Material* settingsMat = loadList->getMaterial("SettingsBtnMat");

	Arrangement* mainArrangement = new Arrangement(ORIENT_VERTICAL, 1.0f, 0.0f, 0.25f, 0.8f, 0.01f);
	Arrangement* projectorArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.2f, 0.01f, ARRANGE_FILL, SCALE_BY_DIMENSIONS);
	Arrangement* projectorFinishBtns = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.2f, 0.01f, ARRANGE_FILL, SCALE_BY_DIMENSIONS);
	Arrangement* exitArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.2f, 0.01f);
	exitArrangement->addItem(getPtr(new spacer()));
	exitArrangement->addItem(getPtr(new Button(closeMat, std::bind(&TextureSettings::selfDestruct, this, std::placeholders::_1))));

	std::vector<std::string> functionNames{};

	functionNames.push_back("Remap");
	functionNames.push_back("Fix seams");
	if (tex->isNormal) {
		functionNames.push_back("Convert norm");
		functionNames.push_back("Combine norms");
	}
	else {
		functionNames.push_back("Bake alpha");
		functionNames.push_back("Alpha over");
		functionNames.push_back("Extract alpha");
	}

	Material* panelMat = loadList->replacePtr(new Material(tex, "UI"), "Image view");
	bool isWebcam = (texName == std::string("Webcam View"));
	imagePanel = new ImagePanel(panelMat, isWebcam);

	Material* renderedMat = loadList->getMaterial("RenderBtnMat");
	Material* visibleMat = loadList->getMaterial("TestCheckBtnMat");

	DropdownMenu* functionDropdown = new DropdownMenu(0.0f, 0.0f, 4.0f, 1.0f, renderedMat, visibleMat, loadList->getMaterial("UIRoundBox"), loadList->getFont());
	functionDropdown->addOptions(functionNames);
	functionDropdown->setBlankText("Choose a function:");
	functionDropdown->setSelectCallback(std::bind(&TextureSettings::functionMap, this, std::placeholders::_1));

	TextBox* projectorLabel = new TextBox(loadList->getFont(), 0.0f, 0.0f, 2.5f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
	projectorLabel->addText("Projector");

	Button* projector = new Button(renderedMat, projectorFunc);
	projector->Name = texName;

	projectorArrangement->addItem(getPtr(projectorLabel));
	projectorArrangement->addItem(getPtr(projector));

	Slider* angleSlider = new Slider(ORIENT_HORIZONTAL, visibleMat, 0.0f, 0.0f, 4.0f, 1.0f);
	angleSlider->setFloatCallback(updateProjectorAngleFunc, true);
	angleSlider->setSlideValues(0.0f, glm::pi<float>(), glm::pi<float>() / 2.0f);

	Button* cancelProjector = new Button(loadList->getFont(), loadList->getMaterial("UIRoundBox"), "CANCEL", cancelProjectorFnc);
	Button* finishProjector = new Button(loadList->getFont(), loadList->getMaterial("UIRoundBox"), "FINISH", finishProjectorFnc);

	projectorFinishBtns->addItem(getPtr(cancelProjector));
	projectorFinishBtns->addItem(getPtr(finishProjector));

	mainArrangement->addItem(getPtr(exitArrangement));
	mainArrangement->addItem(getPtr(imagePanel));
	if (texName != std::string("Webcam View")) {
		mainArrangement->addItem(getPtr(functionDropdown));
	}
	mainArrangement->addItem(getPtr(projectorArrangement));
	mainArrangement->addItem(getPtr(angleSlider));
	mainArrangement->addItem(getPtr(projectorFinishBtns));
	mainArrangement->addItem(getPtr(new spacer()));
	mainArrangement->arrangeItems();

	canvas.push_back(getPtr(mainArrangement));

	addBackground(loadList->getMaterial("UIRoundBox"));

	isSetup = true;
}

void TextureSettings::getUVPos(glm::vec4& positions) {
	positions[0] = 2 * imagePanel->UVextentx;
	positions[1] = (imagePanel->posx) - imagePanel->UVextentx;
	positions[2] = 2 * imagePanel->extenty;
	positions[3] = (imagePanel->posy) - imagePanel->extenty;
}

void TextureSettings::updateWebcamTex() {
	if (imageName == std::string("Webcam View")) {
		imagePanel->image->mat[0] = loadList->replacePtr(new Material(textureLL->getTexture("Webcam View"), "UI"), "Image View");
	}
}

void TextureSettings::functionMap(UIItem* owner) {
	if (functions.count(owner->text) > 0 && functions.at(owner->text) != nullptr) {
		owner->Name = imageName;
		functions.at(owner->text)(owner);
	}
	selfDestruct(owner);
}

void WebcamSettings::setup(std::function<void(UIItem*)> finishCallback, std::function<void()> reloadCallback) {
	if (isSetup) {
		return;
	}

	reload = reloadCallback;

	std::function<void(UIItem*)> webcamCalib = bind(&WebcamSettings::calibrateWebcam, this, std::placeholders::_1);

	std::function<void(UIItem*)> addRot = bind(&WebcamSettings::addRotation, this, std::placeholders::_1);
	std::function<void(UIItem*)> subtractRot = bind(&WebcamSettings::subtractRotation, this, std::placeholders::_1);

	std::function<void(UIItem*)> idUp = bind(&WebcamSettings::indexUp, this, std::placeholders::_1);
	std::function<void(UIItem*)> idDown = bind(&WebcamSettings::indexDown, this, std::placeholders::_1);

	std::function<void(UIItem*)> revertAR = bind(&WebcamSettings::revertAspectRatio, this, std::placeholders::_1);

	Arrangement* mainArrangement = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.4f, 0.6f, 0.01f, ARRANGE_START);

	Arrangement* endButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.05f, 0.01f);
	Arrangement* idButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.05f, 0.01f);
	Arrangement* rotationButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.05f, 0.01f);
	Arrangement* ARSettings = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.1f, 0.05f);

	webcamView = new ImagePanel(loadList->getPtr(new Material(webcamTexture::get(), "UI"), "Webcam View Mat"), true);

	Material* settingsMat = loadList->getMaterial("SettingsBtnMat");
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
	endButtons->addItem(getPtr(new Button(loadList->getFont(), loadList->getMaterial("UIRoundBox"), "FINISH", finishCallback)));

	ratioSlider = new Slider(ORIENT_HORIZONTAL, visibleMat, 0.0f, 0.0f, 1.0f, 0.1f);
	ratioSlider->setFloatCallback(std::bind(&WebcamSettings::updateAspectRatio, this, std::placeholders::_1), true);
	ratioSlider->setSlideValues(0.5f, 2.0f, webcamTexture::get()->webCam->sizeRatio);

	ARSettings->addItem(getPtr(ratioSlider));
	ARSettings->addItem(getPtr(new Button(rotBackward, revertAR)));

	ARSettings->arrangeItems();

	mainArrangement->addItem(getPtr(webcamView));
	mainArrangement->addItem(getPtr(ARSettings));
	mainArrangement->addItem(getPtr(idButtons));
	mainArrangement->addItem(getPtr(rotationButtons));
	mainArrangement->addItem(getPtr(endButtons));
	mainArrangement->arrangeItems();

	canvas.push_back(getPtr(mainArrangement));

	if (webcamTexture::get()->webCam != nullptr) {
		webcamTexture::get()->webCam->shouldUpdate = true;
	}

	addBackground(loadList->getMaterial("UIRoundBox"));

	isSetup = true;
}

void WebcamSettings::customUpdate() {
	loadList->updateWebcamMaterials();
	textureLL->updateWebcamMaterials();
}

void WebcamSettings::calibrateWebcam(UIItem* owner) {
	if (webcamTexture::get()->webCam != nullptr) {
		webcamTexture::get()->webCam->calibrateCornerFilter();
	}
}

void WebcamSettings::addRotation(UIItem* owner) {
	webcamTexture::get()->webCam->setRotation(true);
	webcamTexture::get()->recreateWebcamImage();
	ratioSlider->setSlideValues(0.5f, 2.0f, webcamTexture::get()->webCam->sizeRatio);
	update();
}

void WebcamSettings::subtractRotation(UIItem* owner) {
	webcamTexture::get()->webCam->setRotation(false);
	webcamTexture::get()->recreateWebcamImage();
	ratioSlider->setSlideValues(0.5f, 2.0f, webcamTexture::get()->webCam->sizeRatio);
	update();
}

void WebcamSettings::indexUp(UIItem* owner) {
	webcamTexture::get()->webCam->switchWebcam(true);
	webcamTexture::get()->recreateWebcamImage();
	ratioSlider->setSlideValues(0.5f, 2.0f, webcamTexture::get()->webCam->sizeRatio);
	update();
}

void WebcamSettings::indexDown(UIItem* owner) {
	webcamTexture::get()->webCam->switchWebcam(false);
	webcamTexture::get()->recreateWebcamImage();
	ratioSlider->setSlideValues(0.5f, 2.0f, webcamTexture::get()->webCam->sizeRatio);
	update();
}

void WebcamSettings::revertAspectRatio(UIItem* owner) {
	cv::Mat testFrame = webcamTexture::get()->webCam->getTestFrame();
	float aspectRatio = static_cast<float>(testFrame.size().width) / static_cast<float>(testFrame.size().height);
	webcamTexture::get()->webCam->updateAspectRatio(aspectRatio);
	webcamTexture::get()->recreateWebcamImage();
	ratioSlider->setSlideValues(0.5f, 2.0f, webcamTexture::get()->webCam->sizeRatio);
	update();
}

void WebcamSettings::updateAspectRatio(float newRatio) {
	webcamTexture::get()->webCam->updateAspectRatio(newRatio);
	webcamTexture::get()->recreateWebcamImage();
	update();
}

void RenderMenu::setup(std::function<void(UIItem*)> loadObjectFunct, std::function<void(UIItem*)> pipelinefunction, std::function<void(float)> polarCallback, std::function<void(float)> azimuthCallback) {
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

void RenderMenu::hideViewButtons() {
	canvas[0]->Items[0]->setVisibility(false);
	canvas[0]->Items[0]->setIsEnabled(false);
	update();
}

void RenderMenu::showViewButtons() {
	canvas[0]->Items[0]->setVisibility(true);
	canvas[0]->Items[0]->setIsEnabled(true);
	update();
}

void MaterialCreator::setup(std::string sName, GraphicsPass* bPass, std::function<void(UIItem*)> callback) {
	if (isSetup) {
		return;
	}
	autoIndices.clear();
	shaderName = sName;
	boundPass = bPass;
	matTemplate = new MaterialTemplate(shaderName, boundPass);

	finishedCallback = callback;

	Arrangement* totalArrangement = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.25f, 0.8f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

	Material* renderedMat = loadList->getMaterial("RenderBtnMat");
	Material* visibleMat = loadList->getMaterial("TestCheckBtnMat");

	int index = 0;
	int optionIndex = 0;

	std::vector<std::string> materialOptions{};
	std::vector<std::string> invalidMatOptions = { "UI", "UIGray", "UIText", "UV", "W", "UIRoundBox", "Projector_BF", "Projector_Flat", "Projector_Background" };
	for (std::pair<std::string, uint32_t> elem : boundPass->pipelineMap) {
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

	TextBox* matNameTB = new TextBox(loadList->getFont(), 0.0f, 0.0f, 4.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER, true);
	matNameTB->addText(newMaterialName);
	matNameTB->setClickFunction(std::bind(&MaterialCreator::setMaterialName, this, std::placeholders::_1));

	DropdownMenu* materialSelect = new DropdownMenu(0.0f, 0.0f, 5.0f, 1.0f, renderedMat, visibleMat, loadList->getMaterial("UIRoundBox"), loadList->getFont());
	materialSelect->addOptions(materialOptions);
	materialSelect->setBlankText("Select material");
	materialSelect->setSelectCallback(std::bind(&MaterialCreator::createMatOptionsMenu, this, std::placeholders::_1));

	totalArrangement->addItem(getPtr(matNameTB));
	totalArrangement->addItem(getPtr(materialSelect));
	totalArrangement->arrangeItems();

	canvas.push_back(getPtr(totalArrangement));

	addBackground(loadList->getMaterial("UIRoundBox"));

	isSetup = true;
}

void MaterialCreator::setupEditMode(std::string sName, GraphicsPass* bPass, std::function<void(UIItem*)> callback, std::string MatName) {
	if (isSetup) {
		return;
	}
	autoIndices.clear();
	shaderName = sName;
	boundPass = bPass;
	matTemplate = new MaterialTemplate(shaderName, boundPass);

	finishedCallback = callback;

	Arrangement* totalArrangement = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.25f, 0.8f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

	newMaterialName = MatName;
	std::string shaderName = textureLL->getMaterial(newMaterialName)->shaderName;

	UIItem* empty = new spacer();
	empty->text = shaderName;

	TextBox* matNameTB = new TextBox(loadList->getFont(), 0.0f, 0.0f, 4.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
	matNameTB->addText(newMaterialName);

	totalArrangement->addItem(getPtr(matNameTB));

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

	addBackground(loadList->getMaterial("UIRoundBox"));

	isSetup = true;
}

void MaterialCreator::createMatOptionsMenu(UIItem* owner) {
	if (canvas[0]->Items.size() > 1) {
		for (int i = 1; i != canvas[0]->Items.size(); i++) {
			canvas[0]->Items[i]->cleanup();
		}
		if (canvas[0]->Items.size() > 1) {
			canvas[0]->Items.erase(canvas[0]->Items.begin() + 1, canvas[0]->Items.end());
		}
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
	canvas[0]->addItem(getPtr(new Button(loadList->getFont(), loadList->getMaterial("UIRoundBox"), "FINISH", exitCallback)));
	canvas[0]->arrangeItems();
}

void MaterialCreator::setTexCallback(UIItem* owner) {
	std::string channel = owner->Name;
	std::string selectedTexture = owner->text;

	matTemplate->setTexture(channel, textureLL->getTexture(selectedTexture));
}

void MaterialCreator::exit(UIItem* owner) {
	Material* mat = matTemplate->createMaterial();
	Material* flatMat = matTemplate->createFlatMaterial();
	textureLL->replacePtr(mat, newMaterialName);
	textureLL->replacePtr(flatMat, newMaterialName + "_flat");
	owner->Name = newMaterialName;
	owner->text = shaderName;
	if (finishedCallback != nullptr) {
		finishedCallback(owner);
	}
	selfDestruct(owner);
}

void ObjectSettingsMenu::setup(StaticObject* object, std::function<void(UIItem*)> addTex, std::function<void(std::function<void(UIItem*)>)> newMatFunc, std::function<void(UIItem*)> closeMatFunc, std::function<void(std::function<void(UIItem*)>, std::string)> editMatFunc) {
	if (isSetup) {
		return;
	}

	obj = object;

	OSNormName = obj->objectName + "_OSNorm";

	openEditMaterialMenu = editMatFunc;
	openMaterialMenu = newMatFunc;
	closeMaterialMenu = closeMatFunc;

	addTextureFunc = addTex;

	Arrangement* totalArrangement = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.25f, 0.8f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

	Arrangement* materialSettingsArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 4.0f, 1.0f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

	TextBox* matLabel = new TextBox(loadList->getFont(), 0.0f, 0.0f, 3.0f, 1.0f, 24, ARRANGE_START, ARRANGE_CENTER);
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

	int index = 0;
	int optionIndex = -1;

	std::vector<std::string> existingMaterials{};
	textureLL->listMaterials(existingMaterials);

	std::vector<uint32_t> removeIndices{};

	for (uint32_t i = 0; i != existingMaterials.size(); i++) {
		std::string elem = existingMaterials[i];
		if (elem.substr(elem.size() - 5, elem.size() - 1) == "_flat") {
			removeIndices.push_back(i);
		}
		if (elem == obj->materialName && optionIndex == -1) {
			optionIndex = i;
		}
	}

	std::sort(removeIndices.begin(), removeIndices.end(), [](uint32_t a, uint32_t b) {return a > b; });
	for (uint32_t i : removeIndices) {
		existingMaterials.erase(existingMaterials.begin() + i);
	}

	DropdownMenu* materialSelect = new DropdownMenu(0.0f, 0.0f, 4.0f, 1.0f, renderedMat, visibleMat, loadList->getMaterial("UIRoundBox"), loadList->getFont());
	materialSelect->addOptions(existingMaterials);
	if (optionIndex != -1) {
		materialSelect->setOptionIndex(optionIndex);
	}
	materialSelect->setSelectCallback(std::bind(&ObjectSettingsMenu::selectMaterialCallback, this, std::placeholders::_1));

	matSelPtr = getPtr(materialSelect);

	totalArrangement->addItem(matSelPtr);

	Arrangement* genNormArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 5.0f, 1.0f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);
	TextBox* genNormText = new TextBox(loadList->getFont(), 0.0f, 0.0f, 3.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
	genNormText->addText("Generate OS normal:");

	genNormArrangement->addItem(getPtr(genNormText));
	genNormArrangement->addItem(getPtr(new spacer()));
	genNormArrangement->addItem(getPtr(new Button(plusMat, std::bind(&ObjectSettingsMenu::createNormalMap, this, std::placeholders::_1))));

	totalArrangement->addItem(getPtr(genNormArrangement));

	totalArrangement->addItem(getPtr(new Button(loadList->getFont(), loadList->getMaterial("UIRoundBox"), "FINISH", std::bind(&ObjectSettingsMenu::selfDestruct, this, std::placeholders::_1))));
	totalArrangement->arrangeItems();

	canvas.push_back(getPtr(totalArrangement));
	
	addBackground(loadList->getMaterial("UIRoundBox"));

	isSetup = true;
}

void ObjectSettingsMenu::newMaterialMenu(UIItem* owner) {
	std::function<void(UIItem*)> closeFnc = std::bind(&ObjectSettingsMenu::exitMaterialMenu, this, std::placeholders::_1);
	if (openMaterialMenu != nullptr) {
		openMaterialMenu(closeFnc);
	}
}

void ObjectSettingsMenu::editMaterialMenu(UIItem* owner) {
	if (owner->Name == std::string("Webcam Material")) {
		return;
	}
	std::function<void(UIItem*)> closeFnc = std::bind(&ObjectSettingsMenu::exitMaterialMenu, this, std::placeholders::_1);
	if (openEditMaterialMenu != nullptr) {
		openEditMaterialMenu(closeFnc, owner->Name);
	}
}

void ObjectSettingsMenu::exitMaterialMenu(UIItem* owner) {
	obj->mat = textureLL->getMaterial(owner->Name);
	obj->unlitMat = textureLL->getMaterial(owner->Name + "_flat");
	obj->shaderName = owner->text;
	obj->materialName = owner->Name;

	DropdownMenu* ddMenu = static_cast<DropdownMenu*>(matSelPtr);
	std::vector<std::string> options = ddMenu->options;
	if (find(options.begin(), options.end(), obj->materialName) == options.end()) {
		ddMenu->addOption(obj->materialName);
		options = static_cast<DropdownMenu*>(matSelPtr)->options;
	}

	auto it = find(options.begin(), options.end(), obj->materialName);
	if (it != options.end()) {
		ddMenu->setOptionIndex(it - options.begin());
	}

	settingsButton->Name = obj->materialName;

	if (closeMaterialMenu != nullptr) {
		closeMaterialMenu(owner);
	}
}

void ObjectSettingsMenu::selectMaterialCallback(UIItem* owner) {
	obj->mat = textureLL->getMaterial(owner->text);
	obj->shaderName = obj->mat->shaderName;
	obj->materialName = owner->text;
	settingsButton->Name = obj->materialName;
}

void ObjectSettingsMenu::createNormalMap(UIItem* owner) {

	NormalGen generator(textureLL);
	generator.setupOSExtractor();
	Texture* OSNormTex = generator.drawOSMap(obj->mesh);

	OSNormTex->isNormal = true;
	OSNormTex->normalType = false;

	uint8_t existsIndex = 0;
	std::string modifiedOSNormName = OSNormName;

	while (textureLL->checkForTexture(modifiedOSNormName)) {
		modifiedOSNormName = OSNormName + '_' + std::to_string(existsIndex);
		existsIndex++;
	}
	textureLL->getPtr(OSNormTex, modifiedOSNormName);

	if (addTextureFunc != nullptr) {
		owner->Name = modifiedOSNormName;
		addTextureFunc(owner);
	}

	generator.cleanupGenOS();
}

void ObjectMenu::setup(std::function<void(UIItem*)> loadObjectFunct) {
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

	addBackground(loadList->getMaterial("UIRoundBox"));

	isSetup = true;
}

void ObjectMenu::addObject(std::function<void(UIItem*)> toggleFunction, std::function<void(UIItem*)> wireframeToggle, std::function<void(UIItem*)> optionsMenu, std::string nameString = "Object Name") {
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

void ObjectMenu::clearObjects() {
	bool isFirstItem = true;
	for (UIItem* item : ObjectButtons->Items) {
		if (!isFirstItem) {
			isFirstItem = true;
			continue;
		}
		item->image->cleanup();
	}
	if (ObjectButtons->Items.size() > 1) {
		ObjectButtons->Items.erase(ObjectButtons->Items.begin() + 1, ObjectButtons->Items.end());
	}
	ObjectMap.clear();
	ObjectButtons->arrangeItems();
}

void NormalMixer::setup(std::string tName, std::vector<std::string> objects, std::function<Mesh* (std::string)> getMeshFunc, std::function<void(UIItem*)> exitFunc, std::function<void(UIItem*)> cancel) {
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

	Arrangement* totalArrangement = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.25f, 0.8f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

	TextBox* topText = new TextBox(loadList->getFont(), 0.0f, 0.0f, 5.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
	topText->addText("Mixing " + textureName + " with:");

	totalArrangement->addItem(getPtr(topText));

	DropdownMenu* textureSelect = new DropdownMenu(0.0f, 0.0f, 5.0f, 1.0f, invisibleMat, visibleMat, loadList->getMaterial("UIRoundBox"), loadList->getFont());
	textureSelect->setBlankText("Select a texture:");
	textureSelect->addOptions(textures);
	textureSelect->setSelectCallback(std::bind(&NormalMixer::setMixTex, this, std::placeholders::_1));

	totalArrangement->addItem(getPtr(textureSelect));

	TextBox* midText = new TextBox(loadList->getFont(), 0.0f, 0.0f, 5.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
	midText->addText("Based on object:");

	totalArrangement->addItem(getPtr(midText));

	DropdownMenu* objSelect = new DropdownMenu(0.0f, 0.0f, 5.0f, 1.0f, invisibleMat, visibleMat, loadList->getMaterial("UIRoundBox"), loadList->getFont());
	objSelect->setBlankText("Select an object");
	objSelect->addOptions(objects);
	objSelect->setSelectCallback(std::bind(&NormalMixer::setObject, this, std::placeholders::_1));

	totalArrangement->addItem(getPtr(objSelect));

	Arrangement* finishArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 5.0f, 1.0f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);
	Button* cancelBtn = new Button(loadList->getFont(), loadList->getMaterial("UIRoundBox"), "CANCEL", cancel);
	cancelBtn->Name = tName;

	finishArrangement->addItem(getPtr(cancelBtn));
	finishArrangement->addItem(getPtr(new spacer()));
	finishArrangement->addItem(getPtr(new Button(loadList->getFont(), loadList->getMaterial("UIRoundBox"), "FINISH", std::bind(&NormalMixer::mixNorms, this, std::placeholders::_1))));

	totalArrangement->addItem(getPtr(finishArrangement));
	totalArrangement->arrangeItems();

	canvas.push_back(getPtr(totalArrangement));

	addBackground(loadList->getMaterial("UIRoundBox"));

	isSetup = true;
}

void NormalMixer::createRefNormal(std::string& texName) {
	Mesh* mesh = getMesh(objectName);

	NormalGen generator(textureLL);
	generator.setupOSExtractor();
	Texture* OSNormTex = generator.drawOSMap(mesh);

	OSNormTex->isNormal = true;
	OSNormTex->normalType = false;

	uint8_t existsIndex = 0;
	std::string modifiedOSNormName = "TempOSNorm";

	while (textureLL->checkForTexture(modifiedOSNormName)) {
		modifiedOSNormName = "TempOSNorm_" + std::to_string(existsIndex);
		existsIndex++;
	}
	textureLL->getPtr(OSNormTex, modifiedOSNormName);

	createdTextures.push_back(modifiedOSNormName);

	generator.cleanupGenOS();

	texName = modifiedOSNormName;
}

void NormalMixer::changeNormalMapType(std::string tName) {
	if (getMesh == nullptr) {
		return;
	}

	Texture* norm = textureLL->getTexture(tName);
	std::string outNormalName = tName + "_OS";

	Mesh* mesh = getMesh(objectName);

	NormalGen generator(loadList);
	Texture* outNorm = nullptr;

	if (norm->normalType) {
		generator.copyTSImage(norm);
		generator.setupOSConverter();
		outNorm = generator.convertTStoOS(mesh);
	}
	else {
		generator.copyOSImage(norm);
		generator.setupTSConverter();
		outNorm = generator.convertOStoTS(mesh);
	}

	outNorm->isNormal = true;
	outNorm->normalType = !norm->normalType;

	textureLL->replacePtr(outNorm, outNormalName);

	createdTextures.push_back(outNormalName);

	if (norm->normalType) {
		generator.cleanupTStoOS();
	}
	else {
		generator.cleanupOStoTS();
	}
}

void NormalMixer::setMixTex(UIItem* owner) {
	mixTexName = owner->text;
}

void NormalMixer::setObject(UIItem* owner) {
	objectName = owner->text;
}

void NormalMixer::mixNorms(UIItem* owner) {
	if (objectName == "" && mixTexName == "") {
		if (errorFunc != nullptr) {
			errorFunc("SELECT ERROR: ", "No items have been selected!");
		}
		return;
	}
	else if (objectName == "") {
		if (errorFunc != nullptr) {
			errorFunc("SELECT ERROR: ", "No reference object has been selected!");
		}
		return;
	}
	else if (mixTexName == "") {
		if (errorFunc != nullptr) {
			errorFunc("SELECT ERROR: ", "No mix texture has been selected!");
		}
		return;
	}

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

	Texture* mixedNorm = normMixer.filterTarget[0]->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1);
	filter OS_EdgeFill(std::vector<Texture*>({ normMixer.filterTarget[0], mixedNorm }), new EDGEFILLSHADER, VK_FORMAT_R8G8B8A8_UNORM);
	OS_EdgeFill.filterImage();

	mixedNorm->isNormal = true;
	mixedNorm->normalType = resultNormalType;

	mixedNorm->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

	textureLL->replacePtr(mixedNorm, textureName);

	OS_EdgeFill.cleanup();
	normMixer.cleanup();
	for (std::string tName : createdTextures) {
		textureLL->deleteTexture(tName);
	}

	if (resultNormalType) {
		changeNormalMapType(textureName);
	}

	owner->Name = textureName;
	if (exitCallback != nullptr) {
		exitCallback(owner);
	}
	selfDestruct(owner);
}

void SpaceTransitionMenu::setup(std::string tName, std::vector<std::string> objects, std::function<void(UIItem*)> addTexFunc, std::function<Mesh* (std::string)> getMeshFunc, std::function<void(UIItem*)> exitFunc) {
	if (isSetup || objects.size() == 0) {
		return;
	}
	texName = tName;

	addTextureFunc = addTexFunc;
	getMesh = getMeshFunc;
	exitCallback = exitFunc;

	Arrangement* totalArrangement = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.25f, 0.8f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

	Material* invisibleMat = loadList->getMaterial("UnrenderedBtnMat");
	Material* visibleMat = loadList->getMaterial("TestCheckBtnMat");
	Material* renderedMat = loadList->getMaterial("RenderBtnMat");

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
	finishArrangement->addItem(getPtr(new Button(loadList->getFont(), loadList->getMaterial("UIRoundBox"), "FINISH", std::bind(&SpaceTransitionMenu::exit, this, std::placeholders::_1))));

	totalArrangement->addItem(getPtr(objectSelector));
	totalArrangement->addItem(getPtr(replaceArrangement));
	totalArrangement->addItem(getPtr(finishArrangement));

	totalArrangement->arrangeItems();

	canvas.push_back(getPtr(totalArrangement));

	addBackground(loadList->getMaterial("UIRoundBox"));

	isSetup = true;
}

void SpaceTransitionMenu::setObjName(UIItem* owner) {
	objectName = owner->text;
}

void SpaceTransitionMenu::exit(UIItem* owner) {
	changeNormalMapType(owner);
	owner->Name = outNormalName;
	if (exitCallback != nullptr) {
		exitCallback(owner);
	}
	selfDestruct(owner);
}

void SpaceTransitionMenu::changeNormalMapType(UIItem* owner) {
	if (getMesh == nullptr) {
		return;
	}

	overwrite = overwriteToggle->activestate;

	Texture* norm = textureLL->getTexture(texName);
	outNormalName = "";

	bool normalType = norm->normalType;

	if (overwrite) {
		outNormalName = texName;
	}
	else {
		if (normalType) {
			outNormalName = texName + "_OS";
		}
		else {
			outNormalName = texName + "_TS";
		}
	}

	Mesh* mesh = getMesh(objectName);

	NormalGen generator(loadList);
	Texture* outNorm = nullptr;

	if (normalType) {
		generator.copyTSImage(norm);
		generator.setupOSConverter();
		outNorm = generator.convertTStoOS(mesh);
	}
	else {
		generator.copyOSImage(norm);
		generator.setupTSConverter();
		outNorm = generator.convertOStoTS(mesh);
	}

	outNorm->isNormal = true;
	outNorm->normalType = !normalType;

	if (addTextureFunc != nullptr && !textureLL->checkForTexture(outNormalName)) {
		owner->Name = outNormalName;
		addTextureFunc(owner);
	}

	textureLL->replacePtr(outNorm, outNormalName);

	if (normalType) {
		generator.cleanupTStoOS();
	}
	else {
		generator.cleanupOStoTS();
	}
}

void TextureLoadMenu::setup(std::function<void(UIItem*)> update) {
	if (isSetup) {
		return;
	}

	updateTextureMenu = update;

	fileName = winFile::OpenFileDialog();
	if (fileName == std::string("fail")) {
		return; // We will need to check if this menu has been setup after the setup function is called otherwise we will have some draw errors
	}
	textureName = winFile::getBaseName(fileName);
	textureExt = winFile::getExtension(textureName);
	winFile::stripExtension(textureName);

	if (errorFunc != nullptr) {
		cv::Mat test = cv::imread(fileName, cv::IMREAD_UNCHANGED);
		if (test.empty()) {
			errorFunc("FILE ERROR:", std::string("File extension \".") + textureExt + std::string("\" is not currently supported for textures"));
			return;
		}
	}

	Arrangement* mainArrangement = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.25f, 0.8f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

	font* newFont = loadList->getFont();
	TextBox* TexLoadLabel = new TextBox(newFont, 0.0f, 0.0f, 5.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
	TexLoadLabel->addText(std::string("Loading ") + textureName);

	mainArrangement->addItem(getPtr(TexLoadLabel));

	std::vector<std::string> loadOptions{ "Colour", "Normalized vector" };

	Material* renderedMat = loadList->getMaterial("RenderBtnMat");
	Material* visibleMat = loadList->getMaterial("TestCheckBtnMat");
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
	finishButtons->addItem(getPtr(new Button(loadList->getFont(), loadList->getMaterial("UIRoundBox"), "FINISH", std::bind(&TextureLoadMenu::finishCallback, this, std::placeholders::_1))));

	mainArrangement->addItem(getPtr(finishButtons));

	mainArrangement->arrangeItems();

	canvas.push_back(getPtr(mainArrangement));

	addBackground(loadList->getMaterial("UIRoundBox"));

	isSetup = true;
}

void TextureLoadMenu::setOptionCallback(UIItem* owner) {
	if (owner->text == std::string("Colour")) {
		isNormalized = false;
		normalTypeToggle->setVisibility(false);
	}
	else {
		isNormalized = true;
		normalTypeToggle->setVisibility(true);
	}
}

void TextureLoadMenu::finishCallback(UIItem* owner) {
	imageTexture* loadedTexture = nullptr;
	try {
		if (isNormalized) {
			cv::Mat initMat = cv::imread(fileName, cv::IMREAD_UNCHANGED);
			loadedTexture = new imageTexture(initMat, VK_FORMAT_R8G8B8A8_UNORM);
			loadedTexture->isNormal = true;
			normalType = !normalTypeToggle->activestate;
			loadedTexture->normalType = normalType;
		}
		else {
			cv::Mat initMat = cv::imread(fileName, cv::IMREAD_UNCHANGED);
			loadedTexture = new imageTexture(initMat, VK_FORMAT_R8G8B8A8_SRGB);
		}
		textureLL->replacePtr(loadedTexture, textureName);
		owner->Name = textureName;
		owner->text = textureExt;
		if (updateTextureMenu != nullptr) {
			updateTextureMenu(owner);
		}
		selfDestruct(owner);
	}
	catch (...) {
		if (errorFunc != nullptr) {
			errorFunc("FILE ERROR:", std::string("File extension \".") + textureExt + std::string("\" is not currently supported for textures"));
		}
		selfDestruct(owner);
	}
}

void TextureMenu::setup(std::function<void(UIItem*)> modCallback, std::function<void(std::function<void(UIItem*)>)> loadCallback, std::function<void(UIItem*)> tomogCallback) {
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

	addBackground(loadList->getMaterial("UIRoundBox"));

	isSetup = true;
}

void TextureMenu::loadTexture(UIItem* owner) {
	if (loadTextureCallback != nullptr) {
		loadTextureCallback(std::bind(&TextureMenu::addTexture, this, std::placeholders::_1));
	}
}

void TextureMenu::addTexture(UIItem* owner) {
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

	update();
	measureWindowPositions();
}

void TextureMenu::saveTexture(UIItem* owner) {
	std::string textureName = owner->Name;

	std::string saveName = winFile::SaveFileDialog();
	if (saveName != std::string("fail")) {
		Texture* texToSave = textureLoadList->getTexture(textureName);
		Texture* sRGBTex = nullptr;
		if (texToSave->textureFormat == VK_FORMAT_R8G8B8A8_UNORM) {
			sRGBTex = texToSave->copyTexture(VK_FORMAT_R8G8B8A8_SRGB, texToSave->textureLayout, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, texToSave->textureTiling, 0);
		}
		uint32_t lastDotIndex = saveName.size();
		if (saveName.size() > 1) {
			char dot = '.';
			for (uint32_t i = 0; i != saveName.size(); i++) {
				uint32_t j = saveName.size() - i - 1;
				char c = saveName[j];
				if (c == dot) {
					lastDotIndex = i - 1;
					break;
				}
				else if (c == '/' || c == '\\') {
					break;
				}
			}
		}
		if (lastDotIndex == saveName.size()) {
			saveName += ".png";
		}
		if (sRGBTex == nullptr) {
			texToSave->getCVMat();
			imwrite(saveName, texToSave->texMat);
		}
		else {
			sRGBTex->getCVMat();
			imwrite(saveName, sRGBTex->texMat);
		}
		texToSave->destroyCVMat();
		if (sRGBTex != nullptr) {
			sRGBTex->cleanup();
			delete sRGBTex;
		}
	}
}

void WebcamMenu::setup(std::function<void(UIItem*)> lightingFunction, std::function<void(UIItem*)> openSettings) {
	if (isSetup) {
		return;
	}
	Material* renderedMat = loadList->getMaterial("RenderBtnMat");
	Material* unrenderedMat = loadList->getMaterial("UnrenderedBtnMat");
	Material* playMat = loadList->getMaterial("PlayBtnMat");
	Material* pauseMat = loadList->getMaterial("PauseBtnMat");
	Material* settingsMat = loadList->getMaterial("SettingsBtnMat");
	Material* webcamMat = loadList->getMaterial("WebcamOnBtnMat");

	std::function<void(UIItem*)> toggleWebcamFunct = bind(&WebcamMenu::toggleWebcam, this, std::placeholders::_1);

	Arrangement* Videobuttons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 1.0f, 0.2f, 0.05f, 0.01f, ARRANGE_CENTER);

	Videobuttons->addItem(getPtr(new Button(webcamMat)));
	Videobuttons->addItem(getPtr(new Checkbox(playMat, pauseMat, toggleWebcamFunct)));
	Videobuttons->addItem(getPtr(new Button(settingsMat, openSettings)));
	Videobuttons->addItem(getPtr(new Checkbox(renderedMat, unrenderedMat, lightingFunction)));

	Videobuttons->arrangeItems();

	canvas.push_back(getPtr(Videobuttons));

	isSetup = true;
}