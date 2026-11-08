#include"ImageProcessor.h"

using namespace std;

// https://github.com/ttddee/ShaderDev/blob/master/src/vulkanrenderer.cpp

void filter::getFilterLayout(shaderData* sD) {
	
	IObindings = sD->IO;
	
	if (IObindings.size() == 0) {
		throw runtime_error("The specified filter shader does not contain I/O information, so we cannot construct descriptor sets");
	}

	int inputCount = 0;
	int outputCount = 0;
	int inputImageCount = 0;
	
	for (size_t i = 0; i != IObindings.size(); i++) {
		if (IObindings[i].direction) {
			inputCount++;
			if (IObindings[i].type == 0) {
				inputImageCount++;
			}
		}
		else {
			outputCount++;
			if (IObindings[i].type == 0) {
				outputImageCount++;
			}
		}
	}

	if (source.size() < inputImageCount) {
		throw runtime_error("The filter has not been given enough input images to perform filtering");
	}
}

void filter::createDescriptorSetLayout() {
	int descriptorElementCount = static_cast<int>(IObindings.size());

	unsigned int descriptorImageCount = 0;
	unsigned int descriptorBufferCount = 0;

	for (size_t i = 0; i != IObindings.size(); i++) {
		if (IObindings[i].type == 0) {
			descriptorImageCount++;
		}
		else if (IObindings[i].type == 1) {
			descriptorBufferCount++;
		}
	}

	std::vector<VkDescriptorPoolSize> descPoolSizes{};
	if (descriptorImageCount > 0) {
		descPoolSizes.push_back(VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, descriptorImageCount});
	}
	if (descriptorBufferCount > 0) {
		descPoolSizes.push_back(VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptorBufferCount });
	}

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(descPoolSizes.size());
	poolInfo.pPoolSizes = descPoolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(1);

	if (vkCreateDescriptorPool(Engine::get()->device, &poolInfo, nullptr, &descPool) != VK_SUCCESS) {
		throw runtime_error("Failed to create descriptor pool!");
	}

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

	std::vector<VkDescriptorSetLayoutBinding> bindings;
	int index = 0;
	for (size_t i = 0; i != IObindings.size(); i++) {
		VkDescriptorSetLayoutBinding bindingElement{};
		bindingElement.binding = index;
		if (IObindings[i].type == 0) {
			bindingElement.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		}
		else if (IObindings[i].type == 1) {
			bindingElement.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		}
		bindingElement.descriptorCount = 1;
		bindingElement.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		bindings.push_back(bindingElement);
		index++;
	}

	descriptorSetLayoutCreateInfo.pBindings = bindings.data();
	descriptorSetLayoutCreateInfo.bindingCount = index;

	if (vkCreateDescriptorSetLayout(Engine::get()->device, &descriptorSetLayoutCreateInfo, nullptr, &filterDescriptorSetLayout) != VK_SUCCESS) {
		throw runtime_error("Failed to create filter descriptor set layout!");
	}
}

