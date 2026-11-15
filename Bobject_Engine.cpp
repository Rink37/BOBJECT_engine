#include "Bobject_Engine.h"

using namespace std;

void Engine::createSwapChain() {
	SwapChainSupportDetails swapChainSupport = Core::get()->getSwapChainSupport();

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	imageCount = swapChainSupport.capabilities.minImageCount + 1;

	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = Core::getSurface();
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	QueueFamilyIndices indices = Core::get()->indices;
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		throw runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainTextures.resize(imageCount);
	std::vector<VkImage> swapChainImages(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
	for (uint32_t i = 0; i != imageCount; i++) {
		swapChainTextures[i] = new Texture;
		swapChainTextures[i]->textureImage = swapChainImages[i];
	}

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

	for (Texture* tex : swapChainTextures) {
		tex->textureFormat = swapChainImageFormat;
		tex->textureAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		tex->mipLevels = 1;
		tex->texWidth = swapChainExtent.width;
		tex->texHeight = swapChainExtent.height;
		tex->texChannels = 4;
		tex->transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		tex->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

void Engine::createRenderPass(VkRenderPass& newRenderPass, VkFormat imageFormat, VkImageLayout finalLayout) {

	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = imageFormat;
	colorAttachment.samples = Core::maxSamples();
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = Core::maxSamples();
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colourAttachmentResolve{};
	colourAttachmentResolve.format = imageFormat;
	colourAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colourAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colourAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colourAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colourAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colourAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colourAttachmentResolve.finalLayout = finalLayout;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colourAttachmentResolveRef{};
	colourAttachmentResolveRef.attachment = 2;
	colourAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.pResolveAttachments = &colourAttachmentResolveRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colourAttachmentResolve };
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &newRenderPass) != VK_SUCCESS) {
		throw runtime_error("failed to create render pass!");
	}
}

void Engine::createDescriptorSetLayout(std::string key, std::vector<shaderIOValue> IO, GraphicsPass& graphicsPass) const {
	std::vector<VkDescriptorPoolSize> poolSizes{};
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings{};

	uint32_t index = 0;
	for (shaderIOValue IOval : IO) {
		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.descriptorCount = 1;
		layoutBinding.binding = index;
		layoutBinding.pImmutableSamplers = nullptr;
		index++;
		if (IOval.type == 0) {
			layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		else {
			layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		}
		layoutBindings.push_back(layoutBinding);

	}
	VkDescriptorSetLayout newDescriptorSetLayout = nullptr;
	VkPipelineLayout newPipelineLayout = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	layoutInfo.pBindings = layoutBindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &newDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &newDescriptorSetLayout;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &newPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	graphicsPass.descriptorSetLayouts.push_back(newDescriptorSetLayout);
	graphicsPass.pipelineLayouts.push_back(newPipelineLayout);
	graphicsPass.layoutMap.insert({ key, graphicsPass.descriptorSetLayouts.size() - 1 });
}


VkShaderModule Engine::createShaderModule(const vector<unsigned char>& code) const {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw runtime_error("failed to create shader module!");
	}

	return shaderModule;
}

void Engine::createGraphicsPipelines() {
	createGraphicsPipelines(defaultPass);
}

