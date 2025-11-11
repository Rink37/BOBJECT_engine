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
	
	baseHeight = diffTex->texHeight;
	baseWidth = diffTex->texWidth;

	uint32_t height = baseHeight;
	uint32_t width = baseWidth;

	if (height > 1024) {
		height = 1024;
		width = static_cast<uint32_t>(static_cast<float>(baseWidth) / static_cast<float>(baseHeight) * 1024.0f);
	}

	std::cout << width << " " << height << std::endl;

	//OSNormTex->getCVMat();
	//cv::resize(OSNormTex->texMat, OSNormTex->texMat, cv::Size(diffTex->texWidth, diffTex->texHeight));
	baseDiffuse = diffTex->copyImage(VK_FORMAT_R8G8B8A8_UNORM, diffTex->textureLayout, diffTex->textureUsage, diffTex->textureTiling, diffTex->textureMemFlags, 1, width, height);
	baseOSNormal = OSNormTex->copyImage(width, height);
}

void RemapBackend::createBaseMaps() {
	if (baseDiffuse == nullptr) {
		return;
	}

	filter Kuwahara(std::vector<Texture*>{baseDiffuse}, new KUWAHARASHADER, VK_FORMAT_R8G8B8A8_UNORM, paramBuffer, sizeof(RemapParamObject));
	Kuwahara.filterImage();
	
	filter SobelCombined(std::vector<Texture*>{Kuwahara.filterTarget[0]}, new SOBELCOMBINEDSHADER, VK_FORMAT_R16G16_SFLOAT);
	SobelCombined.filterImage();

	if (gradients != nullptr) {
		gradients->cleanup();
		gradients = nullptr;
	}

	gradients = SobelCombined.filterTarget[0]->copyImage();
	
	Kuwahara.cleanup();
	SobelCombined.cleanup();
}

void RemapBackend::performRemap() {
	if (baseOSNormal == nullptr || gradients == nullptr) {
		return;
	}

	filter Averager(std::vector<Texture*>{baseOSNormal, gradients}, new AVERAGERSHADER, VK_FORMAT_R8G8B8A8_UNORM, paramBuffer, sizeof(RemapParamObject));
	Averager.filterImage();

	filter gradRemap(std::vector<Texture*>{Averager.filterTarget[0], gradients}, new GRADREMAPSHADER, VK_FORMAT_R8G8B8A8_UNORM, paramBuffer, sizeof(RemapParamObject));
	gradRemap.filterImage();

	if (filteredOSNormal != nullptr) {
		filteredOSNormal->cleanup();
	}
	
	filteredOSNormal = gradRemap.filterTarget[0]->copyImage(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1);
	if (!smoothePass) {
		filteredOSNormal->textureImageView = filteredOSNormal->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
	}
	
	Averager.cleanup();
	gradRemap.cleanup();
}

void RemapBackend::smootheResult() {
	if (baseDiffuse == nullptr || filteredOSNormal == nullptr) {
		return;
	}
	std::cout << "Smoothing" << std::endl;
	filter referenceKuwahara(std::vector<Texture*>{baseDiffuse, filteredOSNormal}, new REFERENCEKUWAHARASHADER, VK_FORMAT_R8G8B8A8_UNORM, paramBuffer, sizeof(RemapParamObject));
	referenceKuwahara.filterImage();

	filteredOSNormal->cleanup();
	filteredOSNormal = nullptr;

	filteredOSNormal = referenceKuwahara.filterTarget[0]->copyImage(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, baseWidth, baseHeight);
	filteredOSNormal->textureImageView = filteredOSNormal->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

	referenceKuwahara.cleanup();
}

void RemapBackend::cleanup() {
	if (baseDiffuse != nullptr) {
		baseDiffuse->cleanup();
		baseOSNormal->cleanup();
	}
	if (gradients != nullptr) {
		gradients->cleanup();
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
	if (remapper.smoothePass) {
		remapper.smootheResult();
	}
}

void RemapUI::kuwaharaCallback(int kern) {
	remapper.setKuwaharaKernel(kern);
	remapper.createBaseMaps();
	remapper.performRemap();
	if (remapper.smoothePass) {
		remapper.smootheResult();
	}
	outMap->image->mat[0] = loadList->replacePtr(new Material(remapper.filteredOSNormal), "RemapOSMat");
	sConst->normalType = 0;
	sConst->loadNormal(remapper.filteredOSNormal->copyTexture());
}

void RemapUI::averagerCallback(int kern) {
	remapper.setAveragerKernel(kern);
	remapper.performRemap();
	if (remapper.smoothePass) {
		remapper.smootheResult();
	}
	outMap->image->mat[0] = loadList->replacePtr(new Material(remapper.filteredOSNormal), "RemapOSMat");
	sConst->normalType = 0;
	sConst->loadNormal(remapper.filteredOSNormal->copyTexture());
}

void RemapUI::gradientCallback(float thresh) {
	remapper.setGradientThreshold(thresh);
	remapper.performRemap();
	if (remapper.smoothePass) {
		remapper.smootheResult();
	}
	outMap->image->mat[0] = loadList->replacePtr(new Material(remapper.filteredOSNormal), "RemapOSMat");
	sConst->normalType = 0;
	sConst->loadNormal(remapper.filteredOSNormal->copyTexture());
}