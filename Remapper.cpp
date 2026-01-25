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

	baseDiffuse = diffTex->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height);
	//baseDiffuse = diffTex->copyImage(VK_FORMAT_R8G8B8A8_UNORM, diffTex->textureLayout, diffTex->textureUsage, diffTex->textureTiling, diffTex->textureMemFlags, 1, width, height);
	baseOSNormal = OSNormTex->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height);

	if (Kuwahara == nullptr) {
		Kuwahara = new filter(std::vector<Texture*>{baseDiffuse}, new KUWAHARASHADER, VK_FORMAT_R8G8B8A8_UNORM, paramBuffer, sizeof(RemapParamObject));
	}
	Kuwahara->filterImage();

	if (SobelCombined == nullptr) {
		SobelCombined = new filter(std::vector<Texture*>{Kuwahara->filterTarget[0]}, new SOBELCOMBINEDSHADER, VK_FORMAT_R16G16_SFLOAT);
	}
	SobelCombined->filterImage();

	if (Averager == nullptr) {
		Averager = new filter(std::vector<Texture*>{baseOSNormal, SobelCombined->filterTarget[0]}, new AVERAGERSHADER, VK_FORMAT_R8G8B8A8_UNORM, paramBuffer, sizeof(RemapParamObject));
	}
	Averager->filterImage();

	if (gradRemap == nullptr) {
		gradRemap = new filter(std::vector<Texture*>{Averager->filterTarget[0], SobelCombined->filterTarget[0]}, new GRADREMAPSHADER, VK_FORMAT_R8G8B8A8_UNORM, paramBuffer, sizeof(RemapParamObject));
	}
	gradRemap->filterImage();

	if (referenceKuwahara == nullptr) {
		referenceKuwahara = new filter(std::vector<Texture*>{baseDiffuse, gradRemap->filterTarget[0]}, new REFERENCEKUWAHARASHADER, VK_FORMAT_R8G8B8A8_UNORM, paramBuffer, sizeof(RemapParamObject));
	}
	referenceKuwahara->filterImage();

	if (filteredOSNormal != nullptr) {
		filteredOSNormal->cleanup();
		filteredOSNormal = nullptr;
	}

	referenceKuwahara->filterTarget[0]->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	referenceKuwahara->filterTarget[0]->textureLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	filteredOSNormal = referenceKuwahara->filterTarget[0]->copyImage(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, baseWidth, baseHeight);
	filteredOSNormal->textureImageView = filteredOSNormal->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

	referenceKuwahara->filterTarget[0]->transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
	referenceKuwahara->filterTarget[0]->textureLayout = VK_IMAGE_LAYOUT_GENERAL;
}

void RemapBackend::createBaseMaps() {
	if (baseDiffuse == nullptr) {
		return;
	}

	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeComputeCommand();
	Kuwahara->filterImage(commandBuffer);
	SobelCombined->filterImage(commandBuffer);
	Engine::get()->endSingleTimeComputeCommand(commandBuffer);
}

void RemapBackend::performRemap() {
	if (baseOSNormal == nullptr) {
		return;
	}

	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeComputeCommand();
	Averager->filterImage(commandBuffer);
	gradRemap->filterImage(commandBuffer);
	Engine::get()->endSingleTimeComputeCommand(commandBuffer);

	if (!smoothePass) {
		vkDeviceWaitIdle(Engine::get()->device);

		if (filteredOSNormal != nullptr) {
			filteredOSNormal->cleanup();
		}

		gradRemap->filterTarget[0]->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		gradRemap->filterTarget[0]->textureLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		filteredOSNormal = gradRemap->filterTarget[0]->copyImage(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1);
		filteredOSNormal->textureImageView = filteredOSNormal->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

		gradRemap->filterTarget[0]->transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
		gradRemap->filterTarget[0]->textureLayout = VK_IMAGE_LAYOUT_GENERAL;
	}
	
}

