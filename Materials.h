#ifndef MATERIALS
#define MATERIALS

#include"Textures.h"

class Material {
public:
	Material() = default;

	Material(Texture* defaultTex) {
		if (defaultTex->textureFormat == VK_FORMAT_R8_UNORM) {
			isUIMat = true;
		}
		textures.push_back(defaultTex);
		createMaterial();
	}

	Material(std::vector<Texture*> inTextures, std::string targetShader, GraphicsPass* graphicsPass) {
		if (textures[0]->textureFormat == VK_FORMAT_R8_UNORM) {
			isUIMat = true;
		}
		for (Texture* tex : inTextures) {
			textures.push_back(tex);
		}
		createMaterial(targetShader, graphicsPass);
	}

	void init() {
		if (!cleaned) {
			cleanupDescriptor();
			textures.clear();
		}
		createMaterial();
	}

	void init(Texture* defaultTex) {
		if (!cleaned) {
			cleanupDescriptor();
			textures.clear();
		}
		textures.push_back(defaultTex);
		createMaterial();
	}

	void init(Texture* defaultTex, bool isUI) {
		isUIMat = isUI;
		if (!cleaned) {
			cleanupDescriptor();
			textures.clear();
		}
		textures.push_back(defaultTex);
		createMaterial();
	}

	void init(Texture* defaultTex, Texture* normalTex) {
		if (!cleaned) {
			cleanupDescriptor();
			textures.clear();
		}
		textures.push_back(defaultTex);
		textures.push_back(normalTex);
		createMaterial();
	}

	void init(Material &matPtr) {
		if (!cleaned) {
			cleanupDescriptor();
			textures.clear();
		}
		for (Texture* initTex : matPtr.textures) {
			textures.push_back(initTex);
		}
		createMaterial();
	}

	void cleanup(){
		if (cleaned || this->descriptorPool == nullptr) {
			return;
		}
		for (Texture* tex : textures) {
			if (tex != webcamTexture::get() && tex != nullptr) {
				tex->cleanup();
			}
		}
		vkDeviceWaitIdle(Engine::get()->device);
		vkDestroyDescriptorPool(Engine::get()->device, this->descriptorPool, nullptr);
		this->descriptorPool = nullptr;
	}

	void cleanupDescriptor() {
		if (cleaned || this->descriptorPool == nullptr) {
			return;
		}
		vkDeviceWaitIdle(Engine::get()->device);
		vkDestroyDescriptorPool(Engine::get()->device, this->descriptorPool, nullptr);
		this->descriptorPool = nullptr;
	}

	std::vector<Texture*> textures = {};

	VkDescriptorSetLayout descriptorSetLayout = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
	uint32_t pipelineLayoutIndex = 0;
	VkDescriptorPool descriptorPool = nullptr;
	std::vector<VkDescriptorSet> descriptorSets = {};

	void createMaterial() {
		createDescriptorPool();
		createDescriptorSets();
		cleaned = false;
	}

	void createMaterial(std::string shaderName, GraphicsPass* currentPass) {
		createDescriptorPool(shaderName, currentPass);
		createDescriptorSets(shaderName, currentPass);
		cleaned = false;
	}

	void createDescriptorPool();
	void createDescriptorPool(std::string, GraphicsPass*);
	
	void createDescriptorSets();
	void createDescriptorSets(std::string, GraphicsPass*);

	bool cleaned = true;
	bool isUIMat = false;

	~Material() {
		cleanupDescriptor();
	}
};

class MaterialTemplate {
public:
	MaterialTemplate(std::string sName, GraphicsPass* bPass) {
		shaderName = sName;
		boundPass = bPass;
		
		std::vector<shaderIOValue>* IOvals = &boundPass->shaderDatas[boundPass->pipelineMap.at(shaderName)].IO;

		for (shaderIOValue IOval : *IOvals) {
			textureMap.insert({ IOval.name, nullptr });
			textureOrder.push_back(IOval.name);
		}
	}

	std::vector<std::string> listChannels() {
		std::vector<std::string> channels{};
		for (auto mapElem : textureMap) {
			channels.push_back(mapElem.first);
		}
		return channels;
	}

	void setTexture(std::string textureChannel, Texture* newTexture) {
		if (textureMap.count(textureChannel) == 0) {
			std::cout << "Invalid texture channel" << std::endl;
			return;
		}
		textureMap.at(textureChannel) = newTexture;
	}

	Material* createMaterial() {
		bool isValid = true;
		for (auto mapElem : textureMap) {
			if (mapElem.second == nullptr) {
				isValid = false;
				break;
			}
		}
		if (!isValid) {
			throw std::runtime_error("Failed to create new material; insufficient textures available");
		}
		std::vector<Texture*> materialTextures{};
		for (std::string channel : textureOrder) {
			materialTextures.push_back(textureMap.at(channel));
		}
		Material* newMat = new Material(materialTextures, shaderName, boundPass);
		return newMat;
	}

	std::string shaderName;
	GraphicsPass* boundPass;

	std::vector<std::string> textureOrder{};
	std::map<std::string, Texture*> textureMap{};
};

#endif
