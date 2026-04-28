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

	switch (method) {
	case (KUWAHARA):
		if (height > 1024) {
			height = 1024;
			width = static_cast<uint32_t>(static_cast<float>(baseWidth) / static_cast<float>(baseHeight) * 1024.0f);
		}
		baseDiffuse = diffTex->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height);
		baseOSNormal = OSNormTex->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height);

		if (colourConverter == nullptr) {
			colourConverter = new filter(std::vector<Texture*>{baseDiffuse}, new RGB_2_YCBCRSHADER, VK_FORMAT_R8G8B8A8_UNORM);
		}
		colourConverter->filterImage();

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
		break;
	case (ITERATIVE):
		baseDiffuse = diffTex->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height);
		baseOSNormal = OSNormTex->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height);
		if (colourConverter == nullptr) {
			colourConverter = new filter(std::vector<Texture*>{baseDiffuse}, new RGB_2_YCBCRSHADER, VK_FORMAT_R8G8B8A8_UNORM);
		}
		colourConverter->filterImage();

		if (SobelCombined == nullptr) {
			SobelCombined = new filter(std::vector<Texture*>{colourConverter->filterTarget[0]}, new SOBELCOMBINEDSHADER, VK_FORMAT_R16G16_SFLOAT);
		}
		SobelCombined->filterImage();

		filteredOSNormal = baseOSNormal->copyTexture();
		if (Averager == nullptr) {
			Averager = new filter(std::vector<Texture*>{filteredOSNormal, SobelCombined->filterTarget[0]}, new ITERATIVEAVERAGERSHADER, VK_FORMAT_R8G8B8A8_UNORM, paramBuffer, sizeof(RemapParamObject));
		}
		for (int i = 0; i != numIterations; i++) {
			Averager->filterImage();
		}

		filteredOSNormal->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		filteredOSNormal->textureLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		break;
	case (ITERATIVE_COORDMAP):
		baseDiffuse = diffTex->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height);
		baseOSNormal = OSNormTex->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height);

		if (colourConverter == nullptr) {
			colourConverter = new filter(std::vector<Texture*>{baseDiffuse}, new RGB_2_YCBCRSHADER, VK_FORMAT_R8G8B8A8_UNORM);
		}
		colourConverter->filterImage();

		if (SobelCombined == nullptr) {
			SobelCombined = new filter(std::vector<Texture*>{colourConverter->filterTarget[0]}, new SOBELCOMBINEDSHADER, VK_FORMAT_R16G16_SFLOAT);
		}
		SobelCombined->filterImage();

		if (coordMapCreator == nullptr) {
			coordMapCreator = new filter(std::vector<Texture*>{baseOSNormal}, new CREATECOORDMAPSHADER, VK_FORMAT_R16G16_SFLOAT);
		}
		coordMapCreator->filterImage();

		if (coordAverager == nullptr) {
			coordAverager = new filter(std::vector<Texture*>{coordMapCreator->filterTarget[0], SobelCombined->filterTarget[0]}, new COORDITERATIVEAVERAGERSHADER, VK_FORMAT_R16G16_SFLOAT, paramBuffer, sizeof(RemapParamObject));
		}
		for (int i = 0; i != numIterations; i++) {
			coordAverager->filterImage();
		}

		if (coordReader == nullptr) {
			coordReader = new filter(std::vector<Texture*>{baseOSNormal, coordMapCreator->filterTarget[0]}, new COORDMAPREADSHADER, VK_FORMAT_R8G8B8A8_UNORM);

		}
		coordReader->filterImage();

		if (filteredOSNormal != nullptr) {
			filteredOSNormal->cleanup();
			filteredOSNormal = nullptr;
		}

		coordReader->filterTarget[0]->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		coordReader->filterTarget[0]->textureLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		filteredOSNormal = coordReader->filterTarget[0]->copyImage(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, baseWidth, baseHeight);
		filteredOSNormal->textureImageView = filteredOSNormal->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

		coordReader->filterTarget[0]->transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
		coordReader->filterTarget[0]->textureLayout = VK_IMAGE_LAYOUT_GENERAL;
		break;
	default:
		baseDiffuse = diffTex->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height);
		baseOSNormal = OSNormTex->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height);
		if (colourConverter == nullptr) {
			colourConverter = new filter(std::vector<Texture*>{baseDiffuse}, new RGB_2_YCBCRSHADER, VK_FORMAT_R8G8B8A8_UNORM);
		}
		colourConverter->filterImage();

		if (SobelCombined == nullptr) {
			SobelCombined = new filter(std::vector<Texture*>{colourConverter->filterTarget[0]}, new SOBELCOMBINEDSHADER, VK_FORMAT_R16G16_SFLOAT);
		}
		SobelCombined->filterImage();

		filteredOSNormal = baseOSNormal->copyTexture();
		if (Averager == nullptr) {
			Averager = new filter(std::vector<Texture*>{filteredOSNormal, SobelCombined->filterTarget[0]}, new ITERATIVEAVERAGERSHADER, VK_FORMAT_R8G8B8A8_UNORM, paramBuffer, sizeof(RemapParamObject));
		}
		for (int i = 0; i != numIterations; i++) {
			Averager->filterImage();
		}

		filteredOSNormal->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		filteredOSNormal->textureLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		break;
	}
}

