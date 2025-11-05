#include"SurfaceConstructor.h"
#include"GenerateNormalMap.h"
#include"ImageProcessor.h"
#include"WindowsFileManager.h"

#include"include/Kuwahara.h"
#include"include/SobelX.h"
#include"include/SobelY.h"
#include"include/ReferenceKuwahara.h"
#include"include/Averager.h"
#include"include/GradRemap.h"

using namespace std;
using namespace cv;

void surfaceConstructor::generateOSMap(Mesh* inputMesh) {

	NormalGen generator(loadList);
	generator.setupOSExtractor();
	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
	commandBuffer = generator.drawOSMap(commandBuffer, inputMesh);
	Engine::get()->endSingleTimeCommands(commandBuffer);
	
	OSNormTex = loadList->replacePtr(generator.objectSpaceMap.colour->copyImage(VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_TILING_LINEAR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1), "OSNormalTex");
	
	OSNormTex->textureImageView = OSNormTex->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

	Normal[1] = loadList->replacePtr(new Material(OSNormTex), "OSNormMat");

	generator.cleanupGenOS();
}

void surfaceConstructor::contextConvert() {
	if (diffTex == nullptr || OSNormTex == nullptr) {
		return;
	}

	auto start = std::chrono::high_resolution_clock::now();

	Texture* kuwaharaTex = diffTex->copyImage(VK_FORMAT_R8G8B8A8_UNORM, diffTex->textureLayout, diffTex->textureUsage, diffTex->textureTiling, diffTex->textureMemFlags, 1);

	cout << "Kuwahara" << endl;
	filter Kuwahara(kuwaharaTex, new KUWAHARASHADER, VK_FORMAT_R8G8B8A8_UNORM);
	Kuwahara.filterImage();
	
	cout << "SobelX" << endl;
	filter SobelX(Kuwahara.filterTarget[0], new SOBELXSHADER, VK_FORMAT_R16G16B16A16_SFLOAT);
	SobelX.filterImage();
	
	cout << "SobelY" << endl;
	filter SobelY(Kuwahara.filterTarget[0], new SOBELYSHADER, VK_FORMAT_R16G16B16A16_SFLOAT);
	SobelY.filterImage();

	// Presumably this block is faster than using compute to recreate the OS map, but I haven't checked
	OSNormTex->getCVMat(); 
	cv::resize(OSNormTex->texMat, OSNormTex->texMat, cv::Size(diffTex->texWidth, diffTex->texHeight));
	Texture* interrimNorm = new imageTexture(OSNormTex->texMat, VK_FORMAT_R8G8B8A8_UNORM);

	cout << "Averager" << endl;
	filter Averager(interrimNorm, SobelX.filterTarget[0], SobelY.filterTarget[0], new AVERAGERSHADER, VK_FORMAT_R8G8B8A8_UNORM);
	Averager.filterImage();

	interrimNorm->cleanup();
	delete interrimNorm;

	cout << "GradRemap" << endl;
	filter gradRemap(Averager.filterTarget[0], SobelX.filterTarget[0], SobelY.filterTarget[0], new GRADREMAPSHADER, VK_FORMAT_R8G8B8A8_UNORM);
	gradRemap.filterImage();

	cout << "ReferenceKuwahara" << endl;
	filter referenceKuwahara(kuwaharaTex, gradRemap.filterTarget[0], new REFERENCEKUWAHARASHADER);
	referenceKuwahara.filterImage();

	auto end = std::chrono::high_resolution_clock::now();

	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

	cout << "Total filtering completed in " << duration.count() << "ms" << endl;

	referenceKuwahara.filterTarget[0]->getCVMat();

	kuwaharaTex->cleanup();
	delete kuwaharaTex;

	cv::imshow("Finished", referenceKuwahara.filterTarget[0]->texMat);
	cv::waitKey(0);

	normalType = 0;
	loadNormal(new imageTexture(referenceKuwahara.filterTarget[0]->texMat(Range(0, diffTex->texHeight), Range(0, diffTex->texWidth)), VK_FORMAT_R8G8B8A8_UNORM));

	updateSurfaceMat();

	Kuwahara.cleanup();
	SobelX.cleanup();
	SobelY.cleanup();
	Averager.cleanup();
	gradRemap.cleanup();
	referenceKuwahara.cleanup();
	diffTex->destroyCVMat();
	OSNormTex->destroyCVMat();
}