void filter::createDescriptorSet() {
	int descriptorElementCount = static_cast<int>(IObindings.size());

	VkDescriptorSetAllocateInfo descSetAllocInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		nullptr,
		descPool,
		1,
		&filterDescriptorSetLayout
	};

	if (vkAllocateDescriptorSets(Engine::get()->device, &descSetAllocInfo, &filterDescriptorSet) != VK_SUCCESS) {
		throw runtime_error("Failed to allocate filter descriptor");
	}

	std::vector<VkWriteDescriptorSet> descWrite = {};
	std::vector<VkDescriptorImageInfo*> imageInfos = {};
	std::vector<VkDescriptorBufferInfo*> bufferInfos = {};
	int index = 0;
	int inputImageIndex = 0;
	int outputImageIndex = 0;
	for (size_t i = 0; i != IObindings.size(); i++) {
		VkWriteDescriptorSet writeElement = {};
		writeElement.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeElement.dstSet = filterDescriptorSet;
		writeElement.dstBinding = index;
		writeElement.descriptorCount = 1;
		if (IObindings[i].type == 0) {
			VkDescriptorImageInfo* imageInfo = new VkDescriptorImageInfo;
			if (IObindings[i].direction) {
				imageInfo->imageView = source.at(inputImageIndex)->textureImageView;
				inputImageIndex++;
			}
			else {
				imageInfo->imageView = filterTarget.at(outputImageIndex)->textureImageView;
				outputImageIndex++;
			}
			imageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;

			imageInfos.push_back(imageInfo);

			writeElement.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			writeElement.pImageInfo = imageInfos[imageInfos.size() - 1];
		}
		else if (IObindings[i].type == 1) {
			if (!IObindings[i].direction) {
				throw runtime_error("IO map claims uniform buffer as output, which is an invalid specification.");
			}

			VkDescriptorBufferInfo* bufferInfo = new VkDescriptorBufferInfo;
			bufferInfo->buffer = bufferRef;
			bufferInfo->offset = 0;
			bufferInfo->range = bufferSize;

			bufferInfos.push_back(bufferInfo);

			writeElement.dstArrayElement = 0;
			writeElement.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeElement.pBufferInfo = bufferInfos[bufferInfos.size() - 1];
		}
		else {
			throw runtime_error("Invalid binding type");
		}
		descWrite.push_back(writeElement);
		index++;
	}
	vkUpdateDescriptorSets(Engine::get()->device, descriptorElementCount, descWrite.data(), 0, nullptr);

	for (size_t i = 0; i != imageInfos.size(); i++) {
		delete imageInfos[i];
	}

	for (size_t i = 0; i != bufferInfos.size(); i++) {
		delete bufferInfos[i];
	}
}

void filter::updateDescriptorSet() {
	int descriptorElementCount = static_cast<int>(IObindings.size());
	
	std::vector<VkWriteDescriptorSet> descWrite = {};
	std::vector<VkDescriptorImageInfo*> imageInfos = {};
	std::vector<VkDescriptorBufferInfo*> bufferInfos = {};
	int index = 0;
	int inputImageIndex = 0;
	int outputImageIndex = 0;
	for (size_t i = 0; i != IObindings.size(); i++) {
		VkWriteDescriptorSet writeElement = {};
		writeElement.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeElement.dstSet = filterDescriptorSet;
		writeElement.dstBinding = index;
		writeElement.descriptorCount = 1;
		if (IObindings[i].type == 0) {
			VkDescriptorImageInfo* imageInfo = new VkDescriptorImageInfo;
			if (IObindings[i].direction) {
				imageInfo->imageView = source.at(inputImageIndex)->textureImageView;
				inputImageIndex++;
			}
			else {
				imageInfo->imageView = filterTarget.at(outputImageIndex)->textureImageView;
				outputImageIndex++;
			}
			imageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;

			imageInfos.push_back(imageInfo);

			writeElement.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			writeElement.pImageInfo = imageInfos[imageInfos.size() - 1];
		}
		else if (IObindings[i].type == 1) {
			if (!IObindings[i].direction) {
				throw runtime_error("IO map claims uniform buffer as output, which is an invalid specification.");
			}

			VkDescriptorBufferInfo* bufferInfo = new VkDescriptorBufferInfo;
			bufferInfo->buffer = bufferRef;
			bufferInfo->offset = 0;
			bufferInfo->range = bufferSize;

			bufferInfos.push_back(bufferInfo);

			writeElement.dstArrayElement = 0;
			writeElement.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeElement.pBufferInfo = bufferInfos[bufferInfos.size() - 1];
		}
		else {
			throw runtime_error("Invalid binding type");
		}
		descWrite.push_back(writeElement);
		index++;
	}
	vkUpdateDescriptorSets(Engine::get()->device, descriptorElementCount, descWrite.data(), 0, nullptr);

	for (size_t i = 0; i != imageInfos.size(); i++) {
		delete imageInfos[i];
	}

	for (size_t i = 0; i != bufferInfos.size(); i++) {
		delete bufferInfos[i];
	}
}

