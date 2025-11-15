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
	void add_lightVector(float phi, float theta);

	void calculate_normal();
	void calculate_diffuse();
	void calculate_NormAndDiff();

	void setLoadList(LoadList* ll) {
		loadList = ll;
	}

	void clearData() {
		images.clear();
		vectors.clear();
		computedNormal.release();
		computedDiffuse.release();
		alignTemplate.release();
	}

	void cleanup() {
		clearData();
	}

	LoadList* loadList = nullptr;

	std::vector<Texture*> images;
private:
	bool normalExists = false;
	std::vector<Texture*> originalImages;
	
	std::vector<std::vector<float>> vectors;
};

class TomographyLoad : public Widget {
public:
	TomographyLoad(LoadList* assets) {
		loadList = assets;
	}

	void setup(Material* loadedMat, std::function<void(Material*, float, float)> callback, std::function<void(UIItem*)> cancelCallback) {
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

		Arrangement* column = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.4f, 0.6f, 0.01f);
		Arrangement* buttons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.2f, 0.01f);

		std::function<void(UIItem*)> finishFunct = std::bind(&TomographyLoad::finish, this, std::placeholders::_1);

		Button* cancelButton = new Button(cancelMat, cancelCallback);
		Button* finishButton = new Button(finishMat, finishFunct);

		buttons->addItem(getPtr(cancelButton));
		buttons->addItem(getPtr(finishButton));

		column->addItem(getPtr(loadedUI));
		column->addItem(getPtr(buttons));
		column->updateDisplay();

		canvas.push_back(getPtr(column));

		lightDirection = new Rotator(visibleMat, loadedUI->posx, loadedUI->posy, loadedUI->extentx, loadedUI->extentx * loadedUI->sqAxisRatio);
		lightDirection->updateDisplay();
		lightDirection->setSlideValues(0.0f, 360.0f, 0.0f);
		lightDirection->setFloatCallback(std::bind(&TomographyLoad::setAzimuth, this, std::placeholders::_1), false);
		canvas.push_back(getPtr(lightDirection));


		isSetup = true;
	}
private:
	std::function<void(Material*, float, float)> doneCallback = nullptr;
	Rotator* lightDirection = nullptr;
	Material* outMat = nullptr;

	float polar = 50.0f;
	float azimuth = 90.0f;

	void setAzimuth(float az) {
		az = 90.0f - az;
		azimuth = (az < 0) ? az + 360.0f : az;
		std::cout << azimuth << std::endl;
	}

	void finish(UIItem* nothing) {
		if (doneCallback != nullptr) {
			doneCallback(outMat, azimuth, polar);
		}
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

		imageData update = UPDATEBUTTON;
		Material* updateMat = newMaterial(&update, "UpdateBtn");

		imageData finish = FINISHBUTTON;
		Material* finishMat = newMaterial(&finish, "FinishBtn");

		Arrangement* column = new Arrangement(ORIENT_VERTICAL, 1.0f, -1.0f, 0.875f, 0.3f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		Arrangement* buttons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.1f, 0.01f);
		Arrangement* loadButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.1f, 0.01f, ARRANGE_START);

		std::function<void(UIItem*)> tomogLoad = bind(&TomographyMenu::loadFile, this, std::placeholders::_1);
		std::function<void(UIItem*)> performTomog = bind(&TomographyMenu::performTomog, this, std::placeholders::_1);

		std::function<void(UIItem*)> toggleDiffuse = bind(&TomographyMenu::updateDiffuseGen, this, std::placeholders::_1);
		std::function<void(UIItem*)> toggleNormal = bind(&TomographyMenu::updateNormalGen, this, std::placeholders::_1);

		loadButtons->addItem(getPtr(new Button(openMat, tomogLoad)));
		loadButtons->addItem(getPtr(new spacer));
		loadButtons->addItem(getPtr(new Checkbox(visibleMat, invisibleMat, toggleFunction)));

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

private:
	Tomographer tomographer;
	TomographyLoad* tomogLoadMenu = nullptr;
	surfaceConstructor* surface = nullptr;

	MouseManager* mouseManager = nullptr;

	std::function<void(Material*)> loadCallback = nullptr;

	void loadFile(UIItem* owner) {
		std::string fileName = winFile::OpenFileDialog();
		if (fileName != std::string("fail")) {
			std::string name = "Image" + std::to_string(grid->Items.size());
			tomographer.add_image(fileName, name + "Tex");
			Material* imageMat = loadList->getPtr(new Material(tomographer.images[tomographer.images.size() - 1]), name + "Mat");
			tomogLoadMenu = new TomographyLoad(loadList);
			std::function<void(Material*, float, float)> loadCallback = std::bind(&TomographyMenu::addItem, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
			std::function<void(UIItem*)> cancelCallback = std::bind(&TomographyMenu::cancelLoad, this, std::placeholders::_1);
			tomogLoadMenu->setup(imageMat, loadCallback, cancelCallback);
			for (UIItem* item : canvas) {
				item->setIsEnabled(false);
				item->setVisibility(false);
			}
			loadClickIdx = mouseManager->addClickListener(tomogLoadMenu->getClickCallback());
			loadPosIdx = mouseManager->addPositionListener(tomogLoadMenu->getPosCallback());
		}
	}

	void addItem(Material* imageMat, float azimuth, float polar) {
		ImagePanel* loadedUI = new ImagePanel(imageMat, false);
		grid->addItem(getPtr(loadedUI));
		grid->updateDisplay();
		tomographer.add_lightVector(azimuth, polar);
		mouseManager->removeClickListener(loadClickIdx);
		mouseManager->removePositionListener(loadPosIdx);
		tomogLoadMenu->cleanup();
		tomogLoadMenu = nullptr;
		for (UIItem* item : canvas) {
			item->setIsEnabled(true);
			item->setVisibility(true);
		}
	}

	void cancelLoad(UIItem* owner) {
		mouseManager->removeClickListener(loadClickIdx);
		mouseManager->removePositionListener(loadPosIdx);
		tomogLoadMenu->cleanup();
		tomogLoadMenu = nullptr;
		for (UIItem* item : canvas) {
			item->setIsEnabled(true);
			item->setVisibility(true);
		}
	}

	void updateDiffuseGen(UIItem* owner) {
		generateDiffuse = owner->activestate;
	}

	void updateNormalGen(UIItem* owner) {
		generateNormal = owner->activestate;
	}

	void performTomog(UIItem* owner) {
		//tomographer.outdims = cv::Size(baseDiffuse->texMat.cols, baseDiffuse->texMat.rows);
		//tomographer.alignTemplate = &baseDiffuse->texMat;
		if (generateNormal && !generateDiffuse) {
			tomographer.calculate_normal();
			normalAvailable = true;
			scannedMaterial.init(baseDiffuse, loadList->replacePtr(new imageTexture(tomographer.computedNormal), "TomogNormTex"));
			renderPipeline = "TSNormBF";
		}
		else if (generateNormal && generateDiffuse) {
			tomographer.calculate_NormAndDiff();
			normalAvailable = true;
			scannedMaterial.init(loadList->replacePtr(new imageTexture(tomographer.computedDiffuse), "TomogDiffTex"), loadList->replacePtr(new imageTexture(tomographer.computedNormal), "TomogNormTex"));
			renderPipeline = "TSNormBF";
		}
		else {
			std::cout << "Invalid configuration" << std::endl;
		}
	}
};

#endif