void surfaceConstructor::transitionToTS(Mesh* inputMesh) {
	
	NormalGen generator(loadList);
	generator.copyOSImage(OSNormTex);
	generator.setupTSConverter();
	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
	commandBuffer = generator.convertOStoTS(commandBuffer, inputMesh);
	Engine::get()->endSingleTimeCommands(commandBuffer);

	// see https://github.com/SaschaWillems/Vulkan/blob/master/examples/screenshot/screenshot.cpp 

	bool supportsBlit = true;

	VkFormatProperties formatProps;

	vkGetPhysicalDeviceFormatProperties(Engine::get()->physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
	if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
		std::cerr << "Device does not support blitting from optimal tiled images, using copy instead of blit!" << std::endl;
		supportsBlit = false;
	}

	vkGetPhysicalDeviceFormatProperties(Engine::get()->physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
	if (!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
		std::cerr << "Device does not support blitting to linear tiled images, using copy instead of blit!" << std::endl;
		supportsBlit = false;
	}

	TSNormTex = loadList->replacePtr(new Texture, "TSNormalTex");
	TSNormTex->texWidth = generator.tangentSpaceMap.colour->texWidth;
	TSNormTex->texHeight = generator.tangentSpaceMap.colour->texHeight;
	TSNormTex->textureFormat = VK_FORMAT_R8G8B8A8_UNORM;
	TSNormTex->textureUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	TSNormTex->textureTiling = VK_IMAGE_TILING_LINEAR;
	TSNormTex->mipLevels = 1;

	TSNormTex->createImage(VK_SAMPLE_COUNT_1_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	VkCommandBuffer copyCmd;

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = Engine::get()->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(Engine::get()->device, &allocInfo, &copyCmd) != VK_SUCCESS) {
		throw runtime_error("failed to allocate command buffer!");
	}

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(copyCmd, &beginInfo) != VK_SUCCESS) {
		throw runtime_error("failed to begin recording command buffer!");
	}

	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcAccessMask = 0;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.image = TSNormTex->textureImage;
	imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	vkCmdPipelineBarrier(
		copyCmd,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);

	imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	imageMemoryBarrier.image = generator.tangentSpaceMap.colour->textureImage;
	imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	vkCmdPipelineBarrier(
		copyCmd,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);

	if (supportsBlit)
	{
		VkOffset3D blitSize{};
		blitSize.x = generator.tangentSpaceMap.colour->texWidth;
		blitSize.y = generator.tangentSpaceMap.colour->texHeight;
		blitSize.z = 1;
		VkImageBlit imageBlitRegion{};
		imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.srcSubresource.layerCount = 1;
		imageBlitRegion.srcOffsets[1] = blitSize;
		imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.dstSubresource.layerCount = 1;
		imageBlitRegion.dstOffsets[1] = blitSize;

		vkCmdBlitImage(
			copyCmd,
			generator.tangentSpaceMap.colour->textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			TSNormTex->textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageBlitRegion,
			VK_FILTER_NEAREST);
	}
	else
	{
		VkImageCopy imageCopyRegion{};
		imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.srcSubresource.layerCount = 1;
		imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.dstSubresource.layerCount = 1;
		imageCopyRegion.extent.width = generator.tangentSpaceMap.colour->texWidth;
		imageCopyRegion.extent.height = generator.tangentSpaceMap.colour->texHeight;
		imageCopyRegion.extent.depth = 1;

		vkCmdCopyImage(
			copyCmd,
			generator.tangentSpaceMap.colour->textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			TSNormTex->textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageCopyRegion);
	}

	if (vkEndCommandBuffer(copyCmd) != VK_SUCCESS) {
		throw runtime_error("Failed to end command buffer");
	}

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &copyCmd;

	VkFence copyFence;
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	vkCreateFence(Engine::get()->device, &fenceInfo, nullptr, &copyFence);

	vkQueueSubmit(Engine::get()->graphicsQueue, 1, &submitInfo, copyFence);

	if (vkWaitForFences(Engine::get()->device, 1, &copyFence, VK_TRUE, UINT64_MAX) == VK_TIMEOUT) {
		throw runtime_error("Fence timeout");
	};

	vkFreeCommandBuffers(Engine::get()->device, Engine::get()->commandPool, 1, &copyCmd);

	vkDestroyFence(Engine::get()->device, copyFence, nullptr);

	TSNormTex->generateMipmaps();
	TSNormTex->textureImageView = TSNormTex->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

	Normal[2] = loadList->replacePtr(new Material(TSNormTex), "TSNormMat");
	generator.cleanupTS();
}