void RemapBackend::performRemap(VkCommandBuffer commandBuffer) {	
	VkImageCopy imageCopyRegion{};
	
	switch (method) {
	case (KUWAHARA):
		Kuwahara->filterImage(commandBuffer);
		SobelCombined->filterImage(commandBuffer);
		Averager->filterImage(commandBuffer);
		gradRemap->filterImage(commandBuffer);
		referenceKuwahara->filterImage(commandBuffer);

		Engine::get()->endSingleTimeComputeCommand(commandBuffer);

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
		break;
	case (ITERATIVE):
		imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.srcSubresource.layerCount = 1;
		imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.dstSubresource.layerCount = 1;
		imageCopyRegion.extent.width = baseOSNormal->texWidth;
		imageCopyRegion.extent.height = baseOSNormal->texHeight;
		imageCopyRegion.extent.depth = 1;

		filteredOSNormal->transitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
		filteredOSNormal->textureLayout = VK_IMAGE_LAYOUT_GENERAL;

		vkCmdCopyImage(commandBuffer, baseOSNormal->textureImage, baseOSNormal->textureLayout, filteredOSNormal->textureImage, filteredOSNormal->textureLayout, 1, &imageCopyRegion);
		Engine::get()->endSingleTimeComputeCommand(commandBuffer);

		for (int i = 0; i != numIterations; i++) {
			Averager->filterImage();
		}

		filteredOSNormal->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		filteredOSNormal->textureLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		break;
	case (ITERATIVE_COORDMAP):
		Engine::get()->endSingleTimeComputeCommand(commandBuffer);
		coordMapCreator->filterImage();
		for (int i = 0; i != numIterations; i++) {
			coordAverager->filterImage();
		}
		coordReader->filterImage();

		if (filteredOSNormal != nullptr) {
			filteredOSNormal->cleanup();
			filteredOSNormal = nullptr;
		}

		coordReader->filterTarget[0]->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		coordReader->filterTarget[0]->textureLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		filteredOSNormal = coordReader->filterTarget[0]->copyImage(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, baseWidth, baseHeight);
		filteredOSNormal->textureImageView = filteredOSNormal->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

		coordReader->filterTarget[0]->transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
		coordReader->filterTarget[0]->textureLayout = VK_IMAGE_LAYOUT_GENERAL;
		break;
	default:
		imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.srcSubresource.layerCount = 1;
		imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.dstSubresource.layerCount = 1;
		imageCopyRegion.extent.width = baseOSNormal->texWidth;
		imageCopyRegion.extent.height = baseOSNormal->texHeight;
		imageCopyRegion.extent.depth = 1;

		filteredOSNormal->transitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
		filteredOSNormal->textureLayout = VK_IMAGE_LAYOUT_GENERAL;

		vkCmdCopyImage(commandBuffer, baseOSNormal->textureImage, baseOSNormal->textureLayout, filteredOSNormal->textureImage, filteredOSNormal->textureLayout, 1, &imageCopyRegion);
		Engine::get()->endSingleTimeComputeCommand(commandBuffer);

		for (int i = 0; i != numIterations; i++) {
			Averager->filterImage();
		}
		filteredOSNormal->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		filteredOSNormal->textureLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		break;
	}
}

