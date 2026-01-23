#include"ImageProcessor.h"

using namespace std;

// https://github.com/ttddee/ShaderDev/blob/master/src/vulkanrenderer.cpp

void filter::getFilterLayout(shaderData* sD) {
	bindingMap = sD->bindingMap;
	bindingDirections = sD->bindingDirections;
	if (bindingMap.size() == 0 || bindingDirections.size() == 0) {
		return;
	}

	int inputCount = 0;
	int outputCount = 0;
	int inputImageCount = 0;
	int outputImageCount = 0;
	
	int index = 0;
	for (std::map<std::string, int>::iterator i = bindingMap.begin();i != bindingMap.end(); i++) {
		if (bindingDirections.at(index)) {
			inputCount++;
			if (i->second == 0) {
				inputImageCount++;
			}
		}
		else {
			outputCount++;
			if (i->second == 0) {
				outputImageCount++;
			}
		}
		index++;
	}

	std::cout << "Input count = " << inputCount << std::endl;
	std::cout << "Output count = " << outputCount << std::endl;

	std::cout << "Input image count = " << inputImageCount << std::endl;
	std::cout << "Output image count = " << outputImageCount << std::endl;
}

void filter::autoCreateDescriptorSetLayout() {
	int descriptorElementCount = bindingDirections.size();

	VkDescriptorPoolSize descPoolSize = {
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 
		descriptorElementCount
	};

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(1);
	poolInfo.pPoolSizes = &descPoolSize;
	poolInfo.maxSets = static_cast<uint32_t>(1);

	if (vkCreateDescriptorPool(Engine::get()->device, &poolInfo, nullptr, &descPool) != VK_SUCCESS) {
		throw runtime_error("Failed to create descriptor pool!");
	}

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

	std::vector<VkDescriptorSetLayoutBinding> bindings;
	int index = 0;
	for (std::map<std::string, int>::iterator it = bindingMap.begin(); it != bindingMap.end(); it++) {
		VkDescriptorSetLayoutBinding bindingElement;
		bindingElement.binding = index;
		if (it->second == 0) {
			bindingElement.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		}
		else if (it->second == 1) {
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

void filter::autoCreateDescriptorSet() {
	int descriptorElementCount = bindingDirections.size();

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
	for (std::map<std::string, int>::iterator it = bindingMap.begin(); it != bindingMap.end(); it++) {
		VkWriteDescriptorSet writeElement = {};
		writeElement.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeElement.dstSet = filterDescriptorSet;
		writeElement.dstBinding = index;
		writeElement.descriptorCount = 1;
		if (it->second == 0) {
			VkDescriptorImageInfo* imageInfo = new VkDescriptorImageInfo;
			if (bindingDirections[index]) {
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
		else if (it->second == 1) {
			if (!bindingDirections[index]) {
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
	//filterTarget[0]->transitionImageLayout(filterTarget[0]->textureImage, filterTarget[0]->textureFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, filterTarget[0]->mipLevels);
}

void filter::filterImage() {
	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeComputeCommand();

	vkQueueWaitIdle(Engine::get()->computeQueue);

	filterTarget[0]->transitionImageLayout(filterTarget[0]->textureImage, filterTarget[0]->textureFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, filterTarget[0]->mipLevels);

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

int countInputs(std::vector<bool> directions) {
	int count = 0;
	for (bool dir : directions) {
		if (dir) {
			count++;
		}
	}
	return count;
}

postProcessFilter::postProcessFilter(shaderData* sd) {
	filterShaderModule = Engine::get()->createShaderModule(sd->compData);
	int inputCount = countInputs(sd->bindingDirections);
	if (inputCount > 1) {
		throw runtime_error("Post-process shaders with multiple inputs are not currently supported, the specified shader is invalid");
	}
	hasOutput = (inputCount != sd->bindingDirections.size());
	if (hasOutput){
		std::cout << "A post-process filter has detected that it has an output" << std::endl;
		if (sd->bindingDirections.size() - inputCount > 1) {
			throw runtime_error("Post-process shaders with multiple outputs are not currently supported, the specified shader is invalid");
		}
		createOutputImages();
	}
	createDescriptorSetLayout();
	createDescriptorSets();
	createFilterPipelineLayout();
	createFilterPipeline();
}

void postProcessFilter::setup(shaderData* sd, drawImage* target) {
	filterShaderModule = Engine::get()->createShaderModule(sd->compData);
	this->target = target;
	int inputCount = countInputs(sd->bindingDirections);
	if (inputCount > 1) {
		throw runtime_error("Post-process shaders with multiple inputs are not currently supported, the specified shader is invalid");
	}
	hasOutput = (inputCount != sd->bindingDirections.size());
	if (hasOutput) {
		std::cout << "A post-process filter has detected that it has an output" << std::endl;
		if (sd->bindingDirections.size() - inputCount > 1) {
			throw runtime_error("Post-process shaders with multiple outputs are not currently supported, the specified shader is invalid");
		}
		createOutputImages();
	}
	createDescriptorSetLayout();
	createDescriptorSets();
	createFilterPipelineLayout();
	createFilterPipeline();
}

void postProcessFilter::filterImage(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
	VkImage& image = target->images[imageIndex];
	transitionImageLayout(commandBuffer, target->images[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, filterPipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, filterPipelineLayout, 0, 1, &filterDescriptorSets[imageIndex], 0, 0);
	vkCmdDispatch(commandBuffer, target->imageExtent.width / 16, target->imageExtent.height / 16, 1);
	transitionImageLayout(commandBuffer, target->images[imageIndex], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
}

void postProcessFilter::createDescriptorSetLayout() {

	uint32_t imageCount = target->images.size();

	array<VkDescriptorPoolSize, 1> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(imageCount);

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(imageCount);

	if (vkCreateDescriptorPool(Engine::get()->device, &poolInfo, nullptr, &descPool) != VK_SUCCESS) {
		throw runtime_error("failed to create descriptor pool!");
	}

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

	VkDescriptorSetLayoutBinding bindings[1] = {};

	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	descriptorSetLayoutCreateInfo.pBindings = bindings;
	descriptorSetLayoutCreateInfo.bindingCount = 1;

	if (vkCreateDescriptorSetLayout(Engine::get()->device, &descriptorSetLayoutCreateInfo, nullptr, &filterDescriptorSetLayout) != VK_SUCCESS) {
		throw runtime_error("Failed to create inplace filter descriptor set layout");
	}
}

void postProcessFilter::createDescriptorSets() {

	uint32_t imageCount = target->images.size();
	
	vector<VkDescriptorSetLayout> layouts(imageCount, filterDescriptorSetLayout);

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(imageCount);
	allocInfo.pSetLayouts = layouts.data();

	filterDescriptorSets.resize(imageCount);

	if (vkAllocateDescriptorSets(Engine::get()->device, &allocInfo, filterDescriptorSets.data()) != VK_SUCCESS) {
		throw runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < imageCount; i++) {
		VkDescriptorImageInfo sourceInfo = {};
		sourceInfo.imageView = target->imageViews[i];
		sourceInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet descWrite[1] = {};

		descWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descWrite[0].dstSet = filterDescriptorSets[i];
		descWrite[0].dstBinding = 0;
		descWrite[0].descriptorCount = 1;
		descWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descWrite[0].pImageInfo = &sourceInfo;

		vkUpdateDescriptorSets(Engine::get()->device, 1, descWrite, 0, nullptr);
	}
}

void postProcessFilter::recreateDescriptorSets() {

	uint32_t imageCount = target->images.size();

	for (size_t i = 0; i < imageCount; i++) {
		VkDescriptorImageInfo sourceInfo = {};
		sourceInfo.imageView = target->imageViews[i];
		sourceInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet descWrite[1] = {};

		descWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descWrite[0].dstSet = filterDescriptorSets[i];
		descWrite[0].dstBinding = 0;
		descWrite[0].descriptorCount = 1;
		descWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descWrite[0].pImageInfo = &sourceInfo;

		vkUpdateDescriptorSets(Engine::get()->device, 1, descWrite, 0, nullptr);
	}
}

void postProcessFilter::createFilterPipelineLayout() {
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &filterDescriptorSetLayout;

	if (vkCreatePipelineLayout(Engine::get()->device, &pipelineLayoutInfo, nullptr, &filterPipelineLayout) != VK_SUCCESS) {
		throw runtime_error("Failed to create filter pipeline layout");
	}
}

void postProcessFilter::createFilterPipeline() {
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

bool hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void postProcessFilter::transitionImageLayout(VkCommandBuffer commandBuffer, VkImage& image, VkImageLayout oldLayout, VkImageLayout newLayout) {

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL){
		barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		
		sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else {
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
}

void postProcessFilter::cleanup() {
	VkDevice device = Engine::get()->device;

	vkDestroyPipeline(device, filterPipeline, nullptr);
	vkDestroyPipelineLayout(device, filterPipelineLayout, nullptr);

	vkDestroyDescriptorSetLayout(device, filterDescriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device, descPool, nullptr);

	for (size_t i = 0; i != outputImages.size(); i++) {
		vkDestroyImage(device, outputImages[i], nullptr);
		vkDestroyImageView(device, outputImageViews[i], nullptr);
		vkFreeMemory(device, outputImageMemories[i], nullptr);
	}
}

void postProcessFilter::createOutputImages() {
	VkDevice device = Engine::get()->device;

	uint32_t imageCount = target->images.size();

	outputImages.resize(imageCount);
	outputImageMemories.resize(imageCount);

	uint32_t width = 0;
	uint32_t height = 0;

	width = target->imageExtent.width;
	height = target->imageExtent.height;

	for (size_t i = 0; i != imageCount; i++) {
		Engine::get()->createImage(width, height, 1, VK_SAMPLE_COUNT_1_BIT, target->imageFormat, VK_IMAGE_TILING_OPTIMAL, target->imageUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, outputImages[i], outputImageMemories[i]);
		Engine::get()->transitionImageLayout(outputImages[i], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL); // This causes no problems - all draw images should be in the expected layout
	}

	outputImageViews.resize(outputImages.size());

	for (uint32_t i = 0; i < outputImages.size(); i++) {
		outputImageViews[i] = Engine::get()->createImageView(outputImages[i], target->imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}

}