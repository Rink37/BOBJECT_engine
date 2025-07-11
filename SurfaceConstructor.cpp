#include"SurfaceConstructor.h"
#include"GenerateNormalMap.h"
#include"ImageProcessor.h"
#include"include/Kuwahara.h"
#include"include/SobelX.h"
#include"include/SobelY.h"

using namespace std;

void surfaceConstructor::generateOSMap(Mesh* inputMesh) {

	// This function can potentially be simplified using default image operations

	NormalGen generator;
	generator.setupOSExtractor();
	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
	commandBuffer = generator.drawOSMap(commandBuffer, inputMesh);
	Engine::get()->endSingleTimeCommands(commandBuffer);
	vkDestroyBuffer(Engine::get()->device, inputMesh->texCoordIndexBuffer, nullptr);
	vkFreeMemory(Engine::get()->device, inputMesh->texCoordIndexBufferMemory, nullptr);

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

	OSNormTex = new imageTexture;
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
		VkOffset3D blitSize;
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

	Normal[1] = new Material(OSNormTex);

	generator.cleanupOS();
}

void surfaceConstructor::contextConvert() {
	if (diffTex == nullptr || OSNormTex == nullptr) {
		return;
	}
	filter Kuwahara(diffTex, new KUWAHARASHADER);
	Kuwahara.filterImage();
	Texture* kuwaharaFiltered = new imageTexture(Kuwahara.filterTarget->texMat);
	filter SobelX(kuwaharaFiltered, new SOBELXSHADER);
	SobelX.filterImage();
	filter SobelY(kuwaharaFiltered, new SOBELYSHADER);
	SobelY.filterImage();
}

//void surfaceConstructor::contextConvert() {
//	if (diffTex == nullptr || OSNormTex == nullptr) {
//		// We need both of these images to perform context conversion
//		return;
//	}
//	// OpenCV functions are used for conversion so we use CV matrices instead of engine images
//	OSNormTex->getCVMat();
//	diffTex->getCVMat();
//	NormalGen generator;
//	generator.OSNormalMap = OSNormTex->texMat;
//	generator.contextualConvertMap(diffTex->texMat);
//	//imageTexture* convertedOS = new imageTexture(generator.OSNormalMap);
//	//loadNormal(convertedOS);
//}

void surfaceConstructor::transitionToTS(Mesh* inputMesh) {
	NormalGen generator;
	OSNormTex->getCVMat();
	generator.createOSImageFromMat(OSNormTex->texMat);
	generator.setupTSExtractor();
	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
	commandBuffer = generator.convertOStoTS(commandBuffer, inputMesh);
	Engine::get()->endSingleTimeCommands(commandBuffer);
	vkDestroyBuffer(Engine::get()->device, inputMesh->texCoordIndexBuffer, nullptr);
	vkFreeMemory(Engine::get()->device, inputMesh->texCoordIndexBufferMemory, nullptr);

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

	TSNormTex = new imageTexture;
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
		VkOffset3D blitSize;
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

	Normal[2] = new Material(TSNormTex);
	generator.cleanupTS();
}