//void RemapBackend::smootheResult(VkCommandBuffer commandBuffer) {
//	referenceKuwahara->filterImage(commandBuffer);

//	Engine::get()->endSingleTimeComputeCommand(commandBuffer);

//	if (filteredOSNormal != nullptr) {
//		filteredOSNormal->cleanup();
//		filteredOSNormal = nullptr;
//	}

//	referenceKuwahara->filterTarget[0]->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
//	referenceKuwahara->filterTarget[0]->textureLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	
//	filteredOSNormal = referenceKuwahara->filterTarget[0]->copyImage(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, baseWidth, baseHeight);
//	filteredOSNormal->textureImageView = filteredOSNormal->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

//	referenceKuwahara->filterTarget[0]->transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
//	referenceKuwahara->filterTarget[0]->textureLayout = VK_IMAGE_LAYOUT_GENERAL;
//}

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

	switch (method) {
	case (KUWAHARA):
		Kuwahara->cleanup();
		gradRemap->cleanup();
		referenceKuwahara->cleanup();
		Averager->cleanup();
		break;
	case (ITERATIVE_COORDMAP):
		coordMapCreator->cleanup();
		coordAverager->cleanup();
		coordReader->cleanup();
		break;
	case (ITERATIVE):
		Averager->cleanup();
		break;
	default:
		break;
	}
	colourConverter->cleanup();
	SobelCombined->cleanup();
	
	
	vkDestroyBuffer(Engine::get()->device, paramBuffer, nullptr);
	vkFreeMemory(Engine::get()->device, paramBufferMemory, nullptr);
}

void RemapUI::fullRemap(Texture*diffTex, Texture*OSNormTex) {
	remapper->createReferenceMaps(diffTex, OSNormTex);
}

void RemapUI::kuwaharaCallback(int kern) {
	remapper->setKuwaharaKernel(kern);

	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeComputeCommand();
	remapper->performRemap(commandBuffer);
	outMap->image->mat[0] = loadList->replacePtr(new Material(remapper->filteredOSNormal), "RemapOSMat");
	sConst->normalType = 0;
	sConst->loadNormal(remapper->filteredOSNormal->copyTexture());
}

void RemapUI::zeroCrossCallback(float zeroCross) {
	remapper->setZeroCross(zeroCross);
	
	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeComputeCommand();
	remapper->performRemap(commandBuffer);
	outMap->image->mat[0] = loadList->replacePtr(new Material(remapper->filteredOSNormal), "RemapOSMat");
	sConst->normalType = 0;
	sConst->loadNormal(remapper->filteredOSNormal->copyTexture());
}

void RemapUI::sharpnessCallback(float sharpness) {
	remapper->setKuwaharaSharpness(sharpness);
	
	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeComputeCommand();
	remapper->performRemap(commandBuffer);
	outMap->image->mat[0] = loadList->replacePtr(new Material(remapper->filteredOSNormal), "RemapOSMat");
	sConst->normalType = 0;
	sConst->loadNormal(remapper->filteredOSNormal->copyTexture());
}
void RemapUI::hardnessCallback(float hardness) {
	remapper->setKuwaharaHardness(hardness);
	
	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeComputeCommand();
	remapper->performRemap(commandBuffer);
	outMap->image->mat[0] = loadList->replacePtr(new Material(remapper->filteredOSNormal), "RemapOSMat");
	sConst->normalType = 0;
	sConst->loadNormal(remapper->filteredOSNormal->copyTexture());
}

