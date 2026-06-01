#ifndef REMAPPER
#define REMAPPER

#include"UIelements.h"
#include"Bobject_Engine.h"
#include"Textures.h"
//#include"SurfaceConstructor.h"

#include"ImageProcessor.h"
#include"include/Kuwahara.h"
#include"include/RGB_2_YCbCr.h"
#include"include/SobelCombined.h"
#include"include/ReferenceKuwahara.h"
#include"include/Averager.h"
#include"include/IterativeAverager.h"
#include"include/CoordIterativeAverager.h"
#include"include/GradRemap.h"
#include"include/CreateCoordMap.h"
#include"include/CoordMapRead.h"
#include"GaussBlurX.h"
#include"GaussBlurY.h"

#define KUWAHARA 0
#define ITERATIVE 1
#define ITERATIVE_COORDMAP 2
#define METHODCOUNT 3

struct RemapParamObject {
	// Kuwahara params
	alignas(4) int kuwaharaKernelRadius;
	// We could potentially add zero cross/sharpness params but these values are also currently unused

	// Averager and gradient remapper params
	alignas(4) int averagerKernelRadius;
	alignas(4) float gradientThreshold;

	alignas(4) float zeroCross;
	alignas(4) float hardness;
	alignas(4) float sharpness;

	alignas(1) bool shouldNormalize;
};

class RemapBackend {
public:
	RemapBackend() = default;

	void setup() {
		params.kuwaharaKernelRadius = 15;
		switch (method) {
		case (KUWAHARA):
			params.averagerKernelRadius = 15;
			params.gradientThreshold = 0.06f;
			break;
		default:
			params.averagerKernelRadius = 5;
			params.gradientThreshold = 0.05f;
			minAveragerKernel = 1;
			maxAveragerKernel = 25;
			break;
		}
		params.zeroCross = 0.58f;
		params.hardness = 8.0f; // modifying this appears to have minimal effect when we are already flattening
		params.sharpness = 8.0f;

		params.shouldNormalize = true;

		createParamBuffer();
		updateParamBuffer();
	}

	void setKuwaharaKernel(int kern) {
		params.kuwaharaKernelRadius = kern;
		updateParamBuffer();
	}

	void setZeroCross(float zeroCross) {
		params.zeroCross = zeroCross;
		updateParamBuffer();
	}

	void setKuwaharaSharpness(float sharpness) {
		params.sharpness = sharpness;
		updateParamBuffer();
	}

	void setKuwaharaHardness(float hardness) {
		params.hardness = hardness;
		updateParamBuffer();
	}

	void setAveragerKernel(int kern) {
		params.averagerKernelRadius = kern;
		updateParamBuffer();
	}

	void setGradientThreshold(float thresh) {
		params.gradientThreshold = thresh;
		updateParamBuffer();
	}

	void setIterationCount(int count) {
		numIterations = count;
	}

	void toggleNormalization() {
		params.shouldNormalize = !params.shouldNormalize;
		updateParamBuffer();
	}

	int minKuwaharaKernel = 2;
	int maxKuwaharaKernel = 32;

	int minAveragerKernel = 2;
	int maxAveragerKernel = 128;

	int minIts = 1;
	int maxIts = 100;

	float minGradientThreshold = 0.01f;
	float maxGradientThreshold = 0.25f;

	void updateParamBuffer();
	
	void createParamBuffer();

	void createReferenceMaps(Texture*, Texture*);

	void performRemap(VkCommandBuffer);

	void cleanup();

	Texture* filteredTarget = nullptr;
	Texture* baseRef = nullptr;
	Texture* baseTarget = nullptr;

	uint32_t method = ITERATIVE;

private:
	RemapParamObject params{};

	VkBuffer paramBuffer = nullptr;
	VkDeviceMemory paramBufferMemory = nullptr;
	void* paramBufferMapped = nullptr;

	uint32_t baseHeight = 0, baseWidth = 0;

