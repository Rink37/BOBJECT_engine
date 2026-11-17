#include"WidgetTemplates.h"

void ErrorDialog::setup(std::string title, std::string message) {
	if (isSetup) {
		return;
	}

	Arrangement* totalArrangement = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.25f, 0.8f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

	TextBox* titleBox = new TextBox(loadList->getFont(), 0.0f, 0.0f, 5.0f, 1.0f, 24, ARRANGE_START, ARRANGE_CENTER);
	titleBox->addText(title);
	TextBox* messageBox = new TextBox(loadList->getFont(), 0.0f, 0.0f, 5.0f, 2.0f, 18, ARRANGE_START, ARRANGE_START);
	messageBox->addText(message);

	totalArrangement->addItem(getPtr(titleBox));
	totalArrangement->addItem(getPtr(messageBox));

	Arrangement* finishArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 5.0f, 1.0f, 0.01f);
	finishArrangement->addItem(getPtr(new spacer));
	finishArrangement->addItem(getPtr(new Button(loadList->getMaterial("FinishBtnMat"), std::bind(&ErrorDialog::selfDestruct, this, std::placeholders::_1))));

	totalArrangement->addItem(getPtr(finishArrangement));

	canvas.push_back(totalArrangement);

	addBackground(loadList->getMaterial("UIRoundBox"));

	isSetup = true;
}

void TexSelector::setup(std::string targetTextureName, std::function<void(std::string, std::string)> callback) {
	if (isSetup) {
		return;
	}
	targetName = targetTextureName;

	continueCallback = callback;

	std::vector<std::string> availableTextures{};
	textureLL->listTextures(availableTextures);

	if (std::find(availableTextures.begin(), availableTextures.end(), targetName) == availableTextures.end()) {
		if (errorFunc != nullptr) {
			errorFunc("FILE ERROR: ", "The target texture is not valid!");
		}
		return;
	}

	if (headerText == "" || promptText == "") {
		std::cout << "Warning: text has not been set for the tex selector for " << targetTextureName << std::endl;
	}

	Material* renderedMat = loadList->getMaterial("RenderBtnMat");
	Material* visibleMat = loadList->getMaterial("TestCheckBtnMat");
	Material* finishedMat = loadList->getMaterial("FinishBtnMat");
	Material* cancelMat = loadList->getMaterial("CancelBtnMat");

	std::function<void(UIItem*)> finishFunct = std::bind(&TexSelector::finish, this, std::placeholders::_1);

	Arrangement* mainArrangement = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.25f, 0.8f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

	font* newFont = textureLL->getFont();

	TextBox* baseText = new TextBox(newFont, 0.0f, 0.0f, 5.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
	baseText->addText(headerText);

	TextBox* targetText = new TextBox(newFont, 0.0f, 0.0f, 5.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
	targetText->addText(promptText);

	DropdownMenu* refDropdown = new DropdownMenu(0.0f, 0.0f, 5.0f, 1.0f, renderedMat, visibleMat, loadList->getMaterial("UIRoundBox"), newFont);
	refDropdown->setBlankText("Select a texture");
	refDropdown->addOptions(availableTextures);
	refDropdown->setSelectCallback(std::bind(&TexSelector::setRefName, this, std::placeholders::_1));

	Arrangement* finishButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 5.0f, 2.0f, 0.01f);
	finishButtons->addItem(getPtr(new Button(cancelMat, std::bind(&TexSelector::selfDestruct, this, std::placeholders::_1))));
	finishButtons->addItem(getPtr(new spacer()));
	finishButtons->addItem(getPtr(new Button(finishedMat, finishFunct)));

	mainArrangement->addItem(getPtr(baseText));
	mainArrangement->addItem(getPtr(targetText));
	mainArrangement->addItem(getPtr(refDropdown));
	mainArrangement->addItem(getPtr(finishButtons));
	mainArrangement->arrangeItems();

	canvas.push_back(getPtr(mainArrangement));

	addBackground(loadList->getMaterial("UIRoundBox"));

	isSetup = true;
}

void TexSelector::finish(UIItem* owner) {
	if (targetName.size() == 0 || refName.size() == 0) {
		if (errorFunc != nullptr) {
			errorFunc("SELECT ERROR: ", "No reference texture has been selected!");
		}
		return;
	}

	if (continueCallback != nullptr) {
		continueCallback(refName, targetName);
	}
	selfDestruct(owner);
}