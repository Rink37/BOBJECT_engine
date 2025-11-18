#ifndef TOMOG
#define TOMOG

#include"ImageProcessor.h"
#include"Textures.h"
#include"LoadLists.h"
#include"UIelements.h"
#include"BakedImages.h"
#include"SurfaceConstructor.h"
#include"InputManager.h"
#include"WindowsFileManager.h"

struct TomogItem {
	Texture* baseImage = nullptr;
	Texture* correctedImage = nullptr;
	float rotation = 0.0f;
	std::vector<float> lightDirection = {0.0f, 0.0f, 0.0f};
	std::string name = "";

	~TomogItem() {
		if (baseImage != nullptr) {
			baseImage->cleanup();
		}
		if (correctedImage != nullptr) {
			correctedImage->cleanup();
		}
	}
};

class Tomographer {
public:
	// The assumption is that the normal and the template exist elsewhere
	// This class acts to modify a pre-existing surface Normal
	cv::Mat computedNormal;
	cv::Mat computedDiffuse;
	bool alignRequired = true;
	cv::Mat alignTemplate;

	cv::Size outdims;

	void add_image(std::string, std::string);
	void align(int);
	void add_lightVector(float phi, float theta, int idx);

	void remove_element(int);

	void calculate_normal();
	void calculate_diffuse();
	void calculate_NormAndDiff();

	void setLoadList(LoadList* ll) {
		loadList = ll;
	}

	void clearData() {
		//images.clear();
		//vectors.clear();
		items.clear();
		computedNormal.release();
		computedDiffuse.release();
		alignTemplate.release();
	}

	void cleanup() {
		clearData();
	}

	LoadList* loadList = nullptr;

	//std::vector<Texture*> images;
	std::vector<TomogItem*> items = {};
private:
	bool normalExists = false;
	//std::vector<Texture*> originalImages;
	
	//std::vector<std::vector<float>> vectors;
};

class TomographyLoad : public Widget {
public:
	TomographyLoad(LoadList* assets) {
		loadList = assets;
	}

	void setup(Material* loadedMat, std::function<void(Material*, float, float)> callback, std::function<void(UIItem*)> cancelCallback, std::function<void(UIItem*)> updateCallback) {
		if (isSetup) {
			return;
		}

		doneCallback = callback;

		outMat = loadedMat;

		ImagePanel* loadedUI = new ImagePanel(loadedMat, false);
		loadedUI->update(0.0f, 0.0f, 0.4f, 0.4f);
		loadedUI->updateDisplay();

		imageData tcb = TESTCHECKBOXBUTTON;
		Material* visibleMat = newMaterial(&tcb, "TestCheckBtn");

		imageData cancel = CANCELBUTTON;
		Material* cancelMat = newMaterial(&cancel, "CancelBtn");

		imageData finish = FINISHBUTTON;
		Material* finishMat = newMaterial(&finish, "FinishBtn");

		imageData update = UPDATEBUTTON;
		Material* updateMat = newMaterial(&update, "UpdateBtn");

		Arrangement* column = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.4f, 0.6f, 0.01f);
		Arrangement* imageArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.4f, 0.01f, ARRANGE_CENTER);
		Arrangement* buttons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.2f, 0.01f);

		Slider* polarSlider = new Slider(ORIENT_VERTICAL, visibleMat, 0.0f, 0.0f, 0.1f, 1.0f);
		polarSlider->setSlideValues(0.0f, 90.0f, polar);
		polarSlider->setFloatCallback(std::bind(&TomographyLoad::setPolar, this, std::placeholders::_1), false);

		std::function<void(UIItem*)> finishFunct = std::bind(&TomographyLoad::finish, this, std::placeholders::_1);

		Button* cancelButton = new Button(cancelMat, cancelCallback);
		Button* updateButton = new Button(updateMat, updateCallback);
		Button* finishButton = new Button(finishMat, finishFunct);

		buttons->addItem(getPtr(cancelButton));
		buttons->addItem(getPtr(updateButton));
		buttons->addItem(getPtr(finishButton));

		imageArrangement->addItem(getPtr(loadedUI));
		imageArrangement->addItem(getPtr(polarSlider));

		column->addItem(getPtr(imageArrangement));
		column->addItem(getPtr(buttons));
		column->updateDisplay();

		canvas.push_back(getPtr(column));

		lightDirection = new Rotator(visibleMat, loadedUI->posx, loadedUI->posy, loadedUI->extentx, loadedUI->extentx * loadedUI->sqAxisRatio);
		lightDirection->updateDisplay();
		lightDirection->setSlideValues(0.0f, 360.0f, 0.0f);
		lightDirection->setFloatCallback(std::bind(&TomographyLoad::setAzimuth, this, std::placeholders::_1), false);
		canvas.push_back(getPtr(lightDirection));

		customUpdate();

		isSetup = true;
	}

	void recreateUI(Material* loadedMat, float angle) {
		if (!isSetup) {
			return;
		}
		outMat = loadedMat;
		canvas[0]->Items[0]->Items[0]->cleanup();
		canvas[0]->Items[0]->Items[0] = getPtr(new ImagePanel(loadedMat, false));
		canvas[0]->Items[0]->Items[0]->update(0.0f, 0.0f, 0.4f, 0.4f);
		canvas[0]->Items[0]->Items[0]->updateDisplay();
		canvas[0]->Items[1]->Items[1]->cleanup();
		canvas[0]->Items[1]->Items.erase(canvas[0]->Items[1]->Items.begin() + 1);
		std::cout << lightDirection->getValue() << " " << angle << std::endl;
		customUpdate();
		update();
		lightDirection->update(canvas[0]->Items[0]->Items[0]->posx, canvas[0]->Items[0]->Items[0]->posy, canvas[0]->Items[0]->Items[0]->extentx, canvas[0]->Items[0]->Items[0]->extentx * canvas[0]->Items[0]->Items[0]->sqAxisRatio);
		lightDirection->setSlideValues(0.0f, 360.0f, lightDirection->getValue() + angle);
		lightDirection->updateDisplay();
	}

