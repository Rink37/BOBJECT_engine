#ifndef SEAMFIX
#define SEAMFIX

#include"Bobject_Engine.h"
#include"Materials.h"
#include"LoadLists.h"
#include"UIelements.h"
#include"Remapper.h"

//#include"include/BakedImages.h"

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
		//targetTex->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	struct OverlayMap {
		VkFramebuffer frameBuffer = nullptr;
		VkRenderPass renderPass = nullptr;
		Texture* colour = nullptr;
		VkDescriptorPool descriptorPool = nullptr;
		VkDescriptorSet descriptorSet = nullptr;
		VkDescriptorSetLayout descriptorSetLayout = nullptr;
		VkPipelineLayout pipelineLayout = nullptr;
		VkPipeline pipeline = nullptr;

		void cleanup() {
			vkDeviceWaitIdle(Engine::get()->device);
			if (colour != nullptr) {
				colour->cleanup();
				delete colour;
			}
			if (descriptorSetLayout != nullptr) {
				vkDestroyDescriptorSetLayout(Engine::get()->device, descriptorSetLayout, nullptr);
				vkDestroyDescriptorPool(Engine::get()->device, descriptorPool, nullptr);
			}
			if (pipeline != nullptr) {
				vkDestroyPipeline(Engine::get()->device, pipeline, nullptr);
				vkDestroyPipelineLayout(Engine::get()->device, pipelineLayout, nullptr);
			}
			if (renderPass != nullptr) {
				vkDestroyRenderPass(Engine::get()->device, renderPass, nullptr);
			}
			if (frameBuffer != nullptr) {
				vkDestroyFramebuffer(Engine::get()->device, frameBuffer, nullptr);
			}
			cleaned = true;
		}

		bool cleaned = true;
	};

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

	void alphaOverRight() {
		vkDeviceWaitIdle(Engine::get()->device);
		drawRightMap();
		drawRightAlpha();
		alphaOverMap(true);
	}

	void alphaOverLeft() {
		vkDeviceWaitIdle(Engine::get()->device);
		drawLeftMap();
		drawLeftAlpha();
		alphaOverMap(false);
	}

	void drawRightMap() {
		if (rightMap.cleaned) {
			prepMap(&rightMap);
			prepareColourDescriptor(&rightMap);
			createTexWritePipeline(&rightMap);
		}
		VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
		commandBuffer = drawColourMap(commandBuffer, true);
		Engine::get()->endSingleTimeCommands(commandBuffer);
	}

	void drawLeftMap() {
		if (leftMap.cleaned) {
			prepMap(&leftMap);
			prepareColourDescriptor(&leftMap);
			createTexWritePipeline(&leftMap);
		}
		VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
		commandBuffer = drawColourMap(commandBuffer, false);
		Engine::get()->endSingleTimeCommands(commandBuffer);
	}

	void drawRightAlpha() {
		if (rightAlpha.cleaned) {
			prepMap(&rightAlpha);
			createAlphaWritePipeline(&rightAlpha);
		}
		VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
		commandBuffer = drawAlphaMap(commandBuffer, true);
		Engine::get()->endSingleTimeCommands(commandBuffer);
	}

	void drawLeftAlpha() {
		if (leftAlpha.cleaned) {
			prepMap(&leftAlpha);
			createAlphaWritePipeline(&leftAlpha);
		}
		VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
		commandBuffer = drawAlphaMap(commandBuffer, false);
		Engine::get()->endSingleTimeCommands(commandBuffer);
	}

	void cleanup() {
		leftMap.cleanup();
		leftAlpha.cleanup();
		rightMap.cleanup();
		rightAlpha.cleanup();

		colourMap.cleanup();
		alphaMap.cleanup();

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
	}

	void createRemapper() {
		if (targetTex == nullptr) {
			return;
		}
		useRemapper = true;
		if (alphaMap.cleaned) {
			prepMap(&alphaMap);
			createAlphaWritePipeline(&alphaMap);
		}
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

	OverlayMap colourMap{};
	OverlayMap alphaMap{};

	OverlayMap leftMap{};
	OverlayMap leftAlpha{};
	OverlayMap rightMap{};
	OverlayMap rightAlpha{};

	LoadList* textureLL = nullptr;

	std::vector<OverlayMap*> maps{ &leftMap, &rightMap };
	std::vector<OverlayMap*> alphaMaps{ &leftAlpha, &rightAlpha };

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
	void prepMap(OverlayMap*);
	void prepareColourDescriptor(OverlayMap*);
	void createTexWritePipeline(OverlayMap*);
	void createAlphaWritePipeline(OverlayMap*);
	VkCommandBuffer drawColourMap(VkCommandBuffer, bool);
	VkCommandBuffer drawColourMap(VkCommandBuffer, OverlayMap*, Mesh*);
	VkCommandBuffer drawColourMap(VkCommandBuffer, OverlayMap*, std::vector<Mesh*>);

	VkCommandBuffer drawAlphaMap(VkCommandBuffer, bool);
	VkCommandBuffer drawAlphaMap(VkCommandBuffer, OverlayMap*, Mesh*);
	VkCommandBuffer drawAlphaMap(VkCommandBuffer, OverlayMap*, std::vector<Mesh*>);

	void alphaOverMap(bool);
	void alphaOverMap(Texture*, Texture*, Texture*);
};