void surfaceConstructor::transitionToOS(Mesh* inputMesh) {
	NormalGen generator(loadList);
	generator.copyTSImage(TSNormTex);
	generator.setupOSConverter();
	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
	commandBuffer = generator.convertTStoOS(commandBuffer, inputMesh);
	Engine::get()->endSingleTimeCommands(commandBuffer);

	// see https://github.com/SaschaWillems/Vulkan/blob/master/examples/screenshot/screenshot.cpp 

	bool supportsBlit = true;

	VkFormatProperties formatProps;

	vkGetPhysicalDeviceFormatProperties(Engine::get()->physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
	if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
		std::cerr << "Device does not support blitting from optimal tiled images, using copy instead of blit!" << std::endl;
		supportsBlit = false;
	}

	vkGetPhysicalDeviceFormatProperties(Engine::get()->physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
	if (!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
		std::cerr << "Device does not support blitting to linear tiled images, using copy instead of blit!" << std::endl;
		supportsBlit = false;
	}

	OSNormTex = loadList->replacePtr(new Texture, "OSNormalTex");
	OSNormTex->texWidth = generator.objectSpaceMap.colour->texWidth;
	OSNormTex->texHeight = generator.objectSpaceMap.colour->texHeight;
	OSNormTex->textureFormat = VK_FORMAT_R8G8B8A8_UNORM;
	OSNormTex->textureUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	OSNormTex->textureTiling = VK_IMAGE_TILING_LINEAR;
	OSNormTex->mipLevels = 1;

	OSNormTex->createImage(VK_SAMPLE_COUNT_1_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	VkCommandBuffer copyCmd;

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = Engine::get()->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(Engine::get()->device, &allocInfo, &copyCmd) != VK_SUCCESS) {
		throw runtime_error("failed to allocate command buffer!");
	}

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(copyCmd, &beginInfo) != VK_SUCCESS) {
		throw runtime_error("failed to begin recording command buffer!");
	}

	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcAccessMask = 0;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.image = OSNormTex->textureImage;
	imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	vkCmdPipelineBarrier(
		copyCmd,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);

	imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	imageMemoryBarrier.image = generator.objectSpaceMap.colour->textureImage;
	imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	vkCmdPipelineBarrier(
		copyCmd,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);

	if (supportsBlit)
	{
		VkOffset3D blitSize{};
		blitSize.x = generator.objectSpaceMap.colour->texWidth;
		blitSize.y = generator.objectSpaceMap.colour->texHeight;
		blitSize.z = 1;
		VkImageBlit imageBlitRegion{};
		imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.srcSubresource.layerCount = 1;
		imageBlitRegion.srcOffsets[1] = blitSize;
		imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.dstSubresource.layerCount = 1;
		imageBlitRegion.dstOffsets[1] = blitSize;

		vkCmdBlitImage(
			copyCmd,
			generator.objectSpaceMap.colour->textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			OSNormTex->textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageBlitRegion,
			VK_FILTER_NEAREST);
	}
	else
	{
		VkImageCopy imageCopyRegion{};
		imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.srcSubresource.layerCount = 1;
		imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.dstSubresource.layerCount = 1;
		imageCopyRegion.extent.width = generator.objectSpaceMap.colour->texWidth;
		imageCopyRegion.extent.height = generator.objectSpaceMap.colour->texHeight;
		imageCopyRegion.extent.depth = 1;

		vkCmdCopyImage(
			copyCmd,
			generator.objectSpaceMap.colour->textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			OSNormTex->textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageCopyRegion);
	}

	if (vkEndCommandBuffer(copyCmd) != VK_SUCCESS) {
		throw runtime_error("Failed to end command buffer");
	}

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &copyCmd;

	VkFence copyFence;
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	vkCreateFence(Engine::get()->device, &fenceInfo, nullptr, &copyFence);

	vkQueueSubmit(Engine::get()->graphicsQueue, 1, &submitInfo, copyFence);

	if (vkWaitForFences(Engine::get()->device, 1, &copyFence, VK_TRUE, UINT64_MAX) == VK_TIMEOUT) {
		throw runtime_error("Fence timeout");
	};

	vkFreeCommandBuffers(Engine::get()->device, Engine::get()->commandPool, 1, &copyCmd);

	vkDestroyFence(Engine::get()->device, copyFence, nullptr);

	OSNormTex->generateMipmaps();
	OSNormTex->textureImageView = OSNormTex->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

	Normal[1] = loadList->replacePtr(new Material(OSNormTex), "OSNormMat");
	generator.cleanupOS();
}