private:
	std::function<void(Material*, float, float)> doneCallback = nullptr;
	Rotator* lightDirection = nullptr;
	Material* outMat = nullptr;

	float polar = 50.0f;
	float azimuth = 90.0f;

	void setAzimuth(float az) {
		std::cout << az << std::endl;
		az = 90.0f - az;
		azimuth = (az < 0) ? az + 360.0f : az;
	}

	void setPolar(float pol) {
		polar = pol;
	}

	void finish(UIItem* nothing) {
		if (doneCallback != nullptr) {
			doneCallback(outMat, azimuth, polar);
		}
	}

	void customUpdate() {
		canvas[0]->Items[0]->Items[1]->extenty = canvas[0]->Items[0]->Items[0]->extenty;
		canvas[0]->Items[0]->Items[1]->updateDisplay();
	}
};

class TomographyMenu : public Widget {
public:
	TomographyMenu(LoadList* assets) {
		loadList = assets;
	}

	void setup(surfaceConstructor* sConst, std::function<void(UIItem*)> toggleFunction, MouseManager* mm, std::function<void(UIItem*)> finishTomog) {
		if (isSetup) {
			return;
		}
		surface = sConst;
		mouseManager = mm;

		imageData OpenButton = OPENBUTTON;
		Material* openMat = newMaterial(&OpenButton, "OpenBtn");

		imageData normal = NORMALTEXT;
		Material* normalMat = newMaterial(&normal, "NormalBtn");

		imageData diffuse = DIFFUSETEXT;
		Material* diffuseMat = newMaterial(&diffuse, "DiffuseBtn");

		imageData ub = UNRENDEREDBUTTON;
		Material* invisibleMat = newMaterial(&ub, "UnrenderedBtn");

		imageData tcb = TESTCHECKBOXBUTTON;
		Material* visibleMat = newMaterial(&tcb, "CheckboxBtn");

		imageData plb = PLANEBUTTON;
		Material* planeMat = newMaterial(&plb, "PlaneBtn");

		imageData update = UPDATEBUTTON;
		Material* updateMat = newMaterial(&update, "UpdateBtn");

		imageData finish = FINISHBUTTON;
		Material* finishMat = newMaterial(&finish, "FinishBtn");

		imageData crs = CLOSEBUTTON;
		Material* crossMat = newMaterial(&crs, "CrossBtn");

		imageData rb = RENDEREDBUTTON;
		Material* renderedMat = newMaterial(&rb, "RenderBtn");

		Arrangement* column = new Arrangement(ORIENT_VERTICAL, 1.0f, -1.0f, 0.875f, 0.3f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		Arrangement* buttons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.1f, 0.01f);
		Arrangement* loadButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.1f, 0.01f, ARRANGE_START);

		std::function<void(UIItem*)> tomogLoad = bind(&TomographyMenu::loadFile, this, std::placeholders::_1);
		std::function<void(UIItem*)> performTomog = bind(&TomographyMenu::performTomog, this, std::placeholders::_1);

		std::function<void(UIItem*)> toggleDiffuse = bind(&TomographyMenu::updateDiffuseGen, this, std::placeholders::_1);
		std::function<void(UIItem*)> toggleNormal = bind(&TomographyMenu::updateNormalGen, this, std::placeholders::_1);

