#include"Bobject_VKMem.h"

uint32_t VKMem::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(Core::getPhysicalDevice(), &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

void VKMem::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(Core::getDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(Core::getDevice(), buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(Core::getDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(Core::getDevice(), buffer, bufferMemory, 0);
}

void VKMem::freeBuffer(VkBuffer buffer, VkDeviceMemory memory) {
	vkDestroyBuffer(Core::getDevice(), buffer, nullptr);
	vkFreeMemory(Core::getDevice(), memory, nullptr);
}

void VKMem::copyDataToBuffer(VkBuffer buffer, VkDeviceMemory memory, void* bufferData, VkDeviceSize bufferSize) {
	void* data{};
	vkMapMemory(Core::getDevice(), memory, 0, bufferSize, 0, &data);
	memcpy(data, bufferData, (size_t)bufferSize);
	vkUnmapMemory(Core::getDevice(), memory);
	// Data cannot be deleted or freed because it will cause an RtlValidateHeap error
}