class SeamObjPicker : public Widget {
public:
	SeamObjPicker(LoadList* assets, LoadList* texLL) {
		loadList = assets;
		textureLL = texLL;
	}

	void setup(std::string tName, std::vector<std::string> objects, std::function<void(UIItem*)> exitFunc, std::function<void(UIItem*)> cancelFunc) {
		if (isSetup) {
			return;
		}

		textureName = tName;
		finishCallback = exitFunc;

		std::vector<std::string> textures{};
		textureLL->listTextures(textures);

		auto it = find(textures.begin(), textures.end(), textureName);
		if (it != textures.end()) {
			textures.erase(it);
		}

		Material* invisibleMat = loadList->getMaterial("UnrenderedBtnMat");
		Material* visibleMat = loadList->getMaterial("TestCheckBtnMat");
		Material* finishMat = loadList->getMaterial("FinishBtnMat");
		Material* cancelMat = loadList->getMaterial("CancelBtnMat");

		Arrangement* totalArrangement = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.25f, 0.8f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		TextBox* topText = new TextBox(loadList->getFont(), 0.0f, 0.0f, 5.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
		topText->addText("Seam-fixing " + textureName + " based on Object:");

		totalArrangement->addItem(getPtr(topText));

		DropdownMenu* objSelect = new DropdownMenu(0.0f, 0.0f, 5.0f, 1.0f, invisibleMat, visibleMat, loadList->getMaterial("UIRoundBox"), loadList->getFont());
		objSelect->setBlankText("Select an object");
		objSelect->addOptions(objects);
		objSelect->setSelectCallback(std::bind(&SeamObjPicker::setObject, this, std::placeholders::_1));

		totalArrangement->addItem(getPtr(objSelect));

		Arrangement* finishArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 5.0f, 1.0f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);
		Button* cancelBtn = new Button(cancelMat, cancelFunc);
		cancelBtn->Name = tName;
		
		finishArrangement->addItem(getPtr(cancelBtn));
		finishArrangement->addItem(getPtr(new spacer()));
		finishArrangement->addItem(getPtr(new Button(finishMat, std::bind(&SeamObjPicker::finishFunc, this, std::placeholders::_1))));

		totalArrangement->addItem(getPtr(finishArrangement));
		totalArrangement->arrangeItems();

		canvas.push_back(getPtr(totalArrangement));

		addBackground(loadList->getMaterial("UIRoundBox"));

		isSetup = true;
	}
private:
	LoadList* textureLL = nullptr;
	std::string objectName = "";

	std::string textureName = "";
	std::function<Mesh* (std::string)> getMesh = nullptr;
	std::function<void(UIItem*)> finishCallback = nullptr;

	void setObject(UIItem* owner) {
		objectName = owner->text;
	}