	int numIterations = 5;

	filter* Kuwahara = nullptr;
	filter* colourConverter = nullptr;
	filter* SobelCombined = nullptr;
	filter* Averager = nullptr;
	filter* gradRemap = nullptr;
	filter* referenceKuwahara = nullptr;
	filter* coordMapCreator = nullptr;
	filter* coordAverager = nullptr;
	filter* coordReader = nullptr;
};

class RemapTexSelector : public Widget {
public:
	RemapTexSelector(LoadList* assets, LoadList* texLL) {
		loadList = assets;
		textureLL = texLL;
	}

	void setup(std::string targetTextureName, std::function<void(std::string, std::string)> callback) {
		if (isSetup) {
			return;
		}
		targetName = targetTextureName;

		continueCallback = callback;

		std::vector<std::string> availableTextures{};
		textureLL->listTextures(availableTextures);

		if (std::find(availableTextures.begin(), availableTextures.end(), targetName) == availableTextures.end()) {
			std::cout << "The target texture provided is not a valid texture in the set" << std::endl;
			return;
		}

		Material* renderedMat = loadList->getMaterial("RenderBtnMat");
		Material* visibleMat = loadList->getMaterial("TestCheckBtnMat");
		Material* finishedMat = loadList->getMaterial("FinishBtnMat");

		std::function<void(UIItem*)> finishFunct = std::bind(&RemapTexSelector::finish, this, std::placeholders::_1);
		
		Arrangement* mainArrangement = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.25f, 0.8f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		font* newFont = textureLL->getFont();

		TextBox* targetText = new TextBox(newFont, 0.0f, 0.0f, 5.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
		targetText->addText(std::string("Remapping ") + targetTextureName);

		TextBox* setRefText = new TextBox(newFont, 0.0f, 0.0f, 5.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
		setRefText->addText("Set the reference map:");

		DropdownMenu* refDropdown = new DropdownMenu(0.0f, 0.0f, 5.0f, 1.0f, renderedMat, visibleMat, loadList->getMaterial("UIRoundBox"), newFont);
		refDropdown->addOptions(availableTextures);
		refDropdown->setOptionIndex(0);
		refDropdown->setSelectCallback(std::bind(&RemapTexSelector::setRefName, this, std::placeholders::_1));

		Arrangement* finishButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 5.0f, 2.0f, 0.01f);
		finishButtons->addItem(getPtr(new spacer()));
		finishButtons->addItem(getPtr(new Button(finishedMat, finishFunct)));

		mainArrangement->addItem(getPtr(targetText));
		mainArrangement->addItem(getPtr(setRefText));
		mainArrangement->addItem(getPtr(refDropdown));
		mainArrangement->addItem(getPtr(finishButtons));
		mainArrangement->arrangeItems();

		canvas.push_back(getPtr(mainArrangement));
		isSetup = true;
	}

	int clickIndex = 0;

private:
	LoadList* textureLL = nullptr;

	std::string targetName = "";
	std::string refName = "";

	std::function<void(std::string, std::string)> continueCallback = nullptr;

	void setRefName(UIItem* owner) {
		refName = owner->text;
	}

	void finish(UIItem* owner) {
		if (targetName.size() == 0 || refName.size() == 0) {
			std::cout << "Insufficient textures to continue" << std::endl;
			return;
		}

		if (continueCallback != nullptr) {
			continueCallback(refName, targetName);
		}
	}
};

class RemapUI : public Widget {
public:
	RemapUI(LoadList* assets, LoadList* textureLoadList) {
		loadList = assets;
		textureLL = textureLoadList;
	}

