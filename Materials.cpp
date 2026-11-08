#include"Materials.h"

using namespace std;

void Material::createDescriptorPool(std::string targetShader, GraphicsPass* currentPass) {
	createDescriptorPool(&currentPass->shaderDatas[currentPass->pipelineMap.at(targetShader)]);
}

void Material::createDescriptorPool(shaderData* sD) {
	std::vector<shaderIOValue>* IOvals = &sD->IO;

	VkDescriptorPoolCreateInfo poolInfo{};
	std::vector<VkDescriptorPoolSize> poolSizes{};

	for (shaderIOValue IOval : *IOvals) {
		VkDescriptorPoolSize poolSize{};
		poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		if (IOval.type == 1) {
			poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		}
		else {
			poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		}

		poolSizes.push_back(poolSize);
	}

	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	if (vkCreateDescriptorPool(Engine::get()->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw runtime_error("failed to create descriptor pool!");
	}
}

void Material::createDescriptorSets(std::string targetShader, GraphicsPass* currentPass) {
	
	if (currentPass->pipelineMap.count(targetShader) == 0) {
		throw runtime_error("Shader is not attached to a valid pipeline");
	}

	createDescriptorSets(&currentPass->shaderDatas[currentPass->pipelineMap.at(targetShader)]);

}

void Material::createDescriptorSets(shaderData* sD) {

	VkDescriptorSetAllocateInfo allocInfo{};
	std::vector<shaderIOValue>* IOvals = &sD->IO;

	std::string key = "";

	uint8_t textureCount = 0;
	uint8_t bufferCount = 0;

	for (shaderIOValue ioVal : *IOvals) {
		key += std::to_string(ioVal.type) + "_";
		if (ioVal.type == 0) {
			textureCount++;
		}
		else {
			bufferCount++;
		}
	}

	if (textures.size() < textureCount) {
		throw runtime_error("Material could not be created - not enough textures were provided");
	}
	if (buffers.size() < bufferCount) {
		throw runtime_error("Insufficient buffers to cover all shader slots!");
	}
	if (pass->layoutMap.count(key) == 0) {
		Engine::get()->createDescriptorSetLayout(key, *IOvals, *pass);
	}

	pipelineLayoutIndex = pass->layoutMap.at(key);
	descriptorSetLayout = pass->descriptorSetLayouts[pipelineLayoutIndex];
	pipelineLayout = pass->pipelineLayouts[pipelineLayoutIndex];

	vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

	if (vkAllocateDescriptorSets(Engine::get()->device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
		throw runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		std::vector<VkWriteDescriptorSet> descriptorWrites{ (*IOvals).size(), VkWriteDescriptorSet{} };
		std::vector<VkDescriptorBufferInfo> bufferInfos{ bufferCount, VkDescriptorBufferInfo{} };
		std::vector<VkDescriptorImageInfo> imageInfos{ textureCount, VkDescriptorImageInfo{} };

		uint32_t textureIndex = 0;
		uint32_t bindingIndex = 0;
		uint32_t bufferIndex = 0;
		for (shaderIOValue ioVal : *IOvals) {
			descriptorWrites[bindingIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			
			descriptorWrites[bindingIndex].dstSet = descriptorSets[i];
			descriptorWrites[bindingIndex].dstBinding = bindingIndex;
			descriptorWrites[bindingIndex].dstArrayElement = 0;
			descriptorWrites[bindingIndex].descriptorCount = 1;
			
			if (ioVal.type == 1) {
				if (buffers[bufferIndex].first.size() >= MAX_FRAMES_IN_FLIGHT) {
					bufferInfos[bufferIndex].buffer = buffers[bufferIndex].first[i];
					bufferInfos[bufferIndex].offset = 0;
					bufferInfos[bufferIndex].range = buffers[bufferIndex].second;
				}
				else {
					bufferInfos[bufferIndex].buffer = buffers[bufferIndex].first[0];
					bufferInfos[bufferIndex].offset = 0;
					bufferInfos[bufferIndex].range = buffers[bufferIndex].second;
				}

				descriptorWrites[bindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[bindingIndex].pBufferInfo = &bufferInfos[bufferIndex];

				bufferIndex++;
			}
			else {
				imageInfos[textureIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfos[textureIndex].imageView = textures[textureIndex]->textureImageView;
				imageInfos[textureIndex].sampler = Engine::get()->textureSampler;

				descriptorWrites[bindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[bindingIndex].pImageInfo = &imageInfos[textureIndex];

				textureIndex++;
			}

			bindingIndex++;
		}

		vkUpdateDescriptorSets(Engine::get()->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}