	void finishFunc(UIItem* owner) {
		if (objectName == "") {
			//std::cout << "No object has been selected" << std::endl;
			if (errorFunc != nullptr) {
				errorFunc("SELECT ERROR: ", "No base object has been selected!");
			}
			return;
		}
		owner->Name = textureName;
		owner->text = objectName;
		if (finishCallback != nullptr) {
			finishCallback(owner);
		}
	}
};

class SeamFixMenu : public Widget {
public:
	SeamFixMenu(LoadList* assets, LoadList* texAssets) {
		loadList = assets;
		textureLL = texAssets;
	}

	void setup(Mesh* mesh, std::string texName, std::function<void(UIItem*)> cancelFunct, std::function<void(UIItem*)> finishFunct) {
		fixer = new SeamFixer(mesh, textureLL, texName);
		fixer->findAdjacentStrips();
		//fixer->seamFixAll();

		fixer->updateSeamMeshes(0.025f, true);
		fixer->updateSeamMeshes(0.01f);
		fixer->seamFixAll();

		cancelFunc = cancelFunct;
		finishFunc = finishFunct;

		Material* visibleMat = loadList->getMaterial("TestCheckBtnMat");
		Material* invisibleMat = loadList->getMaterial("UnrenderedBtnMat");

		outMap = getPtr(new ImagePanel(textureLL->replacePtr(new Material(textureLL->getTexture(texName)), "SeamFixMat"), false));

		Arrangement* column = new Arrangement(ORIENT_VERTICAL, 1.0f, 0.0f, 0.25f, 0.8f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		column->addItem(outMap);

		Arrangement* directionButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.2f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		TextBox* directionTB = new TextBox(loadList->getFont(), 0.0f, 0.0f, 4.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
		directionTB->addText("Seam direction:");
		
		directionButtons->addItem(getPtr(directionTB));

		directionButtons->addItem(getPtr(new Checkbox(visibleMat, invisibleMat, std::bind(&SeamFixMenu::swapDirection, this, std::placeholders::_1))));

		column->addItem(getPtr(directionButtons));

		TextBox* sizeText = new TextBox(loadList->getFont(), 0.0f, 0.0f, 1.0f, 0.2f, 18, ARRANGE_START, ARRANGE_CENTER);
		sizeText->addText("Seam size:");

		column->addItem(getPtr(sizeText));

		Slider* sizeSlide = new Slider(ORIENT_HORIZONTAL, visibleMat, 0.0f, 0.0f, 1.0f, 0.2f);
		sizeSlide->setFloatCallback(std::bind(&SeamFixMenu::setSize, this, std::placeholders::_1), false);
		sizeSlide->setSlideValues(0.005f, 0.05f, 0.01f);

		column->addItem(getPtr(sizeSlide));

		Material* settingsMat = loadList->getMaterial("SettingsBtnMat");
		column->addItem(getPtr(new Button(settingsMat, std::bind(&SeamFixMenu::activateRemapper, this, std::placeholders::_1))));

		column->addItem(getPtr(new Button(settingsMat, std::bind(&SeamFixMenu::createDemoMaps, this, std::placeholders::_1))));

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

	std::function<void(UIItem*)> cancelFunc = nullptr;
	std::function<void(UIItem*)> finishFunc = nullptr;

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
		fixer->targetTex->cleanup();
		delete fixer->targetTex;

		owner->Name = fixer->targetTexName;

		textureLL->deleteMaterial("SeamFixMat");

		fixer->cleanup();
		delete fixer;
		fixer = nullptr;
		if (finishFunc != nullptr) {
			finishFunc(owner);
		}
	}

	void cancel(UIItem* owner) {
		outMap->setVisibility(true);
		textureLL->replacePtr(fixer->targetTex, fixer->targetTexName);

		owner->Name = fixer->targetTexName;

		textureLL->deleteMaterial("SeamFixMat");

		fixer->cleanup();
		delete fixer;
		fixer = nullptr;
		if (cancelFunc != nullptr) {
			cancelFunc(owner);
		}
	}
};

#endif