void Engine::createGraphicsPipelines(GraphicsPass& graphicsPass, int blendMode, bool alphaToCoverage) const {

	shaderData flatShader = FLATSHADER;
	shaderData projectorFlatShader = PROJECTOR_FLATSHADER;
	shaderData AC_flatShader = AC_FLATSHADER;
	shaderData SAC_flatShader = SAC_FLATSHADER;
	shaderData bfShader = BFSHADER;
	shaderData projectorBFShader = PROJECTOR_BFSHADER;
	shaderData AC_bfShader = AC_BFSHADER;
	shaderData SAC_bfShader = SAC_BFSHADER;
	shaderData uiShader = UISHADER;
	shaderData wShader = WSHADER;
	shaderData uvShader = UVSHADER;
	shaderData uiShaderGray = UIGRAYSHADER;
	shaderData OS_BF = OS_BFSHADER;
	shaderData AC_OS_BF = AC_OS_BFSHADER;
	shaderData TS_BF = TS_BFSHADER;
	shaderData AC_TS_BF = AC_TS_BFSHADER;
	shaderData uiTextShader = UITEXTSHADER;
	shaderData uiRoundBox = UIROUNDBOXSHADER;
	shaderData projectorBackgroundShader = PROJECTOR_BACKGROUNDSHADER;
	
	std::vector<int> vertexInputInfoIndices;

	graphicsPass.shaderDatas.push_back(flatShader);
	vertexInputInfoIndices.push_back(0);
	graphicsPass.shaderDatas.push_back(projectorFlatShader);
	vertexInputInfoIndices.push_back(0);
	graphicsPass.shaderDatas.push_back(AC_flatShader);
	vertexInputInfoIndices.push_back(0);
	graphicsPass.shaderDatas.push_back(SAC_flatShader);
	vertexInputInfoIndices.push_back(0);
	graphicsPass.shaderDatas.push_back(bfShader);
	vertexInputInfoIndices.push_back(0);
	graphicsPass.shaderDatas.push_back(projectorBFShader);
	vertexInputInfoIndices.push_back(0);
	graphicsPass.shaderDatas.push_back(AC_bfShader);
	vertexInputInfoIndices.push_back(0);
	graphicsPass.shaderDatas.push_back(SAC_bfShader);
	vertexInputInfoIndices.push_back(0);
	graphicsPass.shaderDatas.push_back(uiShader);
	vertexInputInfoIndices.push_back(0);
	graphicsPass.shaderDatas.push_back(wShader);
	vertexInputInfoIndices.push_back(0);
	graphicsPass.shaderDatas.push_back(uvShader);
	vertexInputInfoIndices.push_back(0);
	graphicsPass.shaderDatas.push_back(uiShaderGray);
	vertexInputInfoIndices.push_back(0);
	graphicsPass.shaderDatas.push_back(OS_BF);
	vertexInputInfoIndices.push_back(0);
	graphicsPass.shaderDatas.push_back(AC_OS_BF);
	vertexInputInfoIndices.push_back(0);
	graphicsPass.shaderDatas.push_back(TS_BF);
	vertexInputInfoIndices.push_back(1);
	graphicsPass.shaderDatas.push_back(AC_TS_BF);
	vertexInputInfoIndices.push_back(1);
	graphicsPass.shaderDatas.push_back(uiTextShader);
	vertexInputInfoIndices.push_back(0);
	graphicsPass.shaderDatas.push_back(uiRoundBox);
	vertexInputInfoIndices.push_back(0);
	graphicsPass.shaderDatas.push_back(projectorBackgroundShader);
	vertexInputInfoIndices.push_back(0);

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineVertexInputStateCreateInfo tangentVertexInputInfo{};
	tangentVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto tangentAttributeDescriptions = Vertex::getCompleteAttributeDescriptions();

	tangentVertexInputInfo.vertexBindingDescriptionCount = 1;
	tangentVertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(tangentAttributeDescriptions.size());
	tangentVertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	tangentVertexInputInfo.pVertexAttributeDescriptions = tangentAttributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_TRUE;
	multisampling.rasterizationSamples = Core::maxSamples();
	multisampling.minSampleShading = .2f;
	multisampling.alphaToCoverageEnable = alphaToCoverage;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	if (blendMode == 1) {
		// Additive blending
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}
	else if (blendMode == 2) {
		// Alpha blending
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}
	else {
		colorBlendAttachment.blendEnable = VK_FALSE;
	}

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

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineVertexInputStateCreateInfo vertexInputInfos[2] = { vertexInputInfo, tangentVertexInputInfo };

	for (int i = 0; i != graphicsPass.shaderDatas.size(); i++) {
		shaderData* sd = &graphicsPass.shaderDatas[i];

		std::string key = "";

		for (shaderIOValue ioVal : sd->IO) {
			key += std::to_string(ioVal.type) + "_";
		}

		if (graphicsPass.layoutMap.count(key) == 0) {
			createDescriptorSetLayout(key, sd->IO, graphicsPass);
		}

		uint32_t pipelineLayoutIndex = graphicsPass.layoutMap.at(key);

		if (graphicsPass.pipelineMap.count(sd->shaderName) == 0) {
			graphicsPass.pipelineMap.insert({ sd->shaderName, i });
		}

		VkPipeline* CurrentPipeline = new VkPipeline;

		std::vector<unsigned char> VertShaderCode = *(sd->vertData);
		std::vector<unsigned char> FragShaderCode = *(sd->fragData);

		VkShaderModule VertShaderModule = createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = createShaderModule(FragShaderCode); 
		
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
		if (sd->isWireframe) {
			rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
		}
		else {
			rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		}
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = ShaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfos[vertexInputInfoIndices[i]];
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = graphicsPass.pipelineLayouts[pipelineLayoutIndex];
		pipelineInfo.renderPass = graphicsPass.renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.pDepthStencilState = &depthStencil;

		if (vkCreateGraphicsPipelines(Core::getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, CurrentPipeline) != VK_SUCCESS) {
			throw runtime_error("failed to create graphics pipeline!");
		}

		graphicsPass.GraphicsPipelines.push_back(CurrentPipeline);

		vkDestroyShaderModule(device, FragShaderModule, nullptr);
		vkDestroyShaderModule(device, VertShaderModule, nullptr);
	}
}