	void setup(std::string rTexName, std::string tTexName, std::function<void(UIItem*)> cancelFunct, std::function<void(UIItem*)> finishFunct) {
		refTexName = rTexName;
		targetTexName = tTexName;
		
		refTex = textureLL->getTexture(rTexName);
		if (refTex->textureImageView == nullptr) {
			refTex->textureImageView = refTex->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
		}
		targetTex = textureLL->getTexture(tTexName);
		if (targetTex->textureImageView == nullptr) {
			targetTex->textureImageView = targetTex->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
		}
		if (refTex == nullptr || targetTex == nullptr) {
			return;
		}
		
		remapper = new RemapBackend();
		remapper->setup();
		if (targetTex->textureFormat != VK_FORMAT_R8G8B8A8_UNORM) {
			remapper->toggleNormalization();
		}
		fullRemap(refTex, targetTex);

		Material* visibleMat = loadList->getMaterial("TestCheckBtnMat");
		Material* invisibleMat = loadList->getMaterial("UnrenderedBtnMat");
		Material* forwardMat = loadList->getMaterial("PlayBtnMat");
		Material* backMat = loadList->getMaterial("BackBtnMat");

		std::function<void(UIItem*)> toggleNormFunction = std::bind(&RemapUI::toggleNormalization, this, std::placeholders::_1);
		std::function<void(UIItem*)> incrementMethodFunction = std::bind(&RemapUI::incrementMethod, this, std::placeholders::_1);
		std::function<void(UIItem*)> reduceMethodFunction = std::bind(&RemapUI::reduceMethod, this, std::placeholders::_1);

		outMap = getPtr(new ImagePanel(loadList->replacePtr(new Material(remapper->filteredTarget), "RemapOSMat"), false));

		Arrangement* column = new Arrangement(ORIENT_VERTICAL, 1.0f, 0.0f, 0.25f, 0.8f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		Arrangement* methodButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.25f, 0.01f);

		Button* backButton = new Button(backMat, reduceMethodFunction);
		Button* forwardButton = new Button(forwardMat, incrementMethodFunction);

		Checkbox* normalizeCheckbox = new Checkbox(visibleMat, invisibleMat, toggleNormFunction);

		methodButtons->addItem(getPtr(backButton));
		methodButtons->addItem(getPtr(normalizeCheckbox));
		methodButtons->addItem(getPtr(forwardButton));
		
		column->addItem(outMap);
		column->addItem(methodButtons);

		Arrangement* endButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.2f, 0.01f, ARRANGE_END);

		Material* cancelMat = loadList->getMaterial("CancelBtnMat");
		Material* finishMat = loadList->getMaterial("FinishBtnMat");

		Button* finishButton = new Button(finishMat, finishFunct);
		Button* cancelButton = new Button(cancelMat, cancelFunct);

		endButtons->addItem(getPtr(cancelButton));
		endButtons->addItem(getPtr(finishButton));

		canvas.push_back(getPtr(column));

		createUI();

		canvas[0]->addItem(getPtr(endButtons));
		canvas[0]->updateDisplay();
		
		textureLL->replacePtr(remapper->filteredTarget->copyTexture(), targetTexName);

		isSetup = true;
	}

	void cleanupSubClasses() {
		if (remapper != nullptr) {
			remapper->cleanup();
			delete remapper;
			remapper = nullptr;
		}
	}

	int priorityLayer = 100;

	size_t clickIndex = 0;
	size_t posIndex = 0;

	RemapBackend* remapper = nullptr;
	UIItem* outMap = nullptr;

	std::string targetTexName = "";
private:

	LoadList* textureLL = nullptr;

	Texture* refTex = nullptr;
	Texture* targetTex = nullptr;

	std::string refTexName = "";

	void fullRemap(Texture*, Texture*);

	void kuwaharaCallback(int);
	void zeroCrossCallback(float);
	void sharpnessCallback(float);
	void hardnessCallback(float);
	void averagerCallback(int);
	void gradientCallback(float);
	void iterationCallback(int);

	void toggleNormalization(UIItem*);
	void incrementMethod(UIItem*);
	void reduceMethod(UIItem*);

