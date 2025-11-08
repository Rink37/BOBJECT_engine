#ifndef REMAPPER
#define REMAPPER

#include"UIelements.h"
#include"Bobject_Engine.h"
#include"Textures.h"

#include"ImageProcessor.h"
#include"include/Kuwahara.h"
#include"include/SobelX.h"
#include"include/SobelY.h"
#include"include/ReferenceKuwahara.h"
#include"include/Averager.h"
#include"include/GradRemap.h"

#include"include/BakedImages.h"

struct RemapParamObject {
	// Kuwahara params
	alignas(16) int kuwaharaKernelRadius;
	// We could potentially add zero cross/sharpness params but these values are also currently unused

	// Averager and gradient remapper params
	alignas(16) int averagerKernelRadius;
	alignas(16) float gradientThreshold;
};

class RemapBackend {
public:
	RemapBackend() {
		params.kuwaharaKernelRadius = 15;
		params.averagerKernelRadius = 15;
		params.gradientThreshold = 0.06f;

		createParamBuffer();
		updateParamBuffer();
	}

	void setKuwaharaKernel(int kern) {
		params.kuwaharaKernelRadius = kern;
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

	int minKuwaharaKernel = 3;
	int maxKuwaharaKernel = 16;

	int minAveragerKernel = 3;
	int maxAveragerKernel = 16;

	float minGradientThreshold = 0.0f;
	float maxGradientThreshold = 0.25f;

	void updateParamBuffer();
	
	void createParamBuffer();

	void createReferenceMaps(Texture*, Texture*);

	void createBaseMaps();
	void performRemap();
	void smootheResult();

	void cleanup();

	Texture* filteredOSNormal = nullptr;
private:
	RemapParamObject params{};

	VkBuffer paramBuffer = nullptr;
	VkDeviceMemory paramBufferMemory = nullptr;
	void* paramBufferMapped = nullptr;

	Texture* baseDiffuse = nullptr;
	Texture* baseOSNormal = nullptr;

	Texture* xGradients = nullptr;
	Texture* yGradients = nullptr;

	bool isSmootheNeeded = true;
};

class RemapUI : public Widget {
public:
	RemapUI(LoadList* assets) {
		loadList = assets;
	}

	void setup(Texture* diffTex, Texture* OSNormTex) {
		fullRemap(diffTex, OSNormTex);
		
		imageData tcb = TESTCHECKBOXBUTTON;
		Material* visibleMat = newMaterial(&tcb, "TestCheckBtn");

		std::function<void(int)> kuwaharaSliderFunction = std::bind(&RemapUI::kuwaharaCallback, this, std::placeholders::_1);
		std::function<void(int)> averagerSliderFunction = std::bind(&RemapUI::averagerCallback, this, std::placeholders::_1);
		std::function<void(float)> gradientSliderFunction = std::bind(&RemapUI::gradientCallback, this, std::placeholders::_1);

		ImagePanel* outMap = new ImagePanel(loadList->getPtr(new Material(remapper.filteredOSNormal), "RemappedOS"), false);

		Arrangement* column = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.5f, 0.25f, 0.01f);

		Slider* kuwaharaKernSlider = new Slider(visibleMat, 0.0f, 0.0f, 0.25f, 0.25f);
		kuwaharaKernSlider->setSlideValues(remapper.minKuwaharaKernel, remapper.maxKuwaharaKernel, 5);
		kuwaharaKernSlider->setIntCallback(kuwaharaSliderFunction, false);

		Slider* averagerKernSlider = new Slider(visibleMat, 0.0f, 0.0f, 0.25f, 0.25f);
		averagerKernSlider->setSlideValues(remapper.minAveragerKernel, remapper.maxAveragerKernel, 5);
		averagerKernSlider->setIntCallback(averagerSliderFunction, false);

		Slider* gradientThreshSlider = new Slider(visibleMat, 0.0f, 0.0f, 0.25f, 0.25f);
		gradientThreshSlider->setSlideValues(remapper.minGradientThreshold, remapper.maxGradientThreshold, 0.06f);
		kuwaharaKernSlider->setFloatCallback(gradientSliderFunction, false);
		
		column->addItem(getPtr(outMap));
		column->addItem(getPtr(kuwaharaKernSlider));
		column->addItem(getPtr(averagerKernSlider));
		column->addItem(getPtr(gradientThreshSlider));

		column->updateDisplay();

		canvas.push_back(getPtr(column));

		isSetup = true;
	}

	int priorityLayer = 100;
private:
	RemapBackend remapper;

	void fullRemap(Texture*, Texture*);

	void kuwaharaCallback(int);
	void averagerCallback(int);
	void gradientCallback(float);
};

#endif