void SurfaceMenu::setup(surfaceConstructor* surfConst, std::vector<StaticObject>* objects) {

	sConst = surfConst;
	sConst->loadList = loadList;
	staticObjects = objects;

	std::function<void(UIItem*)> addNormalButton = bind(&SurfaceMenu::createNormalMenu, this, placeholders::_1);
	std::function<void(UIItem*)> toggleDiffuse = bind(&SurfaceMenu::toggleDiffuseCam, this, placeholders::_1);
	std::function<void(UIItem*)> loadDiffuse = bind(&SurfaceMenu::loadDiffuseImage, this, placeholders::_1);
	std::function<void(UIItem*)> saveWebcam = bind(&SurfaceMenu::saveDiffuseImage, this, placeholders::_1);

	imageData diffuse = DIFFUSETEXT;
	Material* diffuseMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&diffuse), "DiffuseBtnTex")), "DiffuseBtnMat");

	imageData normal = NORMALTEXT;
	Material* normalMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&normal), "NormalBtnTex")), "NormalBtnMat");

	imageData webcamOn = WEBCAMONBUTTON;
	Material* webcamOnMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&webcamOn), "WebcamOnBtnTex")), "WebcamOnBtnMat");

	imageData webcamOff = WEBCAMOFFBUTTON;
	Material* webcamOffMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&webcamOff), "WebcamOffBtnTex")), "WebcamOffBtnMat");

	imageData OpenButton = OPENBUTTON;
	Material* openMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&OpenButton), "OpenBtnTex")), "OpenBtnMat");

	imageData SaveButton = SAVEBUTTON;
	Material* saveMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&SaveButton), "SaveBtnTex")), "SaveBtnMat");

	imageData plusButton = PLUSBUTTON;
	Material* plusMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&plusButton), "PlusBtnTex")), "PlusBtnMat");

	Button* diffuseTextPanel = new Button(diffuseMat);

	diffuseTog = new Checkbox(webcamOnMat, webcamOffMat, toggleDiffuse);
	diffuseTog->Name = "ToggleDiffuseWebcam";
	diffuseTog->setClickFunction(toggleDiffuse);

	Button* diffLoad = new Button(openMat, loadDiffuse);
	Button* diffSave = new Button(saveMat, saveWebcam);

	hArrangement* DiffuseButtons = new hArrangement(0.0f, 0.0f, 1.0f, 0.2f, 0.01f);

	DiffuseButtons->addItem(getPtr(diffuseTextPanel));
	DiffuseButtons->addItem(getPtr(diffuseTog));
	DiffuseButtons->addItem(getPtr(new spacer));
	DiffuseButtons->addItem(getPtr(diffLoad));
	DiffuseButtons->addItem(getPtr(diffSave));

	SurfacePanel = new vArrangement(1.0f, 0.0f, 0.25f, 0.8f, 0.01f);

	diffuseView = new ImagePanel(sConst->currentDiffuse(), true);
	SurfacePanel->addItem(getPtr(DiffuseButtons));
	SurfacePanel->addItem(getPtr(diffuseView));
	
	if (normalsEnabled) {
		Button* normalTextPanel = new Button(normalMat);
		Button* normalPlus = new Button(plusMat, addNormalButton);

		NormalButtons = new hArrangement(0.0f, 0.0f, 1.0f, 0.2f, 0.01f);

		NormalButtons->addItem(getPtr(normalTextPanel));
		NormalButtons->addItem(getPtr(normalPlus));
		NormalButtons->addItem(getPtr(new spacer));
		SurfacePanel->addItem(getPtr(NormalButtons));
	}
	
	SurfacePanel->addItem(getPtr(new spacer));

	SurfacePanel->updateDisplay();

	canvas.push_back(getPtr(SurfacePanel));

	hasNormal = false;
}

