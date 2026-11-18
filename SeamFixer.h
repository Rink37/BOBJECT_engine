#ifndef SEAMFIX
#define SEAMFIX

#include"Bobject_Engine.h"
#include"Materials.h"
#include"LoadLists.h"
#include"UIelements.h"
#include"Remapper.h"
#include"include/SeamFix_Colour.h"
#include"include/SeamFix_Alpha.h"

struct SeamStrip {
	std::vector<uint32_t> leftIndices{};
	std::vector<uint32_t> rightIndices{};

	Mesh leftMesh{};
	Mesh leftAlphaMesh{};

	Mesh rightMesh{};
	Mesh rightAlphaMesh{};

	bool rightClosed = false;
	bool leftClosed = false;

	bool isRight = true;
	bool directionLocked = false;
};

class SeamFixer {
public:
	SeamFixer(Mesh* mesh, LoadList* texAssets, std::string texName) {
		target = mesh;
		textureLL = texAssets;
		targetTexName = texName;
		targetTex = textureLL->getTexture(texName)->copyTexture();
		targetTex->getCVMat();

		colourWriter = new textureWriter(targetTex, new SEAMFIX_COLOURSHADER, true);
		alphaWriter = new textureWriter(targetTex, new SEAMFIX_ALPHASHADER, true, false);
	}

	void updateSeamMeshes(float distance = 0.01f, bool updateColour = false, bool smoothOutside = false);

	void createSeamMeshes(SeamStrip&, float, bool, bool smoothOutside);
	void findAdjacentStrips();
	void getStripChain(SeamStrip&, uint32_t, std::vector<std::array<uint32_t, 2>>&, std::vector<uint32_t>&);

	void seamFixAll();
	void setSeamDirections(bool isRight) {
		for (SeamStrip& seam : seamStrips) {
			if (!seam.directionLocked) {
				seam.isRight = isRight;
			}
		}
		this->isRight = isRight;
	}

	void cleanup() {

		colourWriter->cleanup();
		alphaWriter->cleanup();
		delete colourWriter;
		delete alphaWriter;

		for (SeamStrip strip : seamStrips) {
			if (!strip.directionLocked) {
				strip.leftAlphaMesh.cleanup();
				strip.leftMesh.cleanup();
			}
			strip.rightAlphaMesh.cleanup();
			strip.rightMesh.cleanup();
		}

		seamStrips.clear();

		if (remapper != nullptr) {
			remapper->cleanup();
		}

		if (targetTex != nullptr) {
			targetTex->cleanup();
		}
	}

	void createRemapper() {
		if (targetTex == nullptr) {
			return;
		}
		useRemapper = true;
		remapper = new RemapBackend();
	}

	std::string targetTexName = "";
	Texture* targetTex = nullptr;

	void createDemoImages(std::string);

private:
	bool useRemapper = false;
	bool isRight = true;
	RemapBackend* remapper = nullptr;

	float stripSize = 0.0f;

	Mesh* target = nullptr;

	textureWriter* colourWriter = nullptr;
	textureWriter* alphaWriter = nullptr;

	LoadList* textureLL = nullptr;

	std::vector<SeamStrip> seamStrips{};
	std::vector<std::vector<uint32_t>> sortedSeamStrips{};
	std::vector<std::vector<Mesh*>> sortedLeftMeshes{};
	std::vector<std::vector<Mesh*>> sortedRightMeshes{};
	std::vector<std::vector<Mesh*>> sortedLeftAlphaMeshes{};
	std::vector<std::vector<Mesh*>> sortedRightAlphaMeshes{};

	uint32_t width = 0;
	uint32_t height = 0;

	void packSeamStrips();

	void sortSeamIndices(SeamStrip&);
	
	void alphaOverMap(Texture*, Texture*, Texture*) const;
};

class SeamFixMenu : public Widget {
public:
	SeamFixMenu(LoadList* assets, LoadList* texAssets) {
		loadList = assets;
		textureLL = texAssets;
	}