void RemapBackend::smootheResult() {
	if (baseDiffuse == nullptr) {
		return;
	}

	referenceKuwahara->filterImage();

	if (filteredOSNormal != nullptr) {
		filteredOSNormal->cleanup();
		filteredOSNormal = nullptr;
	}

	referenceKuwahara->filterTarget[0]->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	referenceKuwahara->filterTarget[0]->textureLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	
	filteredOSNormal = referenceKuwahara->filterTarget[0]->copyImage(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, baseWidth, baseHeight);
	filteredOSNormal->textureImageView = filteredOSNormal->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

	referenceKuwahara->filterTarget[0]->transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
	referenceKuwahara->filterTarget[0]->textureLayout = VK_IMAGE_LAYOUT_GENERAL;
}

void RemapBackend::cleanup() {
	if (baseDiffuse != nullptr) {
		baseDiffuse->cleanup();
		baseOSNormal->cleanup();
		baseDiffuse = nullptr;
		baseOSNormal = nullptr;
	}
	if (filteredOSNormal != nullptr) {
		filteredOSNormal->cleanup();
		filteredOSNormal = nullptr;
	}

	Kuwahara -> cleanup();
	SobelCombined -> cleanup();
	Averager -> cleanup();
	gradRemap -> cleanup();
	referenceKuwahara -> cleanup();
	vkDestroyBuffer(Engine::get()->device, paramBuffer, nullptr);
	vkFreeMemory(Engine::get()->device, paramBufferMemory, nullptr);
}

void RemapUI::fullRemap(Texture*diffTex, Texture*OSNormTex) {
	remapper->createReferenceMaps(diffTex, OSNormTex);
}

void RemapUI::kuwaharaCallback(int kern) {
	remapper->setKuwaharaKernel(kern);

	remapper->createBaseMaps();
	remapper->performRemap();
	if (remapper->smoothePass) {
		remapper->smootheResult();
	}
	outMap->image->mat[0] = loadList->replacePtr(new Material(remapper->filteredOSNormal), "RemapOSMat");
	sConst->normalType = 0;
	sConst->loadNormal(remapper->filteredOSNormal->copyTexture());
}

void RemapUI::zeroCrossCallback(float zeroCross) {
	remapper->setZeroCross(zeroCross);
	
	remapper->createBaseMaps();
	remapper->performRemap();
	if (remapper->smoothePass) {
		remapper->smootheResult();
	}
	outMap->image->mat[0] = loadList->replacePtr(new Material(remapper->filteredOSNormal), "RemapOSMat");
	sConst->normalType = 0;
	sConst->loadNormal(remapper->filteredOSNormal->copyTexture());
}

void RemapUI::sharpnessCallback(float sharpness) {
	remapper->setKuwaharaSharpness(sharpness);
	remapper->createBaseMaps();
	remapper->performRemap();
	if (remapper->smoothePass) {
		remapper->smootheResult();
	}
	outMap->image->mat[0] = loadList->replacePtr(new Material(remapper->filteredOSNormal), "RemapOSMat");
	sConst->normalType = 0;
	sConst->loadNormal(remapper->filteredOSNormal->copyTexture());
}
void RemapUI::hardnessCallback(float hardness) {
	remapper->setKuwaharaHardness(hardness);
	remapper->createBaseMaps();
	remapper->performRemap();
	if (remapper->smoothePass) {
		remapper->smootheResult();
	}
	outMap->image->mat[0] = loadList->replacePtr(new Material(remapper->filteredOSNormal), "RemapOSMat");
	sConst->normalType = 0;
	sConst->loadNormal(remapper->filteredOSNormal->copyTexture());
}

void RemapUI::averagerCallback(int kern) {
	remapper->setAveragerKernel(kern);
	remapper->performRemap();
	if (remapper->smoothePass) {
		remapper->smootheResult();
	}
	outMap->image->mat[0] = loadList->replacePtr(new Material(remapper->filteredOSNormal), "RemapOSMat");
	sConst->normalType = 0;
	sConst->loadNormal(remapper->filteredOSNormal->copyTexture());
}

void RemapUI::gradientCallback(float thresh) {
	remapper->setGradientThreshold(thresh);
	remapper->performRemap();
	if (remapper->smoothePass) {
		remapper->smootheResult();
	}
	outMap->image->mat[0] = loadList->replacePtr(new Material(remapper->filteredOSNormal), "RemapOSMat");
	sConst->normalType = 0;
	sConst->loadNormal(remapper->filteredOSNormal->copyTexture());
}