void SurfaceMenu::removeNormalMenu(UIItem* owner) {
	if (!hasNormal || !normalsEnabled) {
		return;
	}
	// Clear UI related to the normal component of the surface panel
	SurfacePanel->Items.at(SurfacePanel->Items.size() - 2)->cleanup();
	SurfacePanel->removeItem(static_cast<uint32_t>(SurfacePanel->Items.size() - 2));

	NormalButtons->cleanup();
	NormalButtons->Items.clear();

	std::function<void(UIItem*)> addNormalButton = bind(&SurfaceMenu::createNormalMenu, this, placeholders::_1);

	imageData normal = NORMALTEXT;
	Material* normalMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&normal), "NormalBtnTex")), "NormalBtnMat");

	imageData plusButton = PLUSBUTTON;
	Material* plusMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&plusButton), "PlusBtnTex")), "PlusBtnMat");

	Button* normalTextPanel = new Button(normalMat);
	Button* normalPlus = new Button(plusMat, addNormalButton);

	NormalButtons->addItem(getPtr(normalTextPanel));
	NormalButtons->addItem(getPtr(normalPlus));
	NormalButtons->addItem(getPtr(new spacer));

	SurfacePanel->updateDisplay();
	hasNormal = false;
}

void SurfaceMenu::toggleDiffuseCam(UIItem* owner) {
	sConst->toggleDiffWebcam();
	diffuseView->image->mat[0] = sConst->currentDiffuse();
}

void SurfaceMenu::loadDiffuseImage(UIItem* owner) {
	string fileName = winFile::OpenFileDialog();
	if (fileName != string("fail")) {
		imageTexture* loadedTexture = new imageTexture(fileName, VK_FORMAT_R8G8B8A8_SRGB);
		sConst->loadDiffuse(loadedTexture);
		sConst->diffuseIdx = 1;
		diffuseView->image->mat[0] = sConst->currentDiffuse();
		diffuseView->image->texHeight = diffuseView->image->mat[0]->textures[0]->texHeight;
		diffuseView->image->texWidth = diffuseView->image->mat[0]->textures[0]->texWidth;
		diffuseView->sqAxisRatio = static_cast<float>(diffuseView->image->texHeight) / static_cast<float>(diffuseView->image->texWidth);
		diffuseTog->activestate = false;
		diffuseTog->image->matidx = 1;
		sConst->updateSurfaceMat();
		SurfacePanel->arrangeItems();
		session::get()->currentStudio.diffusePath = fileName;
	}
}

void SurfaceMenu::saveDiffuseImage(UIItem* owner) {
	Mat saveDiffuse;
	if (sConst->diffuseIdx == 0) {
		saveDiffuse = webcamTexture::get()->webCam->webcamFrame;
	}
	else {
		sConst->diffTex->getCVMat();
		saveDiffuse = sConst->diffTex->texMat.clone();
		sConst->diffTex->destroyCVMat();
	}
	string saveName = winFile::SaveFileDialog();
	if (saveName != string("fail")) {
		session::get()->currentStudio.diffusePath = saveName;
		imwrite(saveName, saveDiffuse);
	}
}

void SurfaceMenu::toggleNormalCam(UIItem* owner) {
	sConst->toggleNormWebcam();
	normalView->image->mat[0] = sConst->currentNormal();
}

