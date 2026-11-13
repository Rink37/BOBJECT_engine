#ifndef BOBJECT_MEM
#define BOBJECT_MEM

#include"Bobject_Core.h"

namespace VKMem {
	uint32_t findMemoryType(uint32_t, VkMemoryPropertyFlags);
	void createBuffer(VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags, VkBuffer&, VkDeviceMemory&);
	void freeBuffer(VkBuffer, VkDeviceMemory);
	void copyDataToBuffer(VkBuffer, VkDeviceMemory, void*, VkDeviceSize);
}

#endif