void filter::createFilterPipelineLayout() {

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &filterDescriptorSetLayout;

	if (vkCreatePipelineLayout(Engine::get()->device, &pipelineLayoutInfo, nullptr, &filterPipelineLayout) != VK_SUCCESS) {
		throw runtime_error("Failed to create filter pipeline layout");
	}
}

void filter::createFilterPipeline() {
	
	VkPipelineShaderStageCreateInfo filterStage = {
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		nullptr,
		0,
		VK_SHADER_STAGE_COMPUTE_BIT,
		filterShaderModule,
		"main",
		nullptr
	};

	VkComputePipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = filterStage;
	pipelineInfo.layout = filterPipelineLayout;

	if (vkCreateComputePipelines(Engine::get()->device, nullptr, 1, &pipelineInfo, nullptr, &filterPipeline) != VK_SUCCESS) {
		throw runtime_error("Failed to create filter pipeline");
	}

	vkDestroyShaderModule(Engine::get()->device, filterShaderModule, nullptr);
}

void filter::createFilterTarget() {
	filterTarget.push_back(new Texture);
	uint32_t index = static_cast<uint32_t>(filterTarget.size()) - 1;
	filterTarget[index]->textureFormat = targetFormat;
	filterTarget[index]->textureUsage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; // Transfer dst might cause issues? I'm not sure yet
	filterTarget[index]->textureLayout = VK_IMAGE_LAYOUT_GENERAL;
	filterTarget[index]->texWidth = texWidth;
	filterTarget[index]->texHeight = texHeight;
	filterTarget[index]->mipLevels = 1;
	filterTarget[index]->setup();

	filterTarget[index]->transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
}

void filter::filterImage() {
	vkDeviceWaitIdle(Engine::get()->device);

	for (Texture* tex : source) {
		tex->transitionImageLayout(tex->textureLayout, VK_IMAGE_LAYOUT_GENERAL);
	}

	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeComputeCommand();

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, filterPipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, filterPipelineLayout, 0, 1, &filterDescriptorSet, 0, 0);
	vkCmdDispatch(commandBuffer, source[0]->texWidth / 16 + 1, source[0]->texHeight / 16 + 1, 1);

	Engine::get()->endSingleTimeComputeCommand(commandBuffer);

	for (Texture* tex : source) {
		tex->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, tex->textureLayout);
	}
}

void filter::filterImage(VkCommandBuffer commandBuffer) {
	
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, filterPipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, filterPipelineLayout, 0, 1, &filterDescriptorSet, 0, 0);
	vkCmdDispatch(commandBuffer, source[0]->texWidth / 16 + 1, source[0]->texHeight / 16 + 1, 1);
	
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	if (filterTarget.size() > 0) {
		barrier.image = filterTarget[0]->textureImage;
	}
	else {
		barrier.image = source[0]->textureImage;
	}
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	if (filterTarget.size() > 0) {
		barrier.subresourceRange.levelCount = filterTarget[0]->mipLevels;
	}
	else {
		barrier.subresourceRange.levelCount = source[0]->mipLevels;
	}
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
}

int countInputs(std::vector<shaderIOValue> IOValues) {
	int count = 0;
	for (size_t i = 0; i != IOValues.size(); i++) {
		if (IOValues[i].direction) {
			count++;
		}
	}
	return count;
}

void postProcessFilter::setup(shaderData* sd, drawImage* target) {
	std::vector<drawImage*> drawImgs{};
	drawImgs.push_back(target);
	setup(sd, drawImgs);
}