	void createUI() {
		Material* visibleMat = loadList->getMaterial("TestCheckBtnMat");

		UIItem* column = canvas[0];

		if (remapper->method == KUWAHARA) {
			std::function<void(int)> kuwaharaSliderFunction = std::bind(&RemapUI::kuwaharaCallback, this, std::placeholders::_1);
			std::function<void(float)> zeroCrossSliderFunction = std::bind(&RemapUI::zeroCrossCallback, this, std::placeholders::_1);
			std::function<void(float)> sharpnessSliderFunction = std::bind(&RemapUI::sharpnessCallback, this, std::placeholders::_1);
			std::function<void(int)> averagerSliderFunction = std::bind(&RemapUI::averagerCallback, this, std::placeholders::_1);
			std::function<void(float)> gradientSliderFunction = std::bind(&RemapUI::gradientCallback, this, std::placeholders::_1);
			std::function<void(int)> iterationSliderFunction = std::bind(&RemapUI::iterationCallback, this, std::placeholders::_1);

			Slider* kuwaharaKernSlider = new Slider(visibleMat, 0.0f, 0.0f, 1.0f, 0.25f);
			kuwaharaKernSlider->updateDisplay();
			kuwaharaKernSlider->setSlideValues(remapper->minKuwaharaKernel, remapper->maxKuwaharaKernel, 15);
			kuwaharaKernSlider->setIntCallback(kuwaharaSliderFunction, false);

			Slider* zeroCrossSlider = new Slider(visibleMat, 0.0f, 0.0f, 1.0f, 0.25f);
			zeroCrossSlider->updateDisplay();
			zeroCrossSlider->setSlideValues(0.5f, 2.0f, 0.58f);
			zeroCrossSlider->setFloatCallback(zeroCrossSliderFunction, false);

			Slider* sharpnessSlider = new Slider(visibleMat, 0.0f, 0.0f, 1.0f, 0.25f);
			sharpnessSlider->updateDisplay();
			sharpnessSlider->setSlideValues(1.0f, 20.0f, 8.0f);
			sharpnessSlider->setFloatCallback(sharpnessSliderFunction, false);

			Slider* averagerKernSlider = new Slider(visibleMat, 0.0f, 0.0f, 1.0f, 0.25f);
			averagerKernSlider->updateDisplay();
			averagerKernSlider->setSlideValues(remapper->minAveragerKernel, remapper->maxAveragerKernel, 15);
			averagerKernSlider->setIntCallback(averagerSliderFunction, false);

			Slider* gradientThreshSlider = new Slider(visibleMat, 0.0f, 0.0f, 1.0f, 0.25f);
			gradientThreshSlider->updateDisplay();
			gradientThreshSlider->setSlideValues(remapper->minGradientThreshold, remapper->maxGradientThreshold, 0.06f);
			gradientThreshSlider->setFloatCallback(gradientSliderFunction, false);

			Material* cancelMat = loadList->getMaterial("CancelBtnMat");
			Material* finishMat = loadList->getMaterial("FinishBtnMat");

			TextBox* noiseText = new TextBox(loadList->getFont(), 0.0f, 0.0f, 1.0f, 0.25f, 18, ARRANGE_START, ARRANGE_CENTER);
			noiseText->addText("Noise removal:");

			TextBox* kuwaharaText = new TextBox(loadList->getFont(), 0.0f, 0.0f, 1.0f, 0.25f, 18, ARRANGE_START, ARRANGE_CENTER);
			kuwaharaText->addText("Search size:");

			TextBox* sharpnessText = new TextBox(loadList->getFont(), 0.0f, 0.0f, 1.0f, 0.25f, 18, ARRANGE_START, ARRANGE_CENTER);
			sharpnessText->addText("Edge sharpness:");

			TextBox* flatnessText = new TextBox(loadList->getFont(), 0.0f, 0.0f, 1.0f, 0.25f, 18, ARRANGE_START, ARRANGE_CENTER);
			flatnessText->addText("Stroke flatness:");

			TextBox* gradientText = new TextBox(loadList->getFont(), 0.0f, 0.0f, 1.0f, 0.25f, 18, ARRANGE_START, ARRANGE_CENTER);
			gradientText->addText("Flatten threshold:");

			column->addItem(getPtr(kuwaharaText));
			column->addItem(getPtr(kuwaharaKernSlider));
			column->addItem(getPtr(noiseText));
			column->addItem(getPtr(zeroCrossSlider));
			column->addItem(getPtr(sharpnessText));
			column->addItem(getPtr(sharpnessSlider));
			column->addItem(getPtr(flatnessText));
			column->addItem(getPtr(averagerKernSlider));
			column->addItem(getPtr(gradientText));
			column->addItem(getPtr(gradientThreshSlider));
		}
		else {
			std::function<void(int)> kuwaharaSliderFunction = std::bind(&RemapUI::kuwaharaCallback, this, std::placeholders::_1);
			std::function<void(float)> zeroCrossSliderFunction = std::bind(&RemapUI::zeroCrossCallback, this, std::placeholders::_1);
			std::function<void(float)> sharpnessSliderFunction = std::bind(&RemapUI::sharpnessCallback, this, std::placeholders::_1);
			std::function<void(int)> averagerSliderFunction = std::bind(&RemapUI::averagerCallback, this, std::placeholders::_1);
			std::function<void(float)> gradientSliderFunction = std::bind(&RemapUI::gradientCallback, this, std::placeholders::_1);
			std::function<void(int)> iterationSliderFunction = std::bind(&RemapUI::iterationCallback, this, std::placeholders::_1);

			TextBox* searchText = new TextBox(loadList->getFont(), 0.0f, 0.0f, 1.0f, 0.25f, 18, ARRANGE_START, ARRANGE_CENTER);
			searchText->addText("Search size:");

			TextBox* flatnessText = new TextBox(loadList->getFont(), 0.0f, 0.0f, 1.0f, 0.25f, 18, ARRANGE_START, ARRANGE_CENTER);
			flatnessText->addText("Stroke flatness:");

			TextBox* sharpnessText = new TextBox(loadList->getFont(), 0.0f, 0.0f, 1.0f, 0.25f, 18, ARRANGE_START, ARRANGE_CENTER);
			sharpnessText->addText("Edge sharpness:");

			Slider* kuwaharaKernSlider = new Slider(visibleMat, 0.0f, 0.0f, 1.0f, 0.25f);
			kuwaharaKernSlider->updateDisplay();
			kuwaharaKernSlider->setSlideValues(remapper->minIts, remapper->maxIts, 5);
			kuwaharaKernSlider->setIntCallback(iterationSliderFunction, false);

			Slider* averagerKernSlider = new Slider(visibleMat, 0.0f, 0.0f, 1.0f, 0.25f);
			averagerKernSlider->updateDisplay();
			averagerKernSlider->setSlideValues(remapper->minAveragerKernel, remapper->maxAveragerKernel, 5);
			averagerKernSlider->setIntCallback(averagerSliderFunction, false);

			Slider* gradientThreshSlider = new Slider(visibleMat, 0.0f, 0.0f, 1.0f, 0.25f);
			gradientThreshSlider->updateDisplay();
			gradientThreshSlider->setSlideValues(remapper->minGradientThreshold, remapper->maxGradientThreshold, 0.05f);
			gradientThreshSlider->setFloatCallback(gradientSliderFunction, false);

			column->addItem(getPtr(searchText));
			column->addItem(getPtr(kuwaharaKernSlider));
			column->addItem(getPtr(flatnessText));
			column->addItem(getPtr(averagerKernSlider));
			column->addItem(getPtr(sharpnessText));
			column->addItem(getPtr(gradientThreshSlider));
		}

		column->updateDisplay();
	}
};

#endif