		loadButtons->addItem(getPtr(new Button(openMat, tomogLoad)));
		loadButtons->addItem(getPtr(new spacer));
		loadButtons->addItem(getPtr(new Checkbox(planeMat, renderedMat, toggleFunction)));

		column->addItem(getPtr(loadButtons));
		column->addItem(getPtr(new Grid(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.3f, 0.01f)));

		grid = column->Items[1];

		buttons->addItem(getPtr(new Button(diffuseMat)));
		buttons->addItem(getPtr(new Checkbox(visibleMat, invisibleMat, toggleDiffuse)));
		buttons->addItem(getPtr(new spacer));
		buttons->addItem(getPtr(new Button(normalMat)));
		buttons->addItem(getPtr(new Checkbox(visibleMat, invisibleMat, toggleNormal)));
		buttons->addItem(getPtr(new spacer));
		buttons->addItem(getPtr(new Button(updateMat, performTomog)));
		buttons->addItem(getPtr(new Button(finishMat, finishTomog)));
		buttons->updateDisplay();

		column->addItem(getPtr(buttons));
		column->updateDisplay();

		baseDiffuse = loadList->getPtr(sConst->diffTex->copyImage(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1), "TomogDiffTex");
		baseDiffuse->textureImageView = baseDiffuse->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
		baseDiffuse->getCVMat();

		tomographer.setLoadList(loadList);
		tomographer.alignTemplate = baseDiffuse->texMat.clone();
		tomographer.outdims = cv::Size(baseDiffuse->texMat.cols, baseDiffuse->texMat.rows);

		canvas.push_back(getPtr(column));

		scannedMaterial.init(baseDiffuse);

		isSetup = true;
	}

	void cleanupSubClasses() {
		scannedMaterial.cleanupDescriptor();
		tomographer.cleanup();
		if (tomogLoadMenu != nullptr) {
			tomogLoadMenu->cleanup();
		}
	}

	void drawUI(VkCommandBuffer commandBuffer, uint32_t currentFrame) {
		if (tomogLoadMenu != nullptr) {
			tomogLoadMenu->drawUI(commandBuffer, currentFrame);
		}
		for (size_t i = 0; i != canvas.size(); i++) {
			canvas[i]->drawUI(commandBuffer, currentFrame);
		}
	}

	void drawImages(VkCommandBuffer commandBuffer, uint32_t currentFrame) {
		if (tomogLoadMenu != nullptr) {
			tomogLoadMenu->drawImages(commandBuffer, currentFrame);
		}
		for (size_t i = 0; i != canvas.size(); i++) {
			canvas[i]->drawImages(commandBuffer, currentFrame);
		}
	}

	size_t clickIdx = 0;
	size_t loadClickIdx = 0;
	size_t loadPosIdx = 0;

	UIItem* grid = nullptr;

	bool generateDiffuse = true;
	bool generateNormal = true;

	std::string renderPipeline = std::string("BFShading");
	Texture* baseDiffuse = nullptr;

	Material scannedMaterial;
	bool normalAvailable = false;

	int imageCount = 0;
	int activeImageCount = 0;

