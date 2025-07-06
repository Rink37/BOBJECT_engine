#include"SurfaceConstructor.h"
#include"GenerateNormalMap.h"

using namespace std;

void surfaceConstructor::generateOSMap(Mesh* inputMesh) {
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
	OSNormTex->texWidth = generator.objectSpaceMap.width;
	OSNormTex->texHeight = generator.objectSpaceMap.height;
	OSNormTex->textureFormat = VK_FORMAT_R8G8B8A8_UNORM;

	Engine::get()->mipLevels = static_cast<uint32_t>(floor(log2(max(OSNormTex->texWidth, OSNormTex->texHeight)))) + 1;

	VkImageCreateInfo imageCreateCi = {};
	imageCreateCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateCi.imageType = VK_IMAGE_TYPE_2D;
	imageCreateCi.format = OSNormTex->textureFormat;
	imageCreateCi.extent.width = generator.objectSpaceMap.width;
	imageCreateCi.extent.height = generator.objectSpaceMap.height;
	imageCreateCi.extent.depth = 1;
	imageCreateCi.arrayLayers = 1;
	imageCreateCi.mipLevels = 1;
	imageCreateCi.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateCi.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateCi.tiling = VK_IMAGE_TILING_LINEAR;
	imageCreateCi.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	if (vkCreateImage(Engine::get()->device, &imageCreateCi, nullptr, &OSNormTex->textureImage) != VK_SUCCESS) {
		throw runtime_error("Failed to create image");
	}

	VkMemoryRequirements memRequirements;
	VkMemoryAllocateInfo memAllocInfo = {};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	vkGetImageMemoryRequirements(Engine::get()->device, OSNormTex->textureImage, &memRequirements);
	memAllocInfo.allocationSize = memRequirements.size;

	memAllocInfo.memoryTypeIndex = Engine::get()->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (vkAllocateMemory(Engine::get()->device, &memAllocInfo, nullptr, &OSNormTex->textureImageMemory) != VK_SUCCESS) {
		throw runtime_error("Failed to allocate memory");
	}
	if (vkBindImageMemory(Engine::get()->device, OSNormTex->textureImage, OSNormTex->textureImageMemory, 0) != VK_SUCCESS) {
		throw runtime_error("Failed to bind image memory");
	}

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
	imageMemoryBarrier.image = generator.objectSpaceMap.colour.image;
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
		blitSize.x = generator.objectSpaceMap.width;
		blitSize.y = generator.objectSpaceMap.height;
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
			generator.objectSpaceMap.colour.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
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
		imageCopyRegion.extent.width = generator.objectSpaceMap.width;
		imageCopyRegion.extent.height = generator.objectSpaceMap.height;
		imageCopyRegion.extent.depth = 1;

		vkCmdCopyImage(
			copyCmd,
			generator.objectSpaceMap.colour.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
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

	OSNormTex->generateMipmaps(OSNormTex->textureImage, OSNormTex->textureFormat, OSNormTex->texWidth, OSNormTex->texHeight, 1);

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = OSNormTex->textureImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = OSNormTex->textureFormat; 

	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(Engine::get()->device, &viewInfo, nullptr, &OSNormTex->textureImageView) != VK_SUCCESS) {
		throw runtime_error("failed to create texture image view!");
	}

	Normal[1] = new Material(OSNormTex);

	generator.cleanupOS();
}

void surfaceConstructor::contextConvert() {
	if (diffTex == nullptr || OSNormTex == nullptr) {
		// We need both of these images to perform context conversion
		return;
	}
	// OpenCV functions are used for conversion so we use CV matrices instead of engine images
	OSNormTex->getCVMat();
	diffTex->getCVMat();
	NormalGen generator;
	generator.OSNormalMap = OSNormTex->texMat;
	generator.contextualConvertMap(diffTex->texMat);
	//imageTexture* convertedOS = new imageTexture(generator.OSNormalMap);
	//loadNormal(convertedOS);
}

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
	TSNormTex->texWidth = generator.tangentSpaceMap.width;
	TSNormTex->texHeight = generator.tangentSpaceMap.height;
	TSNormTex->textureFormat = VK_FORMAT_R8G8B8A8_UNORM;

	VkImageCreateInfo imageCreateCi = {};
	imageCreateCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateCi.imageType = VK_IMAGE_TYPE_2D;
	imageCreateCi.format = TSNormTex->textureFormat;
	imageCreateCi.extent.width = generator.tangentSpaceMap.width;
	imageCreateCi.extent.height = generator.tangentSpaceMap.height;
	imageCreateCi.extent.depth = 1;
	imageCreateCi.arrayLayers = 1;
	imageCreateCi.mipLevels = 1;
	imageCreateCi.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateCi.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateCi.tiling = VK_IMAGE_TILING_LINEAR;
	imageCreateCi.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	if (vkCreateImage(Engine::get()->device, &imageCreateCi, nullptr, &TSNormTex->textureImage) != VK_SUCCESS) {
		throw runtime_error("Failed to create image");
	}

	VkMemoryRequirements memRequirements;
	VkMemoryAllocateInfo memAllocInfo = {};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	vkGetImageMemoryRequirements(Engine::get()->device, TSNormTex->textureImage, &memRequirements);
	memAllocInfo.allocationSize = memRequirements.size;

	memAllocInfo.memoryTypeIndex = Engine::get()->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (vkAllocateMemory(Engine::get()->device, &memAllocInfo, nullptr, &TSNormTex->textureImageMemory) != VK_SUCCESS) {
		throw runtime_error("Failed to allocate memory");
	}
	if (vkBindImageMemory(Engine::get()->device, TSNormTex->textureImage, TSNormTex->textureImageMemory, 0) != VK_SUCCESS) {
		throw runtime_error("Failed to bind image memory");
	}

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
	imageMemoryBarrier.image = generator.tangentSpaceMap.colour.image;
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
		blitSize.x = generator.tangentSpaceMap.width;
		blitSize.y = generator.tangentSpaceMap.height;
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
			generator.tangentSpaceMap.colour.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
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
		imageCopyRegion.extent.width = generator.tangentSpaceMap.width;
		imageCopyRegion.extent.height = generator.tangentSpaceMap.height;
		imageCopyRegion.extent.depth = 1;

		vkCmdCopyImage(
			copyCmd,
			generator.tangentSpaceMap.colour.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
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

	TSNormTex->generateMipmaps(TSNormTex->textureImage, TSNormTex->textureFormat, TSNormTex->texWidth, TSNormTex->texHeight, 1);

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = TSNormTex->textureImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = TSNormTex->textureFormat;

	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(Engine::get()->device, &viewInfo, nullptr, &TSNormTex->textureImageView) != VK_SUCCESS) {
		throw runtime_error("failed to create texture image view!");
	}

	Normal[2] = new Material(TSNormTex);
	generator.cleanupTS();
}