#include"Bobject_VKCmd.h"

VkCommandBuffer VKCmd::allocateSingleTimeCommand(VkCommandPool commandPool) {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	VkCommandBuffer commandBuffer;
	if (vkAllocateCommandBuffers(Core::getDevice(), &allocInfo, &commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffer!");
	};

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Failed to begin single time command!");
	}

	return commandBuffer;
}

VkCommandBuffer VKCmd::allocateSingleTimeCommand(Queue* queue) {
	return VKCmd::allocateSingleTimeCommand(queue->commandPool);
}

void VKCmd::submitSingleTimeCommand(VkCommandBuffer commandBuffer, VkCommandPool commandPool, VkQueue queue) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit command buffer to the queue!");
	};

	if (vkQueueWaitIdle(queue) != VK_SUCCESS) {
		throw std::runtime_error("Failed to wait for compute queue");
	};

	vkFreeCommandBuffers(Core::getDevice(), commandPool, 1, &commandBuffer);
}

void VKCmd::submitSingleTimeCommand(VkCommandBuffer commandBuffer, Queue* queue) {
	VKCmd::submitSingleTimeCommand(commandBuffer, queue->commandPool, queue->queue);
}