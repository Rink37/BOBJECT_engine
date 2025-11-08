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
	if (diffTex == nullptr || OSNormTex == nullptr) {
		return;
	}
	std::cout << "Creating ref maps" << std::endl;
	baseDiffuse = diffTex->copyImage(VK_FORMAT_R8G8B8A8_UNORM, diffTex->textureLayout, diffTex->textureUsage, diffTex->textureTiling, diffTex->textureMemFlags, 1);
	
	OSNormTex->getCVMat();
	cv::resize(OSNormTex->texMat, OSNormTex->texMat, cv::Size(diffTex->texWidth, diffTex->texHeight));
	baseOSNormal = new imageTexture(OSNormTex->texMat, VK_FORMAT_R8G8B8A8_UNORM);
}

void RemapBackend::createBaseMaps() {
	if (baseDiffuse == nullptr) {
		std::cout << "No base found" << std::endl;
		return;
	}
	std::cout << "Creating base maps" << std::endl;
	filter Kuwahara(std::vector<Texture*>{baseDiffuse}, new KUWAHARASHADER, VK_FORMAT_R8G8B8A8_UNORM, paramBuffer, sizeof(RemapParamObject));
	Kuwahara.filterImage();
	
	filter SobelX(std::vector<Texture*>{Kuwahara.filterTarget[0]}, new SOBELXSHADER, VK_FORMAT_R16G16B16A16_SFLOAT);
	SobelX.filterImage();

	filter SobelY(std::vector<Texture*>{Kuwahara.filterTarget[0]}, new SOBELYSHADER, VK_FORMAT_R16G16B16A16_SFLOAT);
	SobelY.filterImage();

	xGradients = SobelX.filterTarget[0]->copyImage();
	yGradients = SobelY.filterTarget[0]->copyImage();
	
	Kuwahara.cleanup();
	SobelX.cleanup();
	SobelY.cleanup();
}

void RemapBackend::performRemap() {
	if (baseOSNormal == nullptr || xGradients == nullptr) {
		std::cout << "No gradients found" << std::endl;
		return;
	}
	std::cout << "Remapping" << std::endl;
	filter Averager(std::vector<Texture*>{baseOSNormal, xGradients, yGradients}, new AVERAGERSHADER, VK_FORMAT_R8G8B8A8_UNORM, paramBuffer, sizeof(RemapParamObject));
	Averager.filterImage();
	
	std::cout << "Gradient calculating" << std::endl;
	filter gradRemap(std::vector<Texture*>{Averager.filterTarget[0], xGradients, yGradients}, new GRADREMAPSHADER, VK_FORMAT_R8G8B8A8_UNORM, paramBuffer, sizeof(RemapParamObject));
	gradRemap.filterImage();

	std::cout << "Copying" << std::endl;
	filteredOSNormal = gradRemap.filterTarget[0]->copyImage(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1);
	filteredOSNormal->textureImageView = filteredOSNormal->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

	Averager.cleanup();
	gradRemap.cleanup();
}

void RemapBackend::smootheResult() {
	if (baseDiffuse == nullptr || filteredOSNormal == nullptr) {
		std::cout << "Cannot smoothe" << std::endl;
		return;
	}
	std::cout << "Smoothing" << std::endl;
	filter referenceKuwahara(std::vector<Texture*>{baseDiffuse, filteredOSNormal}, new REFERENCEKUWAHARASHADER, VK_FORMAT_R8G8B8A8_UNORM, paramBuffer, sizeof(RemapParamObject));
	referenceKuwahara.filterImage();

	filteredOSNormal->cleanup();
	filteredOSNormal = nullptr;

	filteredOSNormal = referenceKuwahara.filterTarget[0]->copyImage(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1);
	filteredOSNormal->textureImageView = filteredOSNormal->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

	referenceKuwahara.cleanup();
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
	std::cout << "Kuwahara callback" << std::endl;
	remapper.setKuwaharaKernel(kern);
	remapper.createBaseMaps();
	remapper.performRemap();
	remapper.smootheResult();
}

void RemapUI::averagerCallback(int kern) {
	std::cout << "Averager callback" << std::endl;
	remapper.setAveragerKernel(kern);
	remapper.performRemap();
	remapper.smootheResult();
}

void RemapUI::gradientCallback(float thresh) {
	std::cout << "Gradient callback" << std::endl;
	remapper.setGradientThreshold(thresh);
	remapper.performRemap();
	remapper.smootheResult();
}