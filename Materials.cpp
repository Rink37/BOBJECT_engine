#include"Materials.h"

using namespace std;

void Material::createDescriptorPool(std::string targetShader, GraphicsPass* currentPass) {
	std::vector<shaderIOValue>* IOvals = &currentPass->shaderDatas[currentPass->pipelineMap.at(targetShader)].IO;

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
		std::cout << targetShader << std::endl;
		throw runtime_error("Shader is not attached to a valid pipeline");
	}

	VkDescriptorSetAllocateInfo allocInfo{};
	std::vector<shaderIOValue>* IOvals = &currentPass->shaderDatas[currentPass->pipelineMap.at(targetShader)].IO;

	std::string key = "";

	uint8_t textureCount = 0;

	for (shaderIOValue ioVal : *IOvals) {
		key += std::to_string(ioVal.type) + "_";
		if (ioVal.type == 0) {
			textureCount++;
		}
	}

	if (textures.size() < textureCount) {
		throw runtime_error("Material could not be created - not enough textures were provided");
	}

	pipelineLayoutIndex = Engine::get()->defaultPass.layoutMap.at(key);
	descriptorSetLayout = Engine::get()->defaultPass.descriptorSetLayouts[pipelineLayoutIndex];
	pipelineLayout = Engine::get()->defaultPass.pipelineLayouts[pipelineLayoutIndex];

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
		std::vector<VkDescriptorImageInfo> imageInfos{ textureCount, VkDescriptorImageInfo{} };
		
		uint32_t textureIndex = 0;
		uint32_t bindingIndex = 0;
		uint32_t bufferIndex = 0;
		for (shaderIOValue ioVal : *IOvals) {

			if (ioVal.type == 1) {
				if (bufferIndex >= buffers.size()) {
					throw runtime_error("Insufficient buffers to cover all shader slots!");
				}

				VkDescriptorBufferInfo bufferInfo{};
				if (buffers[bufferIndex].first.size() >= MAX_FRAMES_IN_FLIGHT) {
					bufferInfo.buffer = buffers[bufferIndex].first[i];
					bufferInfo.offset = 0;
					bufferInfo.range = buffers[bufferIndex].second;
				}
				else {
					bufferInfo.buffer = buffers[bufferIndex].first[0];
					bufferInfo.offset = 0;
					bufferInfo.range = buffers[bufferIndex].second;
				}
				
				descriptorWrites[bindingIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[bindingIndex].dstSet = descriptorSets[i];
				descriptorWrites[bindingIndex].dstBinding = bindingIndex;
				descriptorWrites[bindingIndex].dstArrayElement = 0;
				descriptorWrites[bindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[bindingIndex].descriptorCount = 1;
				descriptorWrites[bindingIndex].pBufferInfo = &bufferInfo;
				
				bufferIndex++;
			}
			else {
				if (textureIndex >= textures.size()) {
					throw runtime_error("Insufficient textures to cover all shader slots!");
				}

				imageInfos[textureIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfos[textureIndex].imageView = textures[textureIndex]->textureImageView;
				imageInfos[textureIndex].sampler = Engine::get()->textureSampler;

				descriptorWrites[bindingIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[bindingIndex].dstSet = descriptorSets[i];
				descriptorWrites[bindingIndex].dstBinding = bindingIndex;
				descriptorWrites[bindingIndex].dstArrayElement = 0;
				descriptorWrites[bindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[bindingIndex].descriptorCount = 1;
				descriptorWrites[bindingIndex].pImageInfo = &imageInfos[textureIndex];

				textureIndex++;
			}

			bindingIndex++;
		}

		vkUpdateDescriptorSets(Engine::get()->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}