void SurfaceMenu::toggleNormalType(UIItem* owner) {
	sConst->toggleNormType();
	//cout << static_cast<int>(sConst->normalIdx) << " " << static_cast<int>(sConst->normalType) << endl;
	if (sConst->normalType == 1 && sConst->OSNormTex != nullptr && !sConst->TSmatching) {
		if (sConst->TSNormTex != nullptr) {
			//sConst->TSNormTex->cleanup();
			loadList->deleteTexture("TSNormalTex");
		}
		sConst->transitionToTS(staticObjects->at(staticObjects->size() - 1).mesh);
		sConst->TSmatching = true;

		std::cout << "Converted from OS to TS" << std::endl;

		//setNormal(sConst->currentNormal().get());
		//normalView->image->mat[0] = sConst->currentNormal().get();
	}
	if (sConst->normalType == 0 && sConst->TSNormTex != nullptr && !sConst->TSmatching) {
		if (sConst->OSNormTex != nullptr) {
			//sConst->OSNormTex->cleanup();
			loadList->deleteTexture("OSNormalTex");
		}
		sConst->transitionToOS(staticObjects->at(staticObjects->size() - 1).mesh);
		sConst->TSmatching = true;

		std::cout << "Converted from TS to OS" << std::endl;

		//setNormal(sConst->currentNormal().get());
		//normalView->image->mat[0] = sConst->currentNormal().get();
	}
	//else {
	//	setNormal(sConst->currentNormal().get());
	//	normalView->image->mat[0] = sConst->currentNormal().get();
	//}
	sConst->updateSurfaceMat();
	setNormal(sConst->currentNormal());
}

void SurfaceMenu::loadNormalImage(UIItem* owner) {
	string fileName = winFile::OpenFileDialog();
	if (fileName != string("fail")) {
		imageTexture* loadedTexture = new imageTexture(fileName, VK_FORMAT_R8G8B8A8_UNORM);
		sConst->loadNormal(loadedTexture);
		sConst->normalIdx = 1;
		normalView->image->mat[0] = sConst->currentNormal();
		normalView->image->texHeight = diffuseView->image->mat[0]->textures[0]->texHeight;
		normalView->image->texWidth = diffuseView->image->mat[0]->textures[0]->texWidth;
		normalView->sqAxisRatio = static_cast<float>(normalView->image->texHeight) / static_cast<float>(normalView->image->texWidth);
		normalTog->activestate = false;
		normalTog->image->matidx = 1;
		sConst->updateSurfaceMat();
		SurfacePanel->arrangeItems();
		if (!sConst->normalType) {
			session::get()->currentStudio.OSPath = fileName;
		}
		else {
			session::get()->currentStudio.TSPath = fileName;
		}
	}
}

void SurfaceMenu::saveNormalImage(UIItem* owner) {
	Mat saveNormal;
	if (sConst->normalIdx == 0) {
		saveNormal = webcamTexture::get()->webCam->webcamFrame;
	}
	else {
		if (sConst->normalType) {
			sConst->TSNormTex->getCVMat();
			saveNormal = sConst->TSNormTex->texMat.clone();
			sConst->TSNormTex->destroyCVMat();
		}
		else {
			sConst->OSNormTex->getCVMat();
			saveNormal = sConst->OSNormTex->texMat.clone();
			sConst->OSNormTex->destroyCVMat();
		}
	}
	string saveName = winFile::SaveFileDialog();
	if (saveName != string("fail")) {
		if (sConst->normalType == 0) {
			session::get()->currentStudio.OSPath = saveName;
		}
		else if (sConst->normalType == 1) {
			session::get()->currentStudio.TSPath = saveName;
		}
		imwrite(saveName, saveNormal);
	}
}

void SurfaceMenu::contextConvertMap(UIItem* owner) {
	sConst->contextConvert();
	sConst->normalIdx = 1;
	normalView->image->mat[0] = sConst->currentNormal();
	normalTog->activestate = false;
	normalTog->image->matidx = 1;
}