	void setup(Mesh* mesh, std::string texName, std::function<void(UIItem*)> exitFunct) {
		fixer = new SeamFixer(mesh, textureLL, texName);
		fixer->findAdjacentStrips();
		//fixer->seamFixAll();

		fixer->updateSeamMeshes(0.025f, true);
		fixer->updateSeamMeshes(0.01f);
		fixer->seamFixAll();

		exitFunc = exitFunct;

		Material* visibleMat = loadList->getMaterial("TestCheckBtnMat");
		Material* invisibleMat = loadList->getMaterial("UnrenderedBtnMat");

		outMap = getPtr(new ImagePanel(textureLL->replacePtr(new Material(textureLL->getTexture(texName), "UI"), "SeamFixMat"), false));

		Arrangement* column = new Arrangement(ORIENT_VERTICAL, 1.0f, 0.0f, 0.25f, 0.8f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		column->addItem(outMap);

		Arrangement* directionButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.2f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		TextBox* directionTB = new TextBox(loadList->getFont(), 0.0f, 0.0f, 4.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
		directionTB->addText("Seam direction:");
		
		directionButtons->addItem(getPtr(directionTB));

		directionButtons->addItem(getPtr(new Checkbox(visibleMat, invisibleMat, std::bind(&SeamFixMenu::swapDirection, this, std::placeholders::_1), false)));

		Arrangement* useRemapperButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.2f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		TextBox* remapperTB = new TextBox(loadList->getFont(), 0.0f, 0.0f, 4.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
		remapperTB->addText("Use remapper:");

		useRemapperButtons->addItem(getPtr(remapperTB));
		useRemapperButtons->addItem(getPtr(new Checkbox(visibleMat, invisibleMat, std::bind(&SeamFixMenu::activateRemapper, this, std::placeholders::_1), false)));

		column->addItem(getPtr(directionButtons));
		column->addItem(getPtr(useRemapperButtons));

		TextBox* sizeText = new TextBox(loadList->getFont(), 0.0f, 0.0f, 1.0f, 0.2f, 18, ARRANGE_START, ARRANGE_CENTER);
		sizeText->addText("Seam size:");

		column->addItem(getPtr(sizeText));

		Slider* sizeSlide = new Slider(ORIENT_HORIZONTAL, visibleMat, 0.0f, 0.0f, 1.0f, 0.2f);
		sizeSlide->setFloatCallback(std::bind(&SeamFixMenu::setSize, this, std::placeholders::_1), false);
		sizeSlide->setSlideValues(0.005f, 0.05f, 0.01f);
		column->addItem(getPtr(sizeSlide));

		Arrangement* endButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.2f, 0.01f, ARRANGE_END);

		Material* cancelMat = loadList->getMaterial("CancelBtnMat");
		Material* finishMat = loadList->getMaterial("FinishBtnMat");

		Button* finishButton = new Button(finishMat, std::bind(&SeamFixMenu::finish, this, std::placeholders::_1));
		Button* cancelButton = new Button(cancelMat, std::bind(&SeamFixMenu::cancel, this, std::placeholders::_1));

		endButtons->addItem(getPtr(cancelButton));
		endButtons->addItem(getPtr(finishButton));

		column->addItem(getPtr(endButtons));
		column->updateDisplay();

		canvas.push_back(getPtr(column));

		addBackground(loadList->getMaterial("UIRoundBox"));

		isSetup = true;
	}

	void cleanupSubClasses() {
		if (fixer != nullptr) {
			fixer->cleanup();
			delete fixer;
			fixer = nullptr;
		}
	}

private:
	LoadList* textureLL = nullptr;

	UIItem* outMap = nullptr;

	SeamFixer* fixer = nullptr;
	bool currentDirection = true;

	std::function<void(UIItem*)> exitFunc = nullptr;

	void activateRemapper(UIItem* owner) {
		fixer->createRemapper();
		fixer->seamFixAll();
	}

	void setSize(float distance) {
		fixer->updateSeamMeshes(distance, false);
		fixer->seamFixAll();
	}
	
	void swapDirection(UIItem* owner) {
		currentDirection = owner->activestate;
		fixer->setSeamDirections(currentDirection);
		fixer->seamFixAll();
	}

	void createDemoMaps(UIItem* owner) {
		fixer->createDemoImages(fixer->targetTexName);
	}

	void finish(UIItem* owner) {
		outMap->setVisibility(false);
		owner->Name = fixer->targetTexName;

		textureLL->deleteMaterial("SeamFixMat");

		fixer->cleanup();
		delete fixer;
		fixer = nullptr;
		if (exitFunc != nullptr) {
			exitFunc(owner);
		}
		selfDestruct(owner);
	}

	void cancel(UIItem* owner) {
		outMap->setVisibility(true);
		textureLL->replacePtr(fixer->targetTex->copyTexture(), fixer->targetTexName);

		owner->Name = fixer->targetTexName;

		textureLL->deleteMaterial("SeamFixMat");

		fixer->cleanup();
		delete fixer;
		fixer = nullptr;
		if (exitFunc != nullptr) {
			exitFunc(owner);
		}
		selfDestruct(owner);
	}
};

#endif