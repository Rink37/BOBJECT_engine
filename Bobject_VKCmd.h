#ifndef BOBJECT_CMD
#define BOBJECT_CMD

#include"Bobject_Core.h"

namespace VKCmd {
	VkCommandBuffer allocateSingleTimeCommand(VkCommandPool);
	void submitSingleTimeCommand(VkCommandBuffer, VkCommandPool, VkQueue);
}

#endif