void RemapUI::averagerCallback(int kern) {
	remapper->setAveragerKernel(kern);
	
	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeComputeCommand();
	remapper->performRemap(commandBuffer);
	outMap->image->mat[0] = loadList->replacePtr(new Material(remapper->filteredOSNormal), "RemapOSMat");
	sConst->normalType = 0;
	sConst->loadNormal(remapper->filteredOSNormal->copyTexture());
}

void RemapUI::gradientCallback(float thresh) {
	remapper->setGradientThreshold(thresh);
	
	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeComputeCommand();
	remapper->performRemap(commandBuffer);
	outMap->image->mat[0] = loadList->replacePtr(new Material(remapper->filteredOSNormal), "RemapOSMat");
	sConst->normalType = 0;
	sConst->loadNormal(remapper->filteredOSNormal->copyTexture());
}

void RemapUI::iterationCallback(int count) {
	remapper->setIterationCount(count);

	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeComputeCommand();
	remapper->performRemap(commandBuffer);
	outMap->image->mat[0] = loadList->replacePtr(new Material(remapper->filteredOSNormal), "RemapOSMat");
	sConst->normalType = 0;
	sConst->loadNormal(remapper->filteredOSNormal->copyTexture());
};

void RemapUI::toggleNormalization(UIItem* owner) {
	remapper->toggleNormalization();

	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeComputeCommand();
	remapper->performRemap(commandBuffer);
	outMap->image->mat[0] = loadList->replacePtr(new Material(remapper->filteredOSNormal), "RemapOSMat");
	sConst->normalType = 0;
	sConst->loadNormal(remapper->filteredOSNormal->copyTexture());
};

void RemapUI::incrementMethod(UIItem*) {
	uint32_t newMethod = remapper->method + 1;
	newMethod %= METHODCOUNT;
	sConst->normalType = 0;
	remapper->baseOSNormal->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	remapper->baseOSNormal->textureLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	sConst->loadNormal(remapper->baseOSNormal->copyTexture());
	remapper->cleanup();
	delete remapper;
	remapper = nullptr;
	remapper = new RemapBackend();
	remapper->method = newMethod;
	remapper->setup();
	fullRemap(sConst->diffTex, sConst->OSNormTex);
	outMap->image->mat[0] = loadList->replacePtr(new Material(remapper->filteredOSNormal), "RemapOSMat");
	sConst->normalType = 0;
	sConst->loadNormal(remapper->filteredOSNormal->copyTexture());
	//std::cout << "Should switch to method " << newMethod << std::endl;
	UIItem* endButtons = canvas[0]->Items[canvas[0]->Items.size() - 1];
	for (int i = 2; i != canvas[0]->Items.size()-1; i++) {
		canvas[0]->Items[i]->cleanup();
	}
	canvas[0]->Items.erase(canvas[0]->Items.begin()+2, canvas[0]->Items.end());
	createUI();
	canvas[0]->addItem(endButtons);
	canvas[0]->updateDisplay();
};

void RemapUI::reduceMethod(UIItem*) {
	uint32_t newMethod = (remapper->method >= 1)?remapper->method - 1 : METHODCOUNT - 1;
	sConst->normalType = 0;
	remapper->baseOSNormal->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	remapper->baseOSNormal->textureLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	sConst->loadNormal(remapper->baseOSNormal->copyTexture());
	remapper->cleanup();
	delete remapper;
	remapper = nullptr;
	remapper = new RemapBackend();
	remapper->method = newMethod;
	remapper->setup();
	fullRemap(sConst->diffTex, sConst->OSNormTex);
	outMap->image->mat[0] = loadList->replacePtr(new Material(remapper->filteredOSNormal), "RemapOSMat");
	sConst->normalType = 0;
	sConst->loadNormal(remapper->filteredOSNormal->copyTexture());
	UIItem* endButtons = canvas[0]->Items[canvas[0]->Items.size() - 1];
	for (int i = 2; i != canvas[0]->Items.size() - 1; i++) {
		canvas[0]->Items[i]->cleanup();
	}
	canvas[0]->Items.erase(canvas[0]->Items.begin() + 2, canvas[0]->Items.end());
	createUI();
	canvas[0]->addItem(endButtons);
	canvas[0]->updateDisplay();
};