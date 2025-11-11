#include"Textures.h"

using namespace std;
using namespace cv;

bool Texture::hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

Texture* Texture::copyImage(uint32_t width, uint32_t height) {
	return copyImage(textureFormat, textureLayout, textureUsage, textureTiling, textureMemFlags, mipLevels, width, height);
}

Texture* Texture::copyImage() {
	// Full copy without modification
	return copyImage(textureFormat, textureLayout, textureUsage, textureTiling, textureMemFlags, mipLevels, texWidth, texHeight);
}

Texture* Texture::copyImage(VkFormat format, VkImageLayout layout, VkImageUsageFlags usage, VkImageTiling tiling, VkMemoryPropertyFlags memFlags, uint32_t mipLevels) {
	return copyImage(format, layout, usage, tiling, memFlags, mipLevels, texWidth, texHeight);
}

Texture* Texture::copyImage(VkFormat format, VkImageLayout layout, VkImageUsageFlags usage, VkImageTiling tiling, VkMemoryPropertyFlags memFlags, uint32_t mipLevels, uint32_t width, uint32_t height)
{
	Texture* copy = new Texture;
	
	bool supportsBlit = true;

	VkFormatProperties formatProps;

	vkGetPhysicalDeviceFormatProperties(Engine::get()->physicalDevice, textureFormat, &formatProps);
	if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
		std::cerr << "Device does not support blitting from optimal tiled images, using copy instead of blit!" << std::endl;
		supportsBlit = false;
	}

	vkGetPhysicalDeviceFormatProperties(Engine::get()->physicalDevice, textureFormat, &formatProps);
	if (!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
		std::cerr << "Device does not support blitting to linear tiled images, using copy instead of blit!" << std::endl;
		supportsBlit = false;
	}

	copy->textureFormat = format;
	copy->texWidth = width;
	copy->texHeight = height;
	copy->textureTiling = tiling;
	copy->textureUsage = usage;
	copy->mipLevels = mipLevels;
	copy->textureLayout = layout;

	copy->createImage(VK_SAMPLE_COUNT_1_BIT, memFlags);

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

	transitionImageLayout(copy->textureImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);

	if (textureLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		transitionImageLayout(textureImage, textureFormat, textureLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1);
	}
	else {
		// This is a bit of a quick fix - I just need to add this transition to the list of valid transitions in the function 
		transitionImageLayout(textureImage, textureFormat, textureLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
		transitionImageLayout(textureImage, textureFormat, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1);
	}

	if (supportsBlit)
	{
		VkOffset3D srcBlitSize;
		srcBlitSize.x = texWidth;
		srcBlitSize.y = texHeight;
		srcBlitSize.z = 1;
		VkOffset3D dstBlitSize;
		dstBlitSize.x = width;
		dstBlitSize.y = height;
		dstBlitSize.z = 1;
		VkImageBlit imageBlitRegion{};
		imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.srcSubresource.layerCount = 1;
		imageBlitRegion.srcOffsets[1] = srcBlitSize;
		imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.dstSubresource.layerCount = 1;
		imageBlitRegion.dstOffsets[1] = dstBlitSize;

		vkCmdBlitImage(
			copyCmd,
			textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			copy->textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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
		imageCopyRegion.extent.width = texWidth;
		imageCopyRegion.extent.height = texHeight;
		imageCopyRegion.extent.depth = 1;

		vkCmdCopyImage(
			copyCmd,
			textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			copy->textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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

	transitionImageLayout(copy->textureImage, copy->textureFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, copy->textureLayout, copy->mipLevels);
	transitionImageLayout(textureImage, textureFormat, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, textureLayout, 1);

	return copy;
}

Texture* Texture::copyTexture(){
	Texture* copy = copyImage();
	copy->textureImageView = copy->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
	return copy;
}

Texture* Texture::copyTexture(VkFormat format, VkImageLayout layout, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t mipLevels) {
	Texture* copy = copyImage(format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, usage, tiling, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT , mipLevels, texWidth, texHeight);
	copy->textureLayout = layout;
	copy->generateMipmaps();
	copy->textureImageView = copy->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
	return copy;
}

void imageTexture::createTextureImage(imageData* imgData) {

	texWidth = imgData->Width;
	texHeight = imgData->Height;
	texChannels = imgData->Channels;

	if (texChannels == 1) {
		textureFormat = VK_FORMAT_R8_UNORM; // We assume that images which have only one channel contain data not an image
	}

	VkDeviceSize imageSize = texWidth * texHeight * texChannels;

	if (!imgData->Bytes) {
		throw runtime_error("failed to load texture image!");
	}

	if (mipLevels == 0) {
		mipLevels = static_cast<uint32_t>(floor(log2(max(texWidth, texHeight)))) + 1;
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	Engine::get()->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(Engine::get()->device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, imgData->Bytes, static_cast<size_t>(imageSize));
	vkUnmapMemory(Engine::get()->device, stagingBufferMemory);

	createImage(VK_SAMPLE_COUNT_1_BIT, textureMemFlags);

	transitionImageLayout(textureImage, textureFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
	copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

	vkDestroyBuffer(Engine::get()->device, stagingBuffer, nullptr);
	vkFreeMemory(Engine::get()->device, stagingBufferMemory, nullptr);

	generateMipmaps();
}

void imageTexture::createTextureImageView() {
	textureImageView = createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
}

VkImageView Texture::createImageView(VkImageAspectFlags aspectFlags) {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = textureImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = textureFormat;

	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(Engine::get()->device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw runtime_error("failed to create texture image view!");
	}

	return imageView;
}

void Texture::createImage(VkSampleCountFlagBits numSamples, VkMemoryPropertyFlags properties) {
	
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = texWidth;
	imageInfo.extent.height = texHeight;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = textureFormat;
	imageInfo.tiling = textureTiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = textureUsage;

	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	imageInfo.samples = numSamples;

	if (vkCreateImage(Engine::get()->device, &imageInfo, nullptr, &textureImage) != VK_SUCCESS) {
		throw runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(Engine::get()->device, textureImage, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = Engine::get()->findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(Engine::get()->device, &allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS) {
		throw runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(Engine::get()->device, textureImage, textureImageMemory, 0);

	cleaned = false;
}

void Texture::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
	if (oldLayout == newLayout) {
		return;
	}

	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		if (hasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
		barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
		barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else {
		cout << oldLayout << " " << newLayout << endl;
		throw invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	Engine::get()->endSingleTimeCommands(commandBuffer);
}

void Texture::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0,0,0 };
	region.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	Engine::get()->endSingleTimeCommands(commandBuffer);
}

void Texture::generateMipmaps() {
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(Engine::get()->physicalDevice, textureFormat, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw runtime_error("texture image format does not support linear blitting!");
	}

	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = textureImage;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	for (uint32_t i = 1; i < mipLevels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0,0,0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0,0,0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer, textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit, VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = textureLayout;//VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = textureLayout;// VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	Engine::get()->endSingleTimeCommands(commandBuffer);
}

void Texture::destroyCVMat() {
	// Free the memory of the openCV matrix if it isn't needed later - saves memory by avoiding duplication
	texMat.release();
}

void Texture::getCVMat() { 
	// Retrieves an openCV Mat type image from the vulkan image of this texture struct
	// see https://github.com/SaschaWillems/Vulkan/blob/master/examples/screenshot/screenshot.cpp 

	bool supportsBlit = true;

	VkFormatProperties formatProps;

	vkGetPhysicalDeviceFormatProperties(Engine::get()->physicalDevice, textureFormat, &formatProps);
	if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
		std::cerr << "Device does not support blitting from optimal tiled images, using copy instead of blit!" << std::endl;
		supportsBlit = false;
	}

	vkGetPhysicalDeviceFormatProperties(Engine::get()->physicalDevice, textureFormat, &formatProps);
	if (!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
		std::cerr << "Device does not support blitting to linear tiled images, using copy instead of blit!" << std::endl;
		supportsBlit = false;
	}

	Texture* dst = copyImage(textureFormat, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TILING_LINEAR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1);

	VkImageSubresource subResource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
	VkSubresourceLayout subResourceLayout;
	vkGetImageSubresourceLayout(Engine::get()->device, dst->textureImage, &subResource, &subResourceLayout);

	const char* data;
	vkMapMemory(Engine::get()->device, dst->textureImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&data);
	data += subResourceLayout.offset;

	bool colorSwizzle = false;

	if (!supportsBlit)
	{
		std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
		colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), textureFormat) != formatsBGR.end());
	}

	texMat.create(texHeight, texWidth, CV_8UC3);

	for (uint32_t y = 0; y < texHeight; y++)
	{
		unsigned int* row = (unsigned int*)data;
		for (uint32_t x = 0; x < texWidth; x++)
		{
			if (!colorSwizzle)
			{
				texMat.at<Vec3b>(y, x) = Vec3b(*((char*)row+2), *((char*)row+1), *((char*)row));
			}
			else
			{
				texMat.at<Vec3b>(y, x) = Vec3b(*((char*)row), *((char*)row+1), *((char*)row+2));
			}
			row++;
		}
		data += subResourceLayout.rowPitch;
	}

	dst->cleanup();

}

void Texture::transitionMatToImg() {
	// Creates a Vulkan image using an opencv Matrix

	uchar* matData = new uchar[texMat.total() * 4];
	cv::Mat continuousRGBA(texMat.size(), CV_8UC4, matData);
	cv::cvtColor(texMat, continuousRGBA, cv::COLOR_BGR2RGBA, 4);

	texWidth = continuousRGBA.size().width;
	texHeight = continuousRGBA.size().height;
	texChannels = continuousRGBA.channels();

	VkDeviceSize imageSize = continuousRGBA.total() * continuousRGBA.elemSize();

	if (mipLevels == 0) {
		mipLevels = static_cast<uint32_t>(floor(log2(max(texWidth, texHeight)))) + 1;
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	Engine::get()->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(Engine::get()->device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, continuousRGBA.ptr(), static_cast<size_t>(imageSize));
	vkUnmapMemory(Engine::get()->device, stagingBufferMemory);

	createImage(VK_SAMPLE_COUNT_1_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	transitionImageLayout(textureImage, textureFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
	copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

	vkDestroyBuffer(Engine::get()->device, stagingBuffer, nullptr);
	vkFreeMemory(Engine::get()->device, stagingBufferMemory, nullptr);

	generateMipmaps();
}

void webcamTexture::createWebcamImage() {
	if (webCam == nullptr) {
		return;
	}
	webCam->getFrame();
	uchar* camData = new uchar[webCam->webcamFrame.total() * 4];
	cv::Mat continuousRGBA(webCam->webcamFrame.size(), CV_8UC4, camData);
	cv::cvtColor(webCam->webcamFrame, continuousRGBA, cv::COLOR_BGR2RGBA, 4);

	texWidth = continuousRGBA.size().width;
	texHeight = continuousRGBA.size().height;
	texChannels = continuousRGBA.channels();
	mipLevels = 1;

	VkDeviceSize imageSize = continuousRGBA.total() * continuousRGBA.elemSize();

	Engine::get()->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, textureBuffer, textureBufferMemory);

	vkMapMemory(Engine::get()->device, textureBufferMemory, 0, imageSize, 0, &tBuffer);
	memcpy(tBuffer, continuousRGBA.ptr(), static_cast<size_t>(imageSize));

	createImage(VK_SAMPLE_COUNT_1_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	transitionImageLayout(textureImage, textureFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
	copyBufferToImage(textureBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	generateMipmaps();
}

void webcamTexture::createWebcamTextureImageView() {
	textureImageView = createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
}

void webcamTexture::updateWebcam() {
	if (webCam == nullptr) {
		return;
	}
	webCam->getFrame();
	uchar* camData = new uchar[webCam->webcamFrame.total() * 4];
	cv::Mat continuousRGBA(webCam->webcamFrame.size(), CV_8UC4, camData);
	cv::cvtColor(webCam->webcamFrame, continuousRGBA, cv::COLOR_BGR2RGBA, 4);

	VkDeviceSize imageSize = continuousRGBA.total() * continuousRGBA.elemSize();

	memcpy(tBuffer, continuousRGBA.ptr(), (size_t)imageSize);

	delete[] camData;
	updateWebcamImage();

} 

void webcamTexture::updateWebcamImage() {

	transitionImageLayout(textureImage, textureFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
	copyBufferToImage(textureBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	generateMipmaps();

}