void Engine::createDepthResources() {
	VkFormat depthFormat = findDepthFormat();
	depthTex = new Texture;
	depthTex->texWidth = swapChainExtent.width;
	depthTex->texHeight = swapChainExtent.height;
	depthTex->mipLevels = 1;
	depthTex->textureSamples = Core::maxSamples();
	depthTex->textureFormat = depthFormat;
	depthTex->textureTiling = VK_IMAGE_TILING_OPTIMAL;
	depthTex->textureUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	depthTex->textureLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthTex->textureMemFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	depthTex->textureAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthTex->setup();
}

void Engine::createTextureSampler() {
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	samplerInfo.anisotropyEnable = VK_TRUE;

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(Core::getPhysicalDevice(), &properties);
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;


	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 10000.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
		throw runtime_error("failed to create texture sampler!");
	}
}

void Engine::createUniformBuffers() {
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
	uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VKMem::createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

		vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
	}
}

void Engine::createColourBuffer() {
	VkDeviceSize bufferSize = sizeof(ColourSchemeObject);

	VKMem::createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, colourBuffer, colourBufferMemory);

	vkMapMemory(device,	colourBufferMemory, 0, bufferSize, 0, &colourBufferMapped);
}

void Engine::createCommandBuffers() {
	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = graphicsQueueObj->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	std::cout << graphicsQueueObj->commandPool << std::endl;

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw runtime_error("failed to allocate command buffers!");
	}
}

void Engine::createSyncObjects() {
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(imageCount);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
			throw runtime_error("failed to create synchronization objects for a frame!");
		}
	}

	for (size_t i = 0; i < imageCount; i++) {
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
			throw runtime_error("failed to create image synchronization objects");
		}
	}
}

void Engine::initEngine(std::string appName) {
	Core::get()->createApp(appName);
	device = Core::getDevice();
	window = Core::getWindow();
	graphicsQueueObj = Core::getSupportedQueue(VK_QUEUE_GRAPHICS_BIT);
	presentQueueObj = Core::getSupportedQueue(QUEUE_SUPPORTS_PRESENT);
	computeQueueObj = Core::getSupportedQueue(VK_QUEUE_COMPUTE_BIT);

	createCommandBuffers();
	createSwapChain();
	createRenderPass(defaultPass.renderPass, swapChainImageFormat, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	createGraphicsPipelines();
	createDepthResources();
	createTextureSampler();
	createUniformBuffers();
	createColourBuffer();
	createSyncObjects();
}

void Engine::cleanup() {
	cleanupSwapChain();

	vkDestroySampler(device, textureSampler, nullptr);

	defaultPass.cleanup(device);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroyBuffer(device, uniformBuffers[i], nullptr);
		vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
	}

	vkDestroyBuffer(device, colourBuffer, nullptr);
	vkFreeMemory(device, colourBufferMemory, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);
	}
	for (size_t i = 0; i < imageCount; i++) {
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
	}
}