void postProcessFilter::setup(shaderData* sd, std::vector<drawImage*> drawImgs) {
	for (size_t i = 0; i != drawImgs.size(); i++) {
		targets.push_back(drawImgs[i]);
	}
	uint32_t imageCount = static_cast<uint32_t>(targets[0]->images.size());
	for (uint32_t i = 0; i != imageCount; i++) {
		std::vector<Texture*> srcs{};
		for (size_t j = 0; j != targets.size(); j++) {
			Texture* testTex = new Texture;
			testTex->textureImage = targets[j]->images[i];
			testTex->textureImageMemory = targets[j]->imageMemory[i];
			testTex->textureImageView = targets[j]->imageViews[i];
			testTex->mipLevels = 1;
			testTex->texWidth = targets[j]->imageExtent.width;
			testTex->texHeight = targets[j]->imageExtent.height;
			constructedTextures.push_back(testTex);
			srcs.push_back(testTex);
		}
		filters.push_back(filter(srcs, sd));
		filters[i].filterTarget[0]->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	}
}

void postProcessFilter::setup(shaderData* sd, std::vector<Texture*> textures){
	uint32_t imageCount = static_cast<uint32_t>(textures.size());
	for (uint32_t i = 0; i != imageCount; i++) {
		filters.push_back(filter(std::vector<Texture*>{textures[i]}, sd));
		filters[i].filterTarget[0]->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	}
}

void postProcessFilter::setup(shaderData* sd, drawImage* target, VkBuffer uniformBuffer, uint32_t bufferSize) {
	std::vector<drawImage*> drawImgs{};
	drawImgs.push_back(target);
	setup(sd, drawImgs, uniformBuffer, bufferSize);
}

void postProcessFilter::setup(shaderData* sd, std::vector<drawImage*> drawImgs, VkBuffer uniformBuffer, uint32_t bufferSize) {
	for (size_t i = 0; i != drawImgs.size(); i++) {
		targets.push_back(drawImgs[i]);
	}
	uint32_t imageCount = static_cast<uint32_t>(targets[0]->images.size());
	for (uint32_t i = 0; i != imageCount; i++) {
		std::vector<Texture*> srcs{};
		for (size_t j = 0; j != targets.size(); j++) {
			Texture* testTex = new Texture;
			testTex->textureImage = targets[j]->images[i];
			testTex->textureImageMemory = targets[j]->imageMemory[i];
			testTex->textureImageView = targets[j]->imageViews[i];
			testTex->mipLevels = 1;
			testTex->texWidth = targets[j]->imageExtent.width;
			testTex->texHeight = targets[j]->imageExtent.height;
			constructedTextures.push_back(testTex);
			srcs.push_back(testTex);
		}
		filters.push_back(filter(srcs, sd, uniformBuffer, bufferSize));
		filters[i].filterTarget[0]->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	}
}

void postProcessFilter::setup(shaderData* sd, std::vector<Texture*> textures, VkBuffer uniformBuffer, uint32_t bufferSize) {
	uint32_t imageCount = static_cast<uint32_t>(textures.size());
	for (uint32_t i = 0; i != imageCount; i++) {
		filters.push_back(filter(std::vector<Texture*>{textures[i]}, sd, uniformBuffer, bufferSize));
		filters[i].filterTarget[0]->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	}
}

void postProcessFilter::filterImage(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
	filters[imageIndex].source[0]->transitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
	filters[imageIndex].filterTarget[0] ->transitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
	filters[imageIndex].filterImage(commandBuffer);
	filters[imageIndex].source[0]->transitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	filters[imageIndex].filterTarget[0]->transitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
}

VkImage postProcessFilter::getFilterResult(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
	return filters[imageIndex].filterTarget[0]->textureImage;
}

void postProcessFilter::recreateDescriptorSets() {

	uint32_t imageCount = static_cast<uint32_t>(filters.size());

	for (size_t i = 0; i < imageCount; i++) {
		filters[i].updateDescriptorSet();
	}
}

