#ifndef REMAPPER
#define REMAPPER

#include"UIelements.h"
#include"Bobject_Engine.h"

struct RemapParamObject {
	// Kuwahara params
	alignas(16) int kuwaharaKernelRadius;
	// We could potentially add zero cross/sharpness params but these values are also currently unused

	// The Sobel operators currently take no params

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
	}

	void updateParamBuffer();
	
	void createParamBuffer();
private:
	RemapParamObject params{};

	VkBuffer paramBuffer;
	VkDeviceMemory paramBufferMemory;
	void* paramBufferMapped;
};

class RemapUI : public Widget {

};

#endif
