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
	baseDiffuse = diffTex->copyImage(VK_FORMAT_R8G8B8A8_UNORM, diffTex->textureLayout, diffTex->textureUsage, diffTex->textureTiling, diffTex->textureMemFlags, 1);
	
	OSNormTex->getCVMat();
	cv::resize(OSNormTex->texMat, OSNormTex->texMat, cv::Size(diffTex->texWidth, diffTex->texHeight));
	baseOSNormal = new imageTexture(OSNormTex->texMat, VK_FORMAT_R8G8B8A8_UNORM);
}

void RemapBackend::createBaseMaps() {
	if (baseDiffuse == nullptr) {
		return;
	}
	std::cout << "Creating base maps" << std::endl;

	filter Kuwahara(std::vector<Texture*>{baseDiffuse}, new KUWAHARASHADER, VK_FORMAT_R8G8B8A8_UNORM, paramBuffer, sizeof(RemapParamObject));
	Kuwahara.filterImage();

	Texture* KuwaharaTex = Kuwahara.filterTarget[0]->copyImage(VK_FORMAT_R8G8B8A8_SRGB, Kuwahara.filterTarget[0]->textureLayout, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, Kuwahara.filterTarget[0]->textureTiling, Kuwahara.filterTarget[0]->textureMemFlags, 1);
	KuwaharaTex->getCVMat();
	cv::imshow("Kuwahara", KuwaharaTex->texMat);
	cv::waitKey(0);
	
	filter SobelX(std::vector<Texture*>{Kuwahara.filterTarget[0]}, new SOBELXSHADER, VK_FORMAT_R16G16B16A16_SFLOAT);
	SobelX.filterImage();

	filter SobelY(std::vector<Texture*>{Kuwahara.filterTarget[0]}, new SOBELYSHADER, VK_FORMAT_R16G16B16A16_SFLOAT);
	SobelY.filterImage();

	if (xGradients != nullptr) {
		xGradients->cleanup();
		xGradients = nullptr;
		yGradients->cleanup();
		yGradients = nullptr;
	}

	xGradients = SobelX.filterTarget[0]->copyImage();
	yGradients = SobelY.filterTarget[0]->copyImage();
	
	Kuwahara.cleanup();
	SobelX.cleanup();
	SobelY.cleanup();
}

void RemapBackend::performRemap() {
	if (baseOSNormal == nullptr || xGradients == nullptr) {
		return;
	}

	filter Averager(std::vector<Texture*>{baseOSNormal, xGradients, yGradients}, new AVERAGERSHADER, VK_FORMAT_R8G8B8A8_UNORM, paramBuffer, sizeof(RemapParamObject));
	Averager.filterImage();
	
	Averager.filterTarget[0]->getCVMat();
	cv::imshow("Averaged", Averager.filterTarget[0]->texMat);

	filter gradRemap(std::vector<Texture*>{Averager.filterTarget[0], xGradients, yGradients}, new GRADREMAPSHADER, VK_FORMAT_R8G8B8A8_UNORM, paramBuffer, sizeof(RemapParamObject));
	gradRemap.filterImage();

	gradRemap.filterTarget[0]->getCVMat();
	cv::imshow("grad remapped", gradRemap.filterTarget[0]->texMat);

	if (filteredOSNormal != nullptr) {
		filteredOSNormal->cleanup();
	}
	
	filteredOSNormal = gradRemap.filterTarget[0]->copyImage(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1);
	filteredOSNormal->textureImageView = filteredOSNormal->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

	Averager.cleanup();
	gradRemap.cleanup();
}

void RemapBackend::smootheResult() {
	if (baseDiffuse == nullptr || filteredOSNormal == nullptr) {
		return;
	}
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
	remapper.setKuwaharaKernel(kern);
	std::cout << "kuwahara" << std::endl;
	remapper.createBaseMaps();
	remapper.performRemap();
	//remapper.smootheResult();
	outMap->image->mat[0] = loadList->replacePtr(new Material(remapper.filteredOSNormal), "RemapOSMat");
	sConst->normalType = 0;
	sConst->loadNormal(remapper.filteredOSNormal->copyTexture());
}

void RemapUI::averagerCallback(int kern) {
	remapper.setAveragerKernel(kern);
	std::cout << "Averager" << std::endl;
	remapper.performRemap();
	//remapper.smootheResult();
	outMap->image->mat[0] = loadList->replacePtr(new Material(remapper.filteredOSNormal), "RemapOSMat");
	sConst->normalType = 0;
	sConst->loadNormal(remapper.filteredOSNormal->copyTexture());
}

void RemapUI::gradientCallback(float thresh) {
	remapper.setGradientThreshold(thresh);
	std::cout << "Gradient" << std::endl;
	remapper.performRemap();
	//remapper.smootheResult();
	outMap->image->mat[0] = loadList->replacePtr(new Material(remapper.filteredOSNormal), "RemapOSMat");
	sConst->normalType = 0;
	sConst->loadNormal(remapper.filteredOSNormal->copyTexture());
}