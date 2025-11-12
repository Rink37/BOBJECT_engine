#ifndef REMAPPER
#define REMAPPER

#include"UIelements.h"
#include"Bobject_Engine.h"
#include"Textures.h"
#include"SurfaceConstructor.h"

#include"ImageProcessor.h"
#include"include/Kuwahara.h"
#include"include/SobelCombined.h"
#include"include/ReferenceKuwahara.h"
#include"include/Averager.h"
#include"include/GradRemap.h"
#include"GaussBlurX.h"
#include"GaussBlurY.h"
#include"include/BakedImages.h"

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
};

class RemapBackend {
public:
	RemapBackend() = default;

	void setup() {
		params.kuwaharaKernelRadius = 15;
		params.averagerKernelRadius = 15;
		params.gradientThreshold = 0.06f;
		params.zeroCross = 0.58f;
		params.hardness = 8.0f; // modifying this appears to have minimal effect when we are already flattening
		params.sharpness = 8.0f;

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

	int minKuwaharaKernel = 2;
	int maxKuwaharaKernel = 32;

	int minAveragerKernel = 2;
	int maxAveragerKernel = 128;

	float minGradientThreshold = 0.02f;
	float maxGradientThreshold = 0.2f;

	void updateParamBuffer();
	
	void createParamBuffer();

	void createReferenceMaps(Texture*, Texture*);

	void createBaseMaps();
	void performRemap();
	void smootheResult();

	void cleanup();

	Texture* filteredOSNormal = nullptr;
	Texture* baseDiffuse = nullptr;
	Texture* baseOSNormal = nullptr;

	bool smoothePass = true;
private:
	RemapParamObject params{};

	VkBuffer paramBuffer = nullptr;
	VkDeviceMemory paramBufferMemory = nullptr;
	void* paramBufferMapped = nullptr;

	Texture* gradients = nullptr;

	uint32_t baseHeight = 0, baseWidth = 0;
};

class RemapUI : public Widget {
public:
	RemapUI(LoadList* assets, surfaceConstructor* sConst) {
		loadList = assets;
		this->sConst = sConst;
	}

	void setup(std::function<void(UIItem*)> cancelFunct, std::function<void(UIItem*)> finishFunct) {
		if (sConst->diffTex == nullptr || sConst->OSNormTex == nullptr) {
			return;
		}
		
		remapper = new RemapBackend();
		remapper->setup();
		fullRemap(sConst->diffTex, sConst->OSNormTex);
		
		imageData tcb = TESTCHECKBOXBUTTON;
		Material* visibleMat = newMaterial(&tcb, "TestCheckBtn");

		std::function<void(int)> kuwaharaSliderFunction = std::bind(&RemapUI::kuwaharaCallback, this, std::placeholders::_1);
		std::function<void(float)> zeroCrossSliderFunction = std::bind(&RemapUI::zeroCrossCallback, this, std::placeholders::_1);
		std::function<void(float)> sharpnessSliderFunction = std::bind(&RemapUI::sharpnessCallback, this, std::placeholders::_1);
		std::function<void(int)> averagerSliderFunction = std::bind(&RemapUI::averagerCallback, this, std::placeholders::_1);
		std::function<void(float)> gradientSliderFunction = std::bind(&RemapUI::gradientCallback, this, std::placeholders::_1);

		outMap = getPtr(new ImagePanel(loadList->replacePtr(new Material(remapper->filteredOSNormal), "RemapOSMat"), false));

		Arrangement* column = new Arrangement(ORIENT_VERTICAL, 1.0f, 0.0f, 0.25f, 0.8f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		imageData searchSize = SEARCHSIZETEXT;
		Material* searchSizeMat = newMaterial(&searchSize, "SearchSizeText");

		imageData flatness = STROKEFLATNESSTEXT;
		Material* flatnessMat = newMaterial(&flatness, "FlatnessText");

		imageData sharpness = EDGESHARPNESSTEXT;
		Material* sharpnessMat = newMaterial(&sharpness, "SharpnessText");

		imageData noiseRemoval = NOISEREMOVALTEXT;
		Material* noiseMat = newMaterial(&noiseRemoval, "NoiseRemovalText");

		imageData flattenThresh = FLATTENTHRESHOLDTEXT;
		Material* threshMat = newMaterial(&flattenThresh, "FlattenThresholdText");

		Button* searchSizeBtn = new Button(searchSizeMat);
		Button* flatnessBtn = new Button(flatnessMat);
		Button* sharpnessBtn = new Button(sharpnessMat);
		Button* noiseBtn = new Button(noiseMat);
		Button* threshBtn = new Button(threshMat);

		Arrangement* kuwaharaArranger = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.25f, 0.01f);
		Arrangement* zeroCrossArranger = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.25f, 0.01f);
		Arrangement* sharpnessArranger = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.25f, 0.01f);
		Arrangement* averagerArranger = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.25f, 0.01f);
		Arrangement* gradientArranger = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.25f, 0.01f);

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

		Arrangement* endButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.2f, 0.01f, ARRANGE_END);

		imageData finish = FINISHBUTTON;
		Material* finishMat = newMaterial(&finish, "FinishBtn");

		imageData cancel = CANCELBUTTON;
		Material* cancelMat = newMaterial(&cancel, "CancelBtn");

		Button* finishButton = new Button(finishMat, finishFunct);
		Button* cancelButton = new Button(cancelMat, cancelFunct);

		endButtons->addItem(getPtr(cancelButton));
		endButtons->addItem(getPtr(finishButton));

		zeroCrossArranger->addItem(getPtr(noiseBtn));
		zeroCrossArranger->addItem(getPtr(zeroCrossSlider));

		kuwaharaArranger->addItem(getPtr(searchSizeBtn));
		kuwaharaArranger->addItem(getPtr(kuwaharaKernSlider));

		sharpnessArranger->addItem(getPtr(sharpnessBtn));
		sharpnessArranger->addItem(getPtr(sharpnessSlider));

		averagerArranger->addItem(getPtr(flatnessBtn));
		averagerArranger->addItem(getPtr(averagerKernSlider));

		gradientArranger->addItem(getPtr(threshBtn));
		gradientArranger->addItem(getPtr(gradientThreshSlider));
		
		column->addItem(outMap);
		column->addItem(getPtr(kuwaharaArranger));
		column->addItem(getPtr(zeroCrossArranger));
		column->addItem(getPtr(sharpnessArranger));
		column->addItem(getPtr(averagerArranger));
		column->addItem(getPtr(gradientArranger));
		column->addItem(getPtr(endButtons));

		column->updateDisplay();

		canvas.push_back(getPtr(column));

		sConst->normalType = 0;
		sConst->loadNormal(remapper->filteredOSNormal->copyTexture());

		isSetup = true;
	}

	void cleanupSubClasses() {
		remapper->cleanup();
		delete remapper;
		remapper = nullptr;
	}

	int priorityLayer = 100;

	size_t clickIndex = 0;
	size_t posIndex = 0;

	RemapBackend* remapper = nullptr;
	UIItem* outMap = nullptr;
private:

	surfaceConstructor* sConst = nullptr;

	void fullRemap(Texture*, Texture*);

	void kuwaharaCallback(int);
	void zeroCrossCallback(float);
	void sharpnessCallback(float);
	void hardnessCallback(float);
	void averagerCallback(int);
	void gradientCallback(float);
};

#endif
