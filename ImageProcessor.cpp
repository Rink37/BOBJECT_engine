#include"ImageProcessor.h"

using namespace std;

// https://github.com/ttddee/ShaderDev/blob/master/src/vulkanrenderer.cpp

void filter::createDescriptorSetLayout() {
	switch (filtertype) {
	case OIOO:
		VkDescriptorPoolSize descPoolSize = {
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		2
		};
		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(1);
		poolInfo.pPoolSizes = &descPoolSize;
		poolInfo.maxSets = static_cast<uint32_t>(1);

		if (vkCreateDescriptorPool(Engine::get()->device, &poolInfo, nullptr, &descPool) != VK_SUCCESS) {
			throw runtime_error("failed to create descriptor pool!");
		}

		VkDescriptorSetLayoutBinding bindings[2] = {};

		bindings[0].binding = 0;
		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		bindings[0].descriptorCount = 1;
		bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		bindings[1].binding = 1;
		bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		bindings[1].descriptorCount = 1;
		bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.pBindings = bindings;
		descriptorSetLayoutCreateInfo.bindingCount = 2;

		if (vkCreateDescriptorSetLayout(Engine::get()->device, &descriptorSetLayoutCreateInfo, nullptr, &filterDescriptorSetLayout) != VK_SUCCESS) {
			throw runtime_error("Failed to create filter descriptor set layout");
		}
		break;
	}
}

void filter::createDescriptorSet() {

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

	VkDescriptorImageInfo destinationInfo = {};
	destinationInfo.imageView = filterTarget->textureImageView;
	destinationInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkDescriptorImageInfo sourceInfo = {};
	sourceInfo.imageView = source->textureImageView;
	sourceInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkWriteDescriptorSet descWrite[2] = {};

	descWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descWrite[0].dstSet = filterDescriptorSet;
	descWrite[0].dstBinding = 0;
	descWrite[0].descriptorCount = 1;
	descWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descWrite[0].pImageInfo = &sourceInfo;

	descWrite[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descWrite[1].dstSet = filterDescriptorSet;
	descWrite[1].dstBinding = 1;
	descWrite[1].descriptorCount = 1;
	descWrite[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descWrite[1].pImageInfo = &destinationInfo;

	vkUpdateDescriptorSets(Engine::get()->device, 2, descWrite, 0, nullptr);
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
	filterTarget = new Texture;
	filterTarget->textureFormat = VK_FORMAT_R8G8B8A8_UNORM;
	filterTarget->textureUsage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; // Transfer dst might cause issues? I'm not sure yet
	filterTarget->textureLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	filterTarget->texWidth = texWidth;
	filterTarget->texHeight = texHeight;
	filterTarget->mipLevels = 1;
	filterTarget->setup();
}

void filter::filterImage() {
	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeComputeCommand();

	vkQueueWaitIdle(Engine::get()->computeQueue);

	//source->transitionImageLayout(source->textureImage, source->textureFormat, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, source->mipLevels);
	filterTarget->transitionImageLayout(filterTarget->textureImage, filterTarget->textureFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, filterTarget->mipLevels);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, filterPipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, filterPipelineLayout, 0, 1, &filterDescriptorSet, 0, 0);
	vkCmdDispatch(commandBuffer, source->texWidth / 16, source->texHeight / 16, 1);

	Engine::get()->endSingleTimeComputeCommand(commandBuffer);

	filterTarget->transitionImageLayout(filterTarget->textureImage, filterTarget->textureFormat, VK_IMAGE_LAYOUT_GENERAL, filterTarget->textureLayout, filterTarget->mipLevels);

	filterTarget->getCVMat();

	cv::namedWindow("Output");
	while (true) {
		cv::imshow("Output", filterTarget->texMat);
		char c = (char)cv::waitKey(25); //Waits for us to press 'Esc', then exits
		if (c == 27) {
			cv::destroyWindow("Output");
			break;
		}
		if (cv::getWindowProperty("Output", cv::WND_PROP_VISIBLE) < 1) {
			break;
		}
	}
}


