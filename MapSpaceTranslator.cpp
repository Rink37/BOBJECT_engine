#include "GenerateNormalMap.h"
#include"include/ShaderDataType.h"
#include"include/NormalConvertor.h"

using namespace cv;
using namespace std;

void NormalGen::createOSImageFromMat(Mat srcImg) {

	// Used to specify the image of the object space normal map

	uchar* imgData = new uchar[srcImg.total() * 4];
	cv::Mat continuousRGBA(srcImg.size(), CV_8UC4, imgData);
	cv::cvtColor(srcImg, OSNormalMap, cv::COLOR_BGR2RGBA, 4);

	delete[] imgData;

	objectSpaceMap.width = OSNormalMap.size().width;
	objectSpaceMap.height = OSNormalMap.size().height;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	VkDeviceSize imageSize = OSNormalMap.total() * OSNormalMap.elemSize();

	Engine::get()->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(Engine::get()->device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, OSNormalMap.ptr(), static_cast<size_t>(imageSize));
	vkUnmapMemory(Engine::get()->device, stagingBufferMemory);

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageInfo.extent.width = objectSpaceMap.width;
	imageInfo.extent.height = objectSpaceMap.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(Engine::get()->device, &imageInfo, nullptr, &objectSpaceMap.colour.image) != VK_SUCCESS) {
		throw runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(Engine::get()->device, objectSpaceMap.colour.image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = Engine::get()->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(Engine::get()->device, &allocInfo, nullptr, &objectSpaceMap.colour.mem) != VK_SUCCESS) {
		throw runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(Engine::get()->device, objectSpaceMap.colour.image, objectSpaceMap.colour.mem, 0);

	VkCommandBuffer transitionCommandBuffer = Engine::get()->beginSingleTimeCommands();

	VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = objectSpaceMap.colour.image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

	vkCmdPipelineBarrier(
		transitionCommandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	Engine::get()->endSingleTimeCommands(transitionCommandBuffer);

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
		objectSpaceMap.width,
		objectSpaceMap.height,
		1
	};

	vkCmdCopyBufferToImage(
		commandBuffer,
		stagingBuffer,
		objectSpaceMap.colour.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	Engine::get()->endSingleTimeCommands(commandBuffer);

	VkCommandBuffer transition2CommandBuffer = Engine::get()->beginSingleTimeCommands();
	
	oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkImageMemoryBarrier barrier2{};
	barrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier2.oldLayout = oldLayout;
	barrier2.newLayout = newLayout;

	barrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier2.image = objectSpaceMap.colour.image;
	barrier2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	barrier2.subresourceRange.baseMipLevel = 0;
	barrier2.subresourceRange.levelCount = 1;
	barrier2.subresourceRange.baseArrayLayer = 0;
	barrier2.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage2;
	VkPipelineStageFlags destinationStage2;

	barrier2.srcAccessMask = 0;
	barrier2.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	sourceStage2 = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	destinationStage2 = VK_PIPELINE_STAGE_TRANSFER_BIT;

	vkCmdPipelineBarrier(
		transition2CommandBuffer,
		sourceStage2, destinationStage2,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier2
	);

	Engine::get()->endSingleTimeCommands(transition2CommandBuffer);

	vkDestroyBuffer(Engine::get()->device, stagingBuffer, nullptr);
	vkFreeMemory(Engine::get()->device, stagingBufferMemory, nullptr);

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = objectSpaceMap.colour.image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(Engine::get()->device, &viewInfo, nullptr, &objectSpaceMap.colour.view) != VK_SUCCESS) {
		throw runtime_error("failed to create texture image view!");
	}

}

void NormalGen::prepareTSMap() {
	tangentSpaceMap.width = objectSpaceMap.width;
	tangentSpaceMap.height = objectSpaceMap.height;

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = MAP_COLOUR_FORMAT;
	imageInfo.extent.width = tangentSpaceMap.width;
	imageInfo.extent.height = tangentSpaceMap.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	if (vkCreateImage(Engine::get()->device, &imageInfo, nullptr, &tangentSpaceMap.colour.image) != VK_SUCCESS) {
		throw runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(Engine::get()->device, tangentSpaceMap.colour.image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = Engine::get()->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(Engine::get()->device, &allocInfo, nullptr, &tangentSpaceMap.colour.mem) != VK_SUCCESS) {
		throw runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(Engine::get()->device, tangentSpaceMap.colour.image, tangentSpaceMap.colour.mem, 0);

	VkImageViewCreateInfo colorImageViewInfo = {};
	colorImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	colorImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	colorImageViewInfo.format = MAP_COLOUR_FORMAT;
	colorImageViewInfo.subresourceRange = {};
	colorImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	colorImageViewInfo.subresourceRange.baseMipLevel = 0;
	colorImageViewInfo.subresourceRange.levelCount = 1;
	colorImageViewInfo.subresourceRange.baseArrayLayer = 0;
	colorImageViewInfo.subresourceRange.layerCount = 1;
	colorImageViewInfo.image = tangentSpaceMap.colour.image;
	if (vkCreateImageView(Engine::get()->device, &colorImageViewInfo, nullptr, &tangentSpaceMap.colour.view) != VK_SUCCESS) {
		throw runtime_error("failed to create texture image view!");
	}

	array<VkAttachmentDescription, 1> attachmentDescriptions = {};
	attachmentDescriptions[0].format = MAP_COLOUR_FORMAT;
	attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
	renderPassInfo.pAttachments = attachmentDescriptions.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	if (vkCreateRenderPass(Engine::get()->device, &renderPassInfo, nullptr, &tangentSpaceMap.renderPass) != VK_SUCCESS) {
		throw runtime_error("Failed to create render pass");
	}

	VkFramebufferCreateInfo fbufCreateInfo = {};
	fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbufCreateInfo.renderPass = tangentSpaceMap.renderPass;
	fbufCreateInfo.attachmentCount = 1;
	fbufCreateInfo.pAttachments = &tangentSpaceMap.colour.view;
	fbufCreateInfo.width = tangentSpaceMap.width;
	fbufCreateInfo.height = tangentSpaceMap.height;
	fbufCreateInfo.layers = 1;

	if (vkCreateFramebuffer(Engine::get()->device, &fbufCreateInfo, nullptr, &tangentSpaceMap.frameBuffer) != VK_SUCCESS) {
		throw runtime_error("Failed to create framebuffer");
	}
}

void NormalGen::createTSPipeline() {
	shaderData* sD = new NORMALCONVERTORSHADER;

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getCompleteAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_TRUE;
	multisampling.rasterizationSamples = msaaSamples;
	multisampling.minSampleShading = .2f;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &tangentSpaceMap.descriptorSetLayout;

	if (vkCreatePipelineLayout(Engine::get()->device, &pipelineLayoutInfo, nullptr, &TSpipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	auto VertShaderCode = sD->vertData;
	auto FragShaderCode = sD->fragData;

	VkShaderModule VertShaderModule = Engine::get()->createShaderModule(VertShaderCode);
	VkShaderModule FragShaderModule = Engine::get()->createShaderModule(FragShaderCode);

	VkPipelineShaderStageCreateInfo VertShaderStageInfo{};
	VertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	VertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	VertShaderStageInfo.module = VertShaderModule;
	VertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo FragShaderStageInfo{};
	FragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	FragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	FragShaderStageInfo.module = FragShaderModule;
	FragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo, FragShaderStageInfo };

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = ShaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = TSpipelineLayout;
	pipelineInfo.renderPass = tangentSpaceMap.renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(Engine::get()->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &TSpipeline) != VK_SUCCESS) {
		throw runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(Engine::get()->device, FragShaderModule, nullptr);
	vkDestroyShaderModule(Engine::get()->device, VertShaderModule, nullptr);
}

void NormalGen::prepareTSDescriptor() {

	VkDescriptorSetLayoutBinding samplerLayoutBinding{}; 
	samplerLayoutBinding.binding = 0;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &samplerLayoutBinding;

	if (vkCreateDescriptorSetLayout(Engine::get()->device, &layoutInfo, nullptr, &tangentSpaceMap.descriptorSetLayout) != VK_SUCCESS) {
		throw runtime_error("failed to create descriptor set layout!");
	}

	VkDescriptorPoolSize poolSize;
	
	poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize.descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = 1;

	if (vkCreateDescriptorPool(Engine::get()->device, &poolInfo, nullptr, &tangentSpaceMap.descriptorPool) != VK_SUCCESS) {
		throw runtime_error("failed to create descriptor pool!");
	}
	
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = tangentSpaceMap.descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &tangentSpaceMap.descriptorSetLayout;

	if (vkAllocateDescriptorSets(Engine::get()->device, &allocInfo, &tangentSpaceMap.descriptorSet) != VK_SUCCESS) {
		throw runtime_error("failed to allocate descriptor sets!");
	}

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = objectSpaceMap.colour.view;
	imageInfo.sampler = Engine::get()->textureSampler;

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = tangentSpaceMap.descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(Engine::get()->device, 1, &descriptorWrite, 0, nullptr);
}

VkCommandBuffer NormalGen::convertOStoTS(VkCommandBuffer commandbuffer, Mesh* mesh) {

	VkClearValue clearValues[1] = {};
	clearValues[0].color = { {0.5f, 0.5f, 1.0f, 1.0f} };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = tangentSpaceMap.renderPass;
	renderPassBeginInfo.framebuffer = tangentSpaceMap.frameBuffer;
	renderPassBeginInfo.renderArea.extent.width = tangentSpaceMap.width;
	renderPassBeginInfo.renderArea.extent.height = tangentSpaceMap.height;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(commandbuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(tangentSpaceMap.width);
	viewport.height = static_cast<float>(tangentSpaceMap.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandbuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = { tangentSpaceMap.width, tangentSpaceMap.height };
	vkCmdSetScissor(commandbuffer, 0, 1, &scissor);

	vkCmdBindPipeline(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, TSpipeline);

	VkBuffer vertexBuffers[] = { mesh->vertexBuffer };
	VkDeviceSize offsets[] = { 0 };

	mesh->createTexCoordIndexBuffer();

	vkCmdBindVertexBuffers(commandbuffer, 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(commandbuffer, mesh->texCoordIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, TSpipelineLayout, 0, 1, &tangentSpaceMap.descriptorSet, 0, nullptr);

	vkCmdDrawIndexed(commandbuffer, static_cast<uint32_t>(mesh->uniqueTexindices.size()), 1, 0, 0, 0);

	vkCmdEndRenderPass(commandbuffer);

	return commandbuffer;
}

void NormalGen::cleanupTS() {
	vkDeviceWaitIdle(Engine::get()->device);

	vkDestroyPipeline(Engine::get()->device, TSpipeline, nullptr);
	vkDestroyPipelineLayout(Engine::get()->device, TSpipelineLayout, nullptr);

	vkDestroyRenderPass(Engine::get()->device, tangentSpaceMap.renderPass, nullptr);
	vkDestroyFramebuffer(Engine::get()->device, tangentSpaceMap.frameBuffer, nullptr);

	vkFreeMemory(Engine::get()->device, tangentSpaceMap.colour.mem, nullptr);
	vkDestroyImageView(Engine::get()->device, tangentSpaceMap.colour.view, nullptr);
	vkDestroyImage(Engine::get()->device, tangentSpaceMap.colour.image, nullptr);

	vkFreeMemory(Engine::get()->device, objectSpaceMap.colour.mem, nullptr);
	vkDestroyImageView(Engine::get()->device, objectSpaceMap.colour.view, nullptr);
	vkDestroyImage(Engine::get()->device, objectSpaceMap.colour.image, nullptr);
}