void postProcessFilter::cleanup() {
	for (size_t i = 0; i != filters.size(); i++) {
		filters[i].cleanup();
	}
	filters.clear();
	constructedTextures.clear();
	targets.clear();
}

std::vector<Texture*> postProcessFilter::getRenderTargets() {
	std::vector<Texture*> renderTargets{};
	for (size_t i = 0; i != filters.size(); i++) {
		renderTargets.push_back(filters[i].filterTarget[0]);
	}
	return renderTargets;
}

void textureWriter::prepMap() {
	map->colour = new Texture;

	VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_2_BIT;

	map->colour->texWidth = refTex->texWidth;
	map->colour->texHeight = refTex->texHeight;
	map->colour->texChannels = 4;
	map->colour->mipLevels = 1;
	map->colour->textureFormat = refTex->textureFormat;
	map->colour->textureLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	map->colour->textureUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	map->colour->createImage(sampleCount, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	map->colour->textureImageView = map->colour->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = refTex->textureFormat;
	colorAttachment.samples = sampleCount;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.dependencyCount = 1;

	VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorReference;

	std::vector<VkAttachmentDescription> attachmentDescriptions{};
	attachmentDescriptions.push_back(colorAttachment);

	VkAttachmentDescription depthAttachment{};
	VkAttachmentDescription colourAttachmentResolve{};
	VkAttachmentReference depthAttachmentRef{};
	VkAttachmentReference colourAttachmentResolveRef{};

	if (depthTested) {
		if (attachment == nullptr) {
			attachment = new Texture;

			attachment->texWidth = refTex->texWidth;
			attachment->texHeight = refTex->texHeight;
			attachment->texChannels = 4;
			attachment->mipLevels = 1;
			attachment->textureFormat = refTex->textureFormat;
			attachment->textureLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachment->textureUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

			attachment->createImage(VK_SAMPLE_COUNT_1_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			attachment->textureImageView = attachment->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
		}
		if (depth == nullptr) {
			VkFormat depthFormat = Engine::get()->findDepthFormat();
			depth = new Texture;

			depth->texWidth = refTex->texWidth;
			depth->texHeight = refTex->texHeight;
			depth->texChannels = 1;
			depth->mipLevels = 1;
			depth->textureFormat = depthFormat;
			depth->textureLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			depth->textureUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			
			depth->createImage(sampleCount, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			depth->textureImageView = depth->createImageView(VK_IMAGE_ASPECT_DEPTH_BIT);
		}
		depthAttachment.format = Engine::get()->findDepthFormat();
		depthAttachment.samples = sampleCount;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		colourAttachmentResolve.format = refTex->textureFormat;
		colourAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colourAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colourAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colourAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colourAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colourAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colourAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		colourAttachmentResolveRef.attachment = 2;
		colourAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		subpass.pDepthStencilAttachment = &depthAttachmentRef;
		subpass.pResolveAttachments = &colourAttachmentResolveRef;

		attachmentDescriptions.push_back(depthAttachment);
		attachmentDescriptions.push_back(colourAttachmentResolve);
	}
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.pDependencies = &dependency;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
	renderPassInfo.pAttachments = attachmentDescriptions.data();

	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	if (vkCreateRenderPass(Engine::get()->device, &renderPassInfo, nullptr, &map->renderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass");
	}

	std::vector<VkImageView> attachments{};
	attachments.push_back(map->colour->textureImageView);
	if (depthTested) {
		attachments.push_back(depth->textureImageView);
		attachments.push_back(attachment->textureImageView);
	}

	VkFramebufferCreateInfo fbufCreateInfo = {};
	fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbufCreateInfo.renderPass = map->renderPass;
	fbufCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	fbufCreateInfo.pAttachments = attachments.data();
	fbufCreateInfo.width = map->colour->texWidth;
	fbufCreateInfo.height = map->colour->texHeight;
	fbufCreateInfo.layers = 1;

	if (vkCreateFramebuffer(Engine::get()->device, &fbufCreateInfo, nullptr, &map->frameBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create framebuffer");
	}

	map->cleaned = false;
}

void textureWriter::prepareDrawDescriptor() {
	map->drawMat = new Material(std::vector<Texture*>{refTex}, std::vector<std::pair<std::vector<VkBuffer>, uint32_t>>{}, sD);
	map->cleaned = false;
}

void textureWriter::prepareDrawDescriptor(std::vector<Texture*> textures, std::vector<std::pair<std::vector<VkBuffer>, uint32_t>> buffers){
	map->drawMat = new Material(textures, buffers, sD);
	map->cleaned = false;
}

void textureWriter::createTexWritePipeline() {

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	if (fullAttributes) {
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(Vertex::getCompleteAttributeDescriptions().size());
		vertexInputInfo.pVertexAttributeDescriptions = Vertex::getCompleteAttributeDescriptions().data();
	}
	else {
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(Vertex::getAttributeDescriptions().size());
		vertexInputInfo.pVertexAttributeDescriptions = Vertex::getAttributeDescriptions().data();
	}

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_2_BIT;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = sampleCount;
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
	if (map->drawMat != nullptr && map->drawMat->descriptorSetLayout != nullptr) {
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &map->drawMat->descriptorSetLayout;
	}
	else {
		pipelineLayoutInfo.setLayoutCount = 0;
	}

	if (vkCreatePipelineLayout(Engine::get()->device, &pipelineLayoutInfo, nullptr, &map->pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	auto VertShaderCode = *(sD->vertData);
	auto FragShaderCode = *(sD->fragData);

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
	rasterizer.cullMode = VK_CULL_MODE_NONE;
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
	pipelineInfo.layout = map->pipelineLayout;
	pipelineInfo.renderPass = map->renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	if (depthTested) {
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f;
		depthStencil.maxDepthBounds = 1.0f;

		pipelineInfo.pDepthStencilState = &depthStencil;
	}

	if (vkCreateGraphicsPipelines(Engine::get()->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &map->pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(Engine::get()->device, FragShaderModule, nullptr);
	vkDestroyShaderModule(Engine::get()->device, VertShaderModule, nullptr);

	delete sD;

	map->cleaned = false;
}

VkCommandBuffer textureWriter::drawMap(VkCommandBuffer commandbuffer, std::vector<Mesh*> meshes) {

	std::vector<VkClearValue> clearValues{};
	clearValues.push_back(VkClearValue{ {clearColour.x, clearColour.y, clearColour.z, clearColour.w} });
	if (depthTested) {
		clearValues.push_back(VkClearValue{ {1.0f, 0.0f} });
	}

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = map->renderPass;
	renderPassBeginInfo.framebuffer = map->frameBuffer;
	renderPassBeginInfo.renderArea.extent.width = map->colour->texWidth;
	renderPassBeginInfo.renderArea.extent.height = map->colour->texHeight;
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandbuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(map->colour->texWidth);
	viewport.height = static_cast<float>(map->colour->texHeight);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandbuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = { map->colour->texWidth, map->colour->texHeight };
	vkCmdSetScissor(commandbuffer, 0, 1, &scissor);

	vkCmdBindPipeline(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, map->pipeline);

	for (Mesh* mesh : meshes) {
		VkBuffer vertexBuffers[] = { mesh->vertexBuffer };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(commandbuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandbuffer, mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		if (map->drawMat != nullptr && map->drawMat->descriptorSets.size() > 0 && map->drawMat->descriptorSets[0] != nullptr) {
			vkCmdBindDescriptorSets(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, map->pipelineLayout, 0, 1, &map->drawMat->descriptorSets[0], 0, nullptr);
		}

		vkCmdDrawIndexed(commandbuffer, static_cast<uint32_t>(mesh->indices.size()), 1, 0, 0, 0);
	}

	vkCmdEndRenderPass(commandbuffer);

	return commandbuffer;
};