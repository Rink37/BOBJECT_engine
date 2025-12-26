#include"ImageProcessor.h"

using namespace std;

// https://github.com/ttddee/ShaderDev/blob/master/src/vulkanrenderer.cpp

void filter::createDescriptorSetLayout() {

	if (filtertype == OIOO) {
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

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (!hasUniformBuffer) {
			VkDescriptorSetLayoutBinding bindings[2] = {};

			bindings[0].binding = 0;
			bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			bindings[0].descriptorCount = 1;
			bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			bindings[1].binding = 1;
			bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			bindings[1].descriptorCount = 1;
			bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			descriptorSetLayoutCreateInfo.pBindings = bindings;
			descriptorSetLayoutCreateInfo.bindingCount = 2;

			if (vkCreateDescriptorSetLayout(Engine::get()->device, &descriptorSetLayoutCreateInfo, nullptr, &filterDescriptorSetLayout) != VK_SUCCESS) {
				throw runtime_error("Failed to create filter descriptor set layout");
			}
		}
		else {
			VkDescriptorSetLayoutBinding bindings[3] = {};

			bindings[0].binding = 0;
			bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			bindings[0].descriptorCount = 1;
			bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			bindings[1].binding = 1;
			bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			bindings[1].descriptorCount = 1;
			bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			bindings[2].binding = 2;
			bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bindings[2].descriptorCount = 1;
			bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			descriptorSetLayoutCreateInfo.pBindings = bindings;
			descriptorSetLayoutCreateInfo.bindingCount = 3;

			if (vkCreateDescriptorSetLayout(Engine::get()->device, &descriptorSetLayoutCreateInfo, nullptr, &filterDescriptorSetLayout) != VK_SUCCESS) {
				throw runtime_error("Failed to create filter descriptor set layout");
			}
		}
		
	} else if (filtertype == TIOO){
		VkDescriptorPoolSize descPoolSize = {
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		3
		};
		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(1);
		poolInfo.pPoolSizes = &descPoolSize;
		poolInfo.maxSets = static_cast<uint32_t>(1);

		if (vkCreateDescriptorPool(Engine::get()->device, &poolInfo, nullptr, &descPool) != VK_SUCCESS) {
			throw runtime_error("failed to create descriptor pool!");
		}

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (!hasUniformBuffer) {
			VkDescriptorSetLayoutBinding bindings[3] = {};

			bindings[0].binding = 0;
			bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			bindings[0].descriptorCount = 1;
			bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			bindings[1].binding = 1;
			bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			bindings[1].descriptorCount = 1;
			bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			bindings[2].binding = 2;
			bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			bindings[2].descriptorCount = 1;
			bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;


			descriptorSetLayoutCreateInfo.pBindings = bindings;
			descriptorSetLayoutCreateInfo.bindingCount = 3;

			if (vkCreateDescriptorSetLayout(Engine::get()->device, &descriptorSetLayoutCreateInfo, nullptr, &filterDescriptorSetLayout) != VK_SUCCESS) {
				throw runtime_error("Failed to create filter descriptor set layout");
			}
		}
		else {
			VkDescriptorSetLayoutBinding bindings[4] = {};

			bindings[0].binding = 0;
			bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			bindings[0].descriptorCount = 1;
			bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			bindings[1].binding = 1;
			bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			bindings[1].descriptorCount = 1;
			bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			bindings[2].binding = 2;
			bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			bindings[2].descriptorCount = 1;
			bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			bindings[3].binding = 3;
			bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bindings[3].descriptorCount = 1;
			bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			descriptorSetLayoutCreateInfo.pBindings = bindings;
			descriptorSetLayoutCreateInfo.bindingCount = 4;

			if (vkCreateDescriptorSetLayout(Engine::get()->device, &descriptorSetLayoutCreateInfo, nullptr, &filterDescriptorSetLayout) != VK_SUCCESS) {
				throw runtime_error("Failed to create filter descriptor set layout");
			}
		}
		
	}
	else if (filtertype == THIOO) {
		VkDescriptorPoolSize descPoolSize = {
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		4
		};
		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(1);
		poolInfo.pPoolSizes = &descPoolSize;
		poolInfo.maxSets = static_cast<uint32_t>(1);

		if (vkCreateDescriptorPool(Engine::get()->device, &poolInfo, nullptr, &descPool) != VK_SUCCESS) {
			throw runtime_error("failed to create descriptor pool!");
		}

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (!hasUniformBuffer) {
			VkDescriptorSetLayoutBinding bindings[4] = {};

			bindings[0].binding = 0;
			bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			bindings[0].descriptorCount = 1;
			bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			bindings[1].binding = 1;
			bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			bindings[1].descriptorCount = 1;
			bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			bindings[2].binding = 2;
			bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			bindings[2].descriptorCount = 1;
			bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			bindings[3].binding = 3;
			bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			bindings[3].descriptorCount = 1;
			bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			descriptorSetLayoutCreateInfo.pBindings = bindings;
			descriptorSetLayoutCreateInfo.bindingCount = 4;

			if (vkCreateDescriptorSetLayout(Engine::get()->device, &descriptorSetLayoutCreateInfo, nullptr, &filterDescriptorSetLayout) != VK_SUCCESS) {
				throw runtime_error("Failed to create filter descriptor set layout");
			}
		}
		else {
			VkDescriptorSetLayoutBinding bindings[5] = {};

			bindings[0].binding = 0;
			bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			bindings[0].descriptorCount = 1;
			bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			bindings[1].binding = 1;
			bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			bindings[1].descriptorCount = 1;
			bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			bindings[2].binding = 2;
			bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			bindings[2].descriptorCount = 1;
			bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			bindings[3].binding = 3;
			bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			bindings[3].descriptorCount = 1;
			bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			bindings[4].binding = 4;
			bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bindings[4].descriptorCount = 1;
			bindings[4].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			descriptorSetLayoutCreateInfo.pBindings = bindings;
			descriptorSetLayoutCreateInfo.bindingCount = 5;

			if (vkCreateDescriptorSetLayout(Engine::get()->device, &descriptorSetLayoutCreateInfo, nullptr, &filterDescriptorSetLayout) != VK_SUCCESS) {
				throw runtime_error("Failed to create filter descriptor set layout");
			}
		}

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

	if (filtertype == OIOO) {
		VkDescriptorImageInfo destinationInfo = {};
		destinationInfo.imageView = filterTarget[0]->textureImageView;
		destinationInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkDescriptorImageInfo sourceInfo = {};
		sourceInfo.imageView = source[0]->textureImageView;
		sourceInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		if (!hasUniformBuffer) {
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
		else {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = bufferRef;
			bufferInfo.offset = 0;
			bufferInfo.range = bufferSize;

			VkWriteDescriptorSet descWrite[3] = {};

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

			descWrite[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descWrite[2].dstSet = filterDescriptorSet;
			descWrite[2].dstBinding = 2;
			descWrite[2].dstArrayElement = 0;
			descWrite[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descWrite[2].descriptorCount = 1;
			descWrite[2].pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(Engine::get()->device, 3, descWrite, 0, nullptr);
		}
		
	} else if (filtertype == TIOO){
		VkDescriptorImageInfo destinationInfo = {};
		destinationInfo.imageView = filterTarget[0]->textureImageView;
		destinationInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkDescriptorImageInfo source0Info = {};
		source0Info.imageView = source[0]->textureImageView;
		source0Info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkDescriptorImageInfo source1Info = {};
		source1Info.imageView = source[1]->textureImageView;
		source1Info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		if (!hasUniformBuffer) {
			VkWriteDescriptorSet descWrite[3] = {};

			descWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descWrite[0].dstSet = filterDescriptorSet;
			descWrite[0].dstBinding = 0;
			descWrite[0].descriptorCount = 1;
			descWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descWrite[0].pImageInfo = &source0Info;

			descWrite[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descWrite[1].dstSet = filterDescriptorSet;
			descWrite[1].dstBinding = 1;
			descWrite[1].descriptorCount = 1;
			descWrite[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descWrite[1].pImageInfo = &source1Info;

			descWrite[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descWrite[2].dstSet = filterDescriptorSet;
			descWrite[2].dstBinding = 2;
			descWrite[2].descriptorCount = 1;
			descWrite[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descWrite[2].pImageInfo = &destinationInfo;

			vkUpdateDescriptorSets(Engine::get()->device, 3, descWrite, 0, nullptr);
		}
		else {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = bufferRef;
			bufferInfo.offset = 0;
			bufferInfo.range = bufferSize;

			VkWriteDescriptorSet descWrite[4] = {};

			descWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descWrite[0].dstSet = filterDescriptorSet;
			descWrite[0].dstBinding = 0;
			descWrite[0].descriptorCount = 1;
			descWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descWrite[0].pImageInfo = &source0Info;

			descWrite[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descWrite[1].dstSet = filterDescriptorSet;
			descWrite[1].dstBinding = 1;
			descWrite[1].descriptorCount = 1;
			descWrite[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descWrite[1].pImageInfo = &source1Info;

			descWrite[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descWrite[2].dstSet = filterDescriptorSet;
			descWrite[2].dstBinding = 2;
			descWrite[2].descriptorCount = 1;
			descWrite[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descWrite[2].pImageInfo = &destinationInfo;

			descWrite[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descWrite[3].dstSet = filterDescriptorSet;
			descWrite[3].dstBinding = 3;
			descWrite[3].dstArrayElement = 0;
			descWrite[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descWrite[3].descriptorCount = 1;
			descWrite[3].pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(Engine::get()->device, 4, descWrite, 0, nullptr);
		}
		
	}
	else if (filtertype == THIOO) {
		VkDescriptorImageInfo destinationInfo = {};
		destinationInfo.imageView = filterTarget[0]->textureImageView;
		destinationInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkDescriptorImageInfo source0Info = {};
		source0Info.imageView = source[0]->textureImageView;
		source0Info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkDescriptorImageInfo source1Info = {};
		source1Info.imageView = source[1]->textureImageView;
		source1Info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkDescriptorImageInfo source2Info = {};
		source2Info.imageView = source[2]->textureImageView;
		source2Info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		if (!hasUniformBuffer) {
			VkWriteDescriptorSet descWrite[4] = {};

			descWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descWrite[0].dstSet = filterDescriptorSet;
			descWrite[0].dstBinding = 0;
			descWrite[0].descriptorCount = 1;
			descWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descWrite[0].pImageInfo = &source0Info;

			descWrite[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descWrite[1].dstSet = filterDescriptorSet;
			descWrite[1].dstBinding = 1;
			descWrite[1].descriptorCount = 1;
			descWrite[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descWrite[1].pImageInfo = &source1Info;

			descWrite[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descWrite[2].dstSet = filterDescriptorSet;
			descWrite[2].dstBinding = 2;
			descWrite[2].descriptorCount = 1;
			descWrite[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descWrite[2].pImageInfo = &source2Info;

			descWrite[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descWrite[3].dstSet = filterDescriptorSet;
			descWrite[3].dstBinding = 3;
			descWrite[3].descriptorCount = 1;
			descWrite[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descWrite[3].pImageInfo = &destinationInfo;

			vkUpdateDescriptorSets(Engine::get()->device, 4, descWrite, 0, nullptr);
		}
		else {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = bufferRef;
			bufferInfo.offset = 0;
			bufferInfo.range = bufferSize;

			VkWriteDescriptorSet descWrite[5] = {};

			descWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descWrite[0].dstSet = filterDescriptorSet;
			descWrite[0].dstBinding = 0;
			descWrite[0].descriptorCount = 1;
			descWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descWrite[0].pImageInfo = &source0Info;

			descWrite[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descWrite[1].dstSet = filterDescriptorSet;
			descWrite[1].dstBinding = 1;
			descWrite[1].descriptorCount = 1;
			descWrite[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descWrite[1].pImageInfo = &source1Info;

			descWrite[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descWrite[2].dstSet = filterDescriptorSet;
			descWrite[2].dstBinding = 2;
			descWrite[2].descriptorCount = 1;
			descWrite[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descWrite[2].pImageInfo = &source2Info;

			descWrite[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descWrite[3].dstSet = filterDescriptorSet;
			descWrite[3].dstBinding = 3;
			descWrite[3].descriptorCount = 1;
			descWrite[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descWrite[3].pImageInfo = &destinationInfo;

			descWrite[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descWrite[4].dstSet = filterDescriptorSet;
			descWrite[4].dstBinding = 4;
			descWrite[4].dstArrayElement = 0;
			descWrite[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descWrite[4].descriptorCount = 1;
			descWrite[4].pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(Engine::get()->device, 5, descWrite, 0, nullptr);
		}
		
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
	filterTarget[0]->textureFormat = targetFormat;
	filterTarget[0]->textureUsage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; // Transfer dst might cause issues? I'm not sure yet
	filterTarget[0]->textureLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	filterTarget[0]->texWidth = texWidth;
	filterTarget[0]->texHeight = texHeight;
	filterTarget[0]->mipLevels = 1;
	filterTarget[0]->setup();
	filterTarget[0]->transitionImageLayout(filterTarget[0]->textureImage, filterTarget[0]->textureFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, filterTarget[0]->mipLevels);
}

void filter::filterImage() {
	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeComputeCommand();

	vkQueueWaitIdle(Engine::get()->computeQueue);

	//filterTarget[0]->transitionImageLayout(filterTarget[0]->textureImage, filterTarget[0]->textureFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, filterTarget[0]->mipLevels);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, filterPipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, filterPipelineLayout, 0, 1, &filterDescriptorSet, 0, 0);
	vkCmdDispatch(commandBuffer, source[0]->texWidth / 16, source[0]->texHeight / 16, 1);

	Engine::get()->endSingleTimeComputeCommand(commandBuffer);

	filterTarget[0]->transitionImageLayout(filterTarget[0]->textureImage, filterTarget[0]->textureFormat, VK_IMAGE_LAYOUT_GENERAL, filterTarget[0]->textureLayout, filterTarget[0]->mipLevels);

}

void filter::filterImage(VkCommandBuffer commandBuffer) {
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, filterPipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, filterPipelineLayout, 0, 1, &filterDescriptorSet, 0, 0);
	vkCmdDispatch(commandBuffer, source[0]->texWidth / 16, source[0]->texHeight / 16, 1);
}