void SurfaceMenu::createNormalMenu(UIItem* owner) {

	if (staticObjects->size() == 0) {
		return;
	}

	std::function<void(UIItem*)> toggleWebcam = bind(&SurfaceMenu::toggleNormalCam, this, placeholders::_1);
	std::function<void(UIItem*)> toggleType = bind(&SurfaceMenu::toggleNormalType, this, placeholders::_1);
	std::function<void(UIItem*)> saveNorm = bind(&SurfaceMenu::saveNormalImage, this, placeholders::_1);
	std::function<void(UIItem*)> loadNorm = bind(&SurfaceMenu::loadNormalImage, this, placeholders::_1);
	std::function<void(UIItem*)> convertImg = bind(&SurfaceMenu::contextConvertMap, this, placeholders::_1);

	sConst->normalType = 0;

	if (sConst->OSNormTex == nullptr) {
		sConst->generateOSMap(staticObjects->at(staticObjects->size() - 1).mesh); // This function is definitely the source of the memory problems - I just don't know why
	}

	sConst->normalAvailable = true;
	webcamTexture::get()->changeFormat(VK_FORMAT_R8G8B8A8_UNORM);
	sConst->normalIdx = 1;
	sConst->updateSurfaceMat();

	SurfacePanel->removeItem(3);

	vector<UIImage*> images;
	NormalButtons->getImages(images);

	for (UIImage* image : images) {
		image->cleanup();
	}

	NormalButtons->Items.clear();

	imageData normal = NORMALTEXT;
	Material* normalMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&normal), "NormalBtnTex")), "NormalBtnMat");

	imageData webcamOn = WEBCAMONBUTTON;
	Material* webcamOnMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&webcamOn), "WebcamOnBtnTex")), "WebcamOnBtnMat");

	imageData webcamOff = WEBCAMOFFBUTTON;
	Material* webcamOffMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&webcamOff), "WebcamOffBtnTex")), "WebcamOffBtnMat");

	imageData OpenButton = OPENBUTTON;
	Material* openMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&OpenButton), "OpenBtnTex")), "OpenBtnMat");

	imageData SaveButton = SAVEBUTTON;
	Material* saveMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&SaveButton), "SaveBtnTex")), "SaveBtnMat");

	imageData plusButton = PLUSBUTTON;
	Material* plusMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&plusButton), "PlusBtnTex")), "PlusBtnMat");

	imageData osType = OSBUTTON;
	Material* osMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&osType), "OSBtnTex")), "OSBtnMat");

	imageData tsType = TANGENTSPACE;
	Material* tsMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&tsType), "TSBtnTex")), "TSBtnMat");

	imageData diffToNorm = D2NBUTTON;
	Material* dtnMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&diffToNorm), "DiffToNormBtnTex")), "DiffToNormBtnMat");

	Button* normalText = new Button(normalMat);

	normalTog = new Checkbox(webcamOnMat, webcamOffMat, toggleWebcam);
	normalTog->activestate = false;
	normalTog->image->matidx = 1;

	Checkbox* mapTypeToggle = new Checkbox(osMat, tsMat, toggleType);
	Button* copyLayout = new Button(dtnMat, convertImg);
	Button* normalLoad = new Button(openMat, loadNorm);
	Button* normalSave = new Button(saveMat, saveNorm);

	NormalButtons->addItem(getPtr(normalText));
	NormalButtons->addItem(getPtr(normalTog));
	NormalButtons->addItem(getPtr(mapTypeToggle));
	NormalButtons->addItem(getPtr(copyLayout));
	NormalButtons->addItem(getPtr(normalLoad));
	NormalButtons->addItem(getPtr(normalSave));

	normalView = new ImagePanel(sConst->currentNormal(), true);
	normalView->image->texHeight = diffuseView->image->texHeight;
	normalView->image->texWidth = diffuseView->image->texWidth;
	normalView->sqAxisRatio = static_cast<float>(normalView->image->texHeight) / static_cast<float>(normalView->image->texWidth);

	SurfacePanel->addItem(getPtr(normalView));
	SurfacePanel->addItem(getPtr(new spacer));
	SurfacePanel->updateDisplay();

	hasNormal = true;
}