#include"GenerateNormalMap.h"
#include"include/ShaderDataType.h"
#include"include/NormalGenerator.h"

using namespace std;

void NormalGen::prepGenerateOSMap() {
	objectSpaceMap.colour = loadList->replacePtr(new Texture, "OSGenTex");

	objectSpaceMap.colour->texWidth = DEFAULTMAPDIM;
	objectSpaceMap.colour->texHeight = DEFAULTMAPDIM;
	objectSpaceMap.colour->texChannels = 4;
	objectSpaceMap.colour->mipLevels = 1;
	objectSpaceMap.colour->textureFormat = MAP_COLOUR_FORMAT;
	objectSpaceMap.colour->textureUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	objectSpaceMap.colour->createImage(VK_SAMPLE_COUNT_1_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	objectSpaceMap.colour->textureImageView = objectSpaceMap.colour->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

	array<VkAttachmentDescription, 1> attachmentDescriptions = {};
	attachmentDescriptions[0].format = MAP_COLOUR_FORMAT;
	attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
	renderPassInfo.pAttachments = attachmentDescriptions.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	if (vkCreateRenderPass(Engine::get()->device, &renderPassInfo, nullptr, &objectSpaceMap.renderPass) != VK_SUCCESS) {
		throw runtime_error("Failed to create render pass");
	}

	VkImageView attachments[1];
	attachments[0] = objectSpaceMap.colour->textureImageView;

	VkFramebufferCreateInfo fbufCreateInfo = {};
	fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbufCreateInfo.renderPass = objectSpaceMap.renderPass;
	fbufCreateInfo.attachmentCount = 1;
	fbufCreateInfo.pAttachments = attachments;
	fbufCreateInfo.width = objectSpaceMap.colour->texWidth;
	fbufCreateInfo.height = objectSpaceMap.colour->texHeight;
	fbufCreateInfo.layers = 1;

	if (vkCreateFramebuffer(Engine::get()->device, &fbufCreateInfo, nullptr, &objectSpaceMap.frameBuffer) != VK_SUCCESS) {
		throw runtime_error("Failed to create framebuffer");
	}
}

void NormalGen::createGenerateOSPipeline() {
	shaderData* sD = new NORMALGENERATORSHADER;

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_TRUE;
	multisampling.rasterizationSamples = msaaSamples;
	multisampling.minSampleShading = .2f;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;

	if (vkCreatePipelineLayout(Engine::get()->device, &pipelineLayoutInfo, nullptr, &OSpipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	auto VertShaderCode = sD->vertData;
	auto FragShaderCode = sD->fragData;

	VkShaderModule VertShaderModule = Engine::get()->createShaderModule(VertShaderCode);
	VkShaderModule FragShaderModule = Engine::get()->createShaderModule(FragShaderCode);

	VkPipelineShaderStageCreateInfo VertShaderStageInfo{};
	VertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	VertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	VertShaderStageInfo.module = VertShaderModule;
	VertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo FragShaderStageInfo{};
	FragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	FragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	FragShaderStageInfo.module = FragShaderModule;
	FragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo, FragShaderStageInfo };

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = ShaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = OSpipelineLayout;
	pipelineInfo.renderPass = objectSpaceMap.renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(Engine::get()->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &OSpipeline) != VK_SUCCESS) {
		throw runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(Engine::get()->device, FragShaderModule, nullptr);
	vkDestroyShaderModule(Engine::get()->device, VertShaderModule, nullptr);

	delete sD;
}

VkCommandBuffer NormalGen::drawOSMap(VkCommandBuffer commandbuffer, Mesh* mesh) {

	VkClearValue clearValues[1] = {};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = objectSpaceMap.renderPass;
	renderPassBeginInfo.framebuffer = objectSpaceMap.frameBuffer;
	renderPassBeginInfo.renderArea.extent.width = objectSpaceMap.colour->texWidth;
	renderPassBeginInfo.renderArea.extent.height = objectSpaceMap.colour->texHeight;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(commandbuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(objectSpaceMap.colour->texWidth);
	viewport.height = static_cast<float>(objectSpaceMap.colour->texHeight);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandbuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = { objectSpaceMap.colour->texWidth, objectSpaceMap.colour->texHeight };
	vkCmdSetScissor(commandbuffer, 0, 1, &scissor);

	vkCmdBindPipeline(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, OSpipeline);

	VkBuffer vertexBuffers[] = { mesh->vertexBuffer };
	VkDeviceSize offsets[] = { 0 };

	mesh->createTexCoordIndexBuffer();

	vkCmdBindVertexBuffers(commandbuffer, 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(commandbuffer, mesh->texCoordIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(commandbuffer, static_cast<uint32_t>(mesh->uniqueTexindices.size()), 1, 0, 0, 0);

	vkCmdEndRenderPass(commandbuffer);

	return commandbuffer;
}

void NormalGen::cleanupGenOS() {
	vkDeviceWaitIdle(Engine::get()->device);

	vkDestroyPipeline(Engine::get()->device, OSpipeline, nullptr);
	vkDestroyPipelineLayout(Engine::get()->device, OSpipelineLayout, nullptr);

	vkDestroyRenderPass(Engine::get()->device, objectSpaceMap.renderPass, nullptr);
	vkDestroyFramebuffer(Engine::get()->device, objectSpaceMap.frameBuffer, nullptr);

	objectSpaceMap.colour->cleanup();
}