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
	int descriptorElementCount = IObindings.size();

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
	int descriptorElementCount = IObindings.size();

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
	int descriptorElementCount = IObindings.size();
	
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
	uint32_t index = filterTarget.size() - 1;
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
	vkQueueWaitIdle(Engine::get()->computeQueue);

	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeComputeCommand();

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, filterPipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, filterPipelineLayout, 0, 1, &filterDescriptorSet, 0, 0);
	vkCmdDispatch(commandBuffer, source[0]->texWidth / 16, source[0]->texHeight / 16, 1);

	Engine::get()->endSingleTimeComputeCommand(commandBuffer);
}

void filter::filterImage(VkCommandBuffer commandBuffer) {
	
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, filterPipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, filterPipelineLayout, 0, 1, &filterDescriptorSet, 0, 0);
	vkCmdDispatch(commandBuffer, source[0]->texWidth / 16, source[0]->texHeight / 16, 1);
	
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
	uint32_t imageCount = targets[0]->images.size();
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
	uint32_t imageCount = textures.size();
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
	uint32_t imageCount = targets[0]->images.size();
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
	uint32_t imageCount = textures.size();
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

	uint32_t imageCount = filters.size();

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