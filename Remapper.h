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
	}

	void updateParamBuffer();
	
	void createParamBuffer();

	void createReferenceMaps(Texture*, Texture*);

	void createBaseMaps();
	void performRemap();
	void smootheResult();

	void cleanup();
private:
	RemapParamObject params{};

	VkBuffer paramBuffer = nullptr;
	VkDeviceMemory paramBufferMemory = nullptr;
	void* paramBufferMapped = nullptr;

	Texture* baseDiffuse = nullptr;
	Texture* baseOSNormal = nullptr;

	Texture* xGradients = nullptr;
	Texture* yGradients = nullptr;

	Texture* filteredOSNormal = nullptr;
};

class RemapUI : public Widget {

};

#endif