void Engine::cleanupSwapChain() {

	depthTex->cleanup();

	for (Texture* tex : swapChainTextures) {
		vkDestroyImageView(device, tex->textureImageView, nullptr);
	}

	vkDestroySwapchainKHR(device, swapChain, nullptr);
}

VkFormat Engine::findSupportedFormat(const vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(Core::getPhysicalDevice(), format, &props);
		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw runtime_error("failed to find supported format!");
}

VkFormat Engine::findDepthFormat() {
	return findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkSurfaceFormatKHR Engine::chooseSwapSurfaceFormat(const vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR Engine::chooseSwapPresentMode(const vector<VkPresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Engine::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {
	if (capabilities.currentExtent.width != numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(Core::getWindow(), &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

void Engine::recreateSwapChain() {
	int width = 0, height = 0;
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(Core::getWindow(), &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(device);
	cleanupSwapChain();

	createSwapChain();
	createDepthResources();
}

void Engine::recreateDrawImage(drawImage* image) const {
	int width = 0, height = 0;
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(Core::getWindow(), &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(device);

	image->cleanup(device);

	for (size_t i = 0; i != imageCount; i++) {
		Texture* tex = new Texture;
		tex->texWidth = width;
		tex->texHeight = height;
		tex->mipLevels = 1;
		tex->textureSamples = VK_SAMPLE_COUNT_1_BIT;
		tex->textureMemFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		tex->textureFormat = image->imageFormat;
		tex->textureTiling = VK_IMAGE_TILING_OPTIMAL;
		tex->textureUsage = image->imageUsage;
		tex->textureLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		tex->setup();
		image->textures[i] = tex;
	}

	image->imageExtent = VkExtent2D(width, height);

	image->colourTex = new Texture;
	image->colourTex->texWidth = width;
	image->colourTex->texHeight = height;
	image->colourTex->mipLevels = 1;
	image->colourTex->textureSamples = Core::maxSamples();
	image->colourTex->textureFormat = image->imageFormat;
	image->colourTex->textureTiling = VK_IMAGE_TILING_OPTIMAL;
	image->colourTex->textureUsage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	image->colourTex->textureMemFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	image->colourTex->textureLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image->colourTex->setup();

	image->imageFrameBuffers.resize(image->textures.size());
	for (size_t i = 0; i < image->textures.size(); i++) {
		array<VkImageView, 3> attachments = {
			image->colourTex->textureImageView,
			depthTex->textureImageView,
			image->textures[i]->textureImageView
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = *image->boundRenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = width;
		framebufferInfo.height = height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &image->imageFrameBuffers[i]) != VK_SUCCESS) {
			throw runtime_error("failed to create framebuffer!");
		}
	}
}

uint32_t Engine::getRenderTarget() {
	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();

		vkResetFences(device, 1, &inFlightFences[currentFrame]);
		vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		return imageIndex;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw runtime_error("failed to acquire swap chain image!");
	}

	vkResetFences(device, 1, &inFlightFences[currentFrame]);
	vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);

	return imageIndex;
}

void Engine::beginRenderPass(VkCommandBuffer commandBuffer, GraphicsPass* graphicsPass, drawImage* target, uint32_t imageIndex, glm::vec3 backgroundColour) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	beginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw runtime_error("failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = graphicsPass->renderPass;
	renderPassInfo.framebuffer = target->imageFrameBuffers[imageIndex];

	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent = target->imageExtent;

	array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {backgroundColour.r, backgroundColour.g, backgroundColour.b, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(target->imageExtent.width);
	viewport.height = static_cast<float>(target->imageExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = target->imageExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Engine::drawObject(VkCommandBuffer commandBuffer, VkBuffer vertexBuffer, VkBuffer indexBuffer, VkPipelineLayout pipelineLayout, VkDescriptorSet descriptorSet, uint32_t drawCount) {
	VkBuffer vertexBuffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	vkCmdDrawIndexed(commandBuffer, drawCount, 1, 0, 0, 0);
}

VkResult Engine::submitAndPresentFrame(uint32_t imageIndex) {
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[imageIndex] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(graphicsQueueObj->queue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkSwapchainKHR swapChains[] = { swapChain };

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	VkResult result = vkQueuePresentKHR(presentQueueObj->queue, &presentInfo);

	return result;
}

drawImage Engine::createDrawImage(uint32_t width, int32_t height, VkFormat format, VkImageUsageFlags flags, VkRenderPass boundRenderPass) const {
	drawImage newDrawImage;
	newDrawImage.imageExtent = VkExtent2D(width, height);
	newDrawImage.imageFormat = format;
	newDrawImage.imageUsage = flags;
	newDrawImage.boundRenderPass = &boundRenderPass;

	newDrawImage.textures.resize(imageCount);

	for (size_t i = 0; i != imageCount; i++) {
		Texture* tex = new Texture;
		tex->texWidth = width;
		tex->texHeight = height;
		tex->mipLevels = 1;
		tex->textureSamples = VK_SAMPLE_COUNT_1_BIT;
		tex->textureMemFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		tex->textureFormat = format;
		tex->textureTiling = VK_IMAGE_TILING_OPTIMAL;
		tex->textureUsage = flags;
		tex->textureLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		tex->setup();
		newDrawImage.textures[i] = tex;
	}

	newDrawImage.colourTex = new Texture;
	newDrawImage.colourTex->texWidth = width;
	newDrawImage.colourTex->texHeight = height;
	newDrawImage.colourTex->mipLevels = 1;
	newDrawImage.colourTex->textureSamples = Core::maxSamples();
	newDrawImage.colourTex->textureFormat = format;
	newDrawImage.colourTex->textureTiling = VK_IMAGE_TILING_OPTIMAL;
	newDrawImage.colourTex->textureUsage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	newDrawImage.colourTex->textureMemFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	newDrawImage.colourTex->textureLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	newDrawImage.colourTex->setup();

	newDrawImage.imageFrameBuffers.resize(newDrawImage.textures.size());
	for (size_t i = 0; i < newDrawImage.textures.size(); i++) {
		array<VkImageView, 3> attachments = {
			newDrawImage.colourTex->textureImageView,
			depthTex->textureImageView,
			newDrawImage.textures[i]->textureImageView
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = boundRenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = width;
		framebufferInfo.height = height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &newDrawImage.imageFrameBuffers[i]) != VK_SUCCESS) {
			throw runtime_error("failed to create framebuffer!");
		}
	}

	return newDrawImage;
}

void Engine::copyImageToSwapchain(VkCommandBuffer commandBuffer, VkImage image, VkExtent2D extent, uint32_t imageIndex) {
	bool supportsBlit = true;

	VkFormatProperties formatProps{};

	swapChainTextures[imageIndex]->transitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	if (supportsBlit)
	{
		VkOffset3D srcBlitSize{};
		srcBlitSize.x = extent.width;
		srcBlitSize.y = extent.height;
		srcBlitSize.z = 1;
		VkOffset3D dstBlitSize{};
		dstBlitSize.x = swapChainExtent.width;
		dstBlitSize.y = swapChainExtent.height;
		dstBlitSize.z = 1;
		VkImageBlit imageBlitRegion{};
		imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.srcSubresource.layerCount = 1;
		imageBlitRegion.srcOffsets[1] = srcBlitSize;
		imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.dstSubresource.layerCount = 1;
		imageBlitRegion.dstOffsets[1] = dstBlitSize;

		vkCmdBlitImage(
			commandBuffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			swapChainTextures[imageIndex]->textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageBlitRegion,
			VK_FILTER_NEAREST);
	}
	else
	{
		throw runtime_error("Blitting cannot be performed");
	}

	swapChainTextures[imageIndex]->transitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

void Engine::copyImageToSwapchain(VkCommandBuffer commandBuffer, drawImage* image, uint32_t imageIndex) {
	copyImageToSwapchain(commandBuffer, image->textures[imageIndex]->textureImage, image->imageExtent, imageIndex);
}