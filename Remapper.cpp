#include"Remapper.h"

void RemapBackend::createParamBuffer() {
	VkDeviceSize bufferSize = sizeof(RemapParamObject);

	Engine::get()->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, paramBuffer, paramBufferMemory);

	vkMapMemory(Engine::get()->device, paramBufferMemory, 0, bufferSize, 0, &paramBufferMapped);
}

void RemapBackend::updateParamBuffer() {
	memcpy(paramBufferMapped, &params, sizeof(params));
}

void RemapBackend::createReferenceMaps(Texture* diffTex, Texture* OSNormTex) {
	baseDiffuse = diffTex->copyImage(VK_FORMAT_R8G8B8A8_UNORM, diffTex->textureLayout, diffTex->textureUsage, diffTex->textureTiling, diffTex->textureMemFlags, 1);
	
	OSNormTex->getCVMat();
	cv::resize(OSNormTex->texMat, OSNormTex->texMat, cv::Size(diffTex->texWidth, diffTex->texHeight));
	baseOSNormal = new imageTexture(OSNormTex->texMat, VK_FORMAT_R8G8B8A8_UNORM);
}

void RemapBackend::createBaseMaps() {
	if (baseDiffuse == nullptr) {
		return;
	}
	filter Kuwahara(baseDiffuse, new KUWAHARASHADER, VK_FORMAT_R8G8B8A8_UNORM);
	Kuwahara.filterImage();
	
	filter SobelX(Kuwahara.filterTarget[0], new SOBELXSHADER, VK_FORMAT_R16G16B16A16_SFLOAT);
	SobelX.filterImage();

	filter SobelY(Kuwahara.filterTarget[0], new SOBELYSHADER, VK_FORMAT_R16G16B16A16_SFLOAT);
	SobelY.filterImage();

	xGradients = SobelX.filterTarget[0]->copyImage();
	yGradients = SobelY.filterTarget[0]->copyImage();
	
	Kuwahara.cleanup();
}

void RemapBackend::performRemap() {
	if (baseOSNormal == nullptr || xGradients == nullptr) {
		return;
	}
	filter Averager(baseOSNormal, xGradients, yGradients, new AVERAGERSHADER, VK_FORMAT_R8G8B8A8_UNORM);
	Averager.filterImage();
	
	filter gradRemap(Averager.filterTarget[0], xGradients, yGradients, new GRADREMAPSHADER, VK_FORMAT_R8G8B8A8_UNORM);
	gradRemap.filterImage();

	filteredOSNormal = gradRemap.filterTarget[0]->copyImage();
}

void RemapBackend::smootheResult() {
	if (baseDiffuse == nullptr || filteredOSNormal == nullptr) {
		return;
	}
	filter referenceKuwahara(baseDiffuse, filteredOSNormal , new REFERENCEKUWAHARASHADER);
	referenceKuwahara.filterImage();

	filteredOSNormal->cleanup();
	filteredOSNormal = nullptr;

	filteredOSNormal = referenceKuwahara.filterTarget[0]->copyImage();
}

void RemapBackend::cleanup() {
	if (baseDiffuse != nullptr) {
		baseDiffuse->cleanup();
		baseOSNormal->cleanup();
	}
	if (xGradients != nullptr) {
		xGradients->cleanup();
		yGradients->cleanup();
	}
	if (filteredOSNormal != nullptr) {
		filteredOSNormal->cleanup();
	}
}

void RemapUI::fullRemap(Texture*diffTex, Texture*OSNormTex) {
	remapper.createReferenceMaps(diffTex, OSNormTex);
	remapper.createBaseMaps();
	remapper.performRemap();
	remapper.smootheResult();
}

void RemapUI::kuwaharaCallback(int kern) {
	remapper.setKuwaharaKernel(kern);
	remapper.createBaseMaps();
	remapper.performRemap();
	remapper.smootheResult();
}

void RemapUI::averagerCallback(int kern) {
	remapper.setAveragerKernel(kern);
	remapper.performRemap();
	remapper.smootheResult();
}

void RemapUI::gradientCallback(float thresh) {
	remapper.setGradientThreshold(thresh);
	remapper.performRemap();
	remapper.smootheResult();
}