private:
	Tomographer tomographer;
	TomographyLoad* tomogLoadMenu = nullptr;
	surfaceConstructor* surface = nullptr;

	MouseManager* mouseManager = nullptr;

	std::function<void(Material*)> loadCallback = nullptr;

	void loadFile(UIItem* owner) {
		std::string fileName = winFile::OpenFileDialog();
		if (fileName != std::string("fail")) {
			std::string name = "Image" + std::to_string(imageCount);
			tomographer.add_image(fileName, name + "Tex");
			Material* imageMat = loadList->replacePtr(new Material(tomographer.items[activeImageCount]->baseImage), name + "Mat");
			tomogLoadMenu = new TomographyLoad(loadList);
			std::function<void(Material*, float, float)> loadCallback = std::bind(&TomographyMenu::addItem, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
			std::function<void(UIItem*)> cancelCallback = std::bind(&TomographyMenu::cancelLoad, this, std::placeholders::_1);
			std::function<void(UIItem*)> updateCallback = std::bind(&TomographyMenu::updateCallback, this, std::placeholders::_1);
			tomogLoadMenu->setup(imageMat, loadCallback, cancelCallback, updateCallback);
			for (UIItem* item : canvas) {
				item->setIsEnabled(false);
				item->setVisibility(false);
			}
			loadClickIdx = mouseManager->addClickListener(tomogLoadMenu->getClickCallback());
			loadPosIdx = mouseManager->addPositionListener(tomogLoadMenu->getPosCallback());
		}
	}

	void updateCallback(UIItem* owner) {
		tomographer.align(activeImageCount);
		Material* imageMat = loadList->replacePtr(new Material(tomographer.items[tomographer.items.size() - 1]->correctedImage), tomographer.items[tomographer.items.size() - 1]->name + "Mat");
		tomogLoadMenu->recreateUI(imageMat, tomographer.items[activeImageCount]->rotation);
	}

	void addItem(Material* imageMat, float azimuth, float polar) {
		ImagePanel* loadedUI = new ImagePanel(imageMat, false);
		Material* visibleMat = loadList->findMatPtr("CrossBtnMat");
		Button* deleteButton = new Button(visibleMat, std::bind(&TomographyMenu::removeItem, this, std::placeholders::_1));
		deleteButton->Name = std::to_string(activeImageCount);
		imageCount++;
		activeImageCount++;
		grid->addItem(getPtr(loadedUI));
		grid->updateDisplay();
		UIItem* ref = grid->Items[grid->Items.size() - 1];
		deleteButton->update(ref->posx + ref->extentx * 0.75f, ref->posy - ref->extenty * 0.75f, ref->extentx * 0.2f, ref->extenty * 0.2f);
		deleteButton->updateDisplay();
		canvas.push_back(getPtr(deleteButton));
		tomographer.add_lightVector(azimuth, polar, activeImageCount-1);
		mouseManager->removeClickListener(loadClickIdx);
		mouseManager->removePositionListener(loadPosIdx);
		tomogLoadMenu->cleanup();
		tomogLoadMenu = nullptr;
		for (UIItem* item : canvas) {
			item->setIsEnabled(true);
			item->setVisibility(true);
		}
	}

	void removeItem(UIItem* owner) {
		int index = std::stoi(owner->Name);
		vkQueueWaitIdle(Engine::get()->graphicsQueue);
		tomographer.remove_element(index);
		grid->Items.erase(grid->Items.begin() + index);
		grid->updateDisplay();
		for (size_t i = 1; i != canvas.size(); i++) {
			int currentIndex = std::stoi(canvas[i]->Name);
			if (currentIndex > index) {
				UIItem* ref = grid->Items[currentIndex-1];
				canvas[i]->update(ref->posx + ref->extentx * 0.75f, ref->posy - ref->extenty * 0.75f, ref->extentx * 0.2f, ref->extenty * 0.2f);
				canvas[i]->updateDisplay();
				canvas[i]->Name = std::to_string(currentIndex - 1);
			}
		}
		activeImageCount--;
		owner->cleanup();
		canvas.erase(find(canvas.begin(), canvas.end(), owner));
	}

	void updateDeleteButtons() {
		for (size_t i = 1; i != canvas.size(); i++) {
			int currentIndex = std::stoi(canvas[i]->Name);
			UIItem* ref = grid->Items[currentIndex];
			canvas[i]->update(ref->posx + ref->extentx * 0.75f, ref->posy - ref->extenty * 0.75f, ref->extentx * 0.2f, ref->extenty * 0.2f);
			canvas[i]->updateDisplay();
		}
	}

	void customUpdate() {
		if (tomogLoadMenu != nullptr) {
			tomogLoadMenu->update();
		}
		updateDeleteButtons();
	}

	void cancelLoad(UIItem* owner) {
		mouseManager->removeClickListener(loadClickIdx);
		mouseManager->removePositionListener(loadPosIdx);
		vkQueueWaitIdle(Engine::get()->graphicsQueue);
		tomogLoadMenu->cleanup();
		tomogLoadMenu = nullptr;
		for (UIItem* item : canvas) {
			item->setIsEnabled(true);
			item->setVisibility(true);
		}
		//tomographer.remove_imageOnly(tomographer.images.size()-1);
	}

	void updateDiffuseGen(UIItem* owner) {
		generateDiffuse = owner->activestate;
	}

	void updateNormalGen(UIItem* owner) {
		generateNormal = owner->activestate;
	}

	void performTomog(UIItem* owner) {
		if (generateNormal && !generateDiffuse) {
			tomographer.calculate_normal();
			normalAvailable = true;
			scannedMaterial.init(baseDiffuse, loadList->replacePtr(new imageTexture(tomographer.computedNormal, VK_FORMAT_R8G8B8A8_UNORM), "TomogNormTex"));
			renderPipeline = "TSNormBF";
		}
		else if (generateNormal && generateDiffuse) {
			tomographer.calculate_NormAndDiff();
			normalAvailable = true;
			scannedMaterial.init(loadList->replacePtr(new imageTexture(tomographer.computedDiffuse), "TomogDiffTex"), loadList->replacePtr(new imageTexture(tomographer.computedNormal, VK_FORMAT_R8G8B8A8_UNORM), "TomogNormTex"));
			renderPipeline = "TSNormBF";
		}
		else {
			std::cout << "Invalid configuration" << std::endl;
		}
	}
};

#endif