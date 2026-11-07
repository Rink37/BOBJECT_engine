#ifndef MATERIALS
#define MATERIALS

#include"Textures.h"

class Material {
public:
	Material() {
		pass = &Engine::get()->defaultPass;
		buffers.push_back(std::pair<std::vector<VkBuffer>, uint32_t>{Engine::get()->uniformBuffers, sizeof(UniformBufferObject)});
	};

	Material(Texture* defaultTex, std::string name) {
		shaderName = name;
		if (name == std::string("UIGray")) {
			buffers.push_back(std::pair<std::vector<VkBuffer>, uint32_t>{std::vector<VkBuffer>{Engine::get()->colourBuffer}, sizeof(ColourSchemeObject)});
		}
		else {
			buffers.push_back(std::pair<std::vector<VkBuffer>, uint32_t>{Engine::get()->uniformBuffers, sizeof(UniformBufferObject)});
		}
		textures.push_back(defaultTex);
		pass = &Engine::get()->defaultPass;
		createMaterial();
	}

	Material(Texture* defaultTex, std::string name, GraphicsPass* graphicsPass) {
		shaderName = name;
		if (name == std::string("UIGray")) {
			buffers.push_back(std::pair<std::vector<VkBuffer>, uint32_t>{std::vector<VkBuffer>{Engine::get()->colourBuffer}, sizeof(ColourSchemeObject)});
		}
		else {
			buffers.push_back(std::pair<std::vector<VkBuffer>, uint32_t>{Engine::get()->uniformBuffers, sizeof(UniformBufferObject)});
		}
		textures.push_back(defaultTex);
		pass = graphicsPass;
		createMaterial();
	}

	Material(std::vector<Texture*> inTextures, std::string targetShader, GraphicsPass* graphicsPass, bool isUI = false) {
		textures = inTextures;
		shaderName = targetShader;
		pass = graphicsPass;
		if (isUI) {
			buffers.push_back(std::pair<std::vector<VkBuffer>, uint32_t>{std::vector<VkBuffer>{Engine::get()->colourBuffer}, sizeof(ColourSchemeObject)});
		}
		else {
			buffers.push_back(std::pair<std::vector<VkBuffer>, uint32_t>{Engine::get()->uniformBuffers, sizeof(UniformBufferObject)});
		}
		createMaterial();
	}

	Material(std::vector<Texture*> inTextures, std::vector<std::pair<std::vector<VkBuffer>, uint32_t>> inBuffers, std::string targetShader, GraphicsPass* graphicsPass) {
		textures = inTextures;
		buffers = inBuffers;
		shaderName = targetShader;
		pass = graphicsPass;
		createMaterial();
	}

	void init(bool clearTextures = true) {
		if (!cleaned) {
			cleanup();
			if (clearTextures) {
				textures.clear();
			}
		}
		createMaterial();
	}

	void init(Texture* defaultTex) {
		if (!cleaned) {
			cleanup();
			textures.clear();
		}
		textures.push_back(defaultTex);
		createMaterial();
	}

	void init(Texture* defaultTex, Texture* normalTex, std::string targetShader) {
		if (!cleaned) {
			cleanup();
			textures.clear();
		}
		shaderName = targetShader;
		textures.push_back(defaultTex);
		textures.push_back(normalTex);
		createMaterial();
	}

	void init(Material &matPtr) {
		if (!cleaned) {
			cleanup();
			textures.clear();
			buffers.clear();
		}
		for (Texture* initTex : matPtr.textures) {
			textures.push_back(initTex);
		}
		for (std::pair<std::vector<VkBuffer>, uint32_t> buffer : matPtr.buffers) {
			buffers.push_back(buffer);
		}
		shaderName = matPtr.shaderName;
		pass = matPtr.pass;
		createMaterial();
	}

	void cleanup() {
		if (cleaned || this->descriptorPool == nullptr) {
			return;
		}
		vkDeviceWaitIdle(Engine::get()->device);
		vkDestroyDescriptorPool(Engine::get()->device, this->descriptorPool, nullptr);
		this->descriptorPool = nullptr;
	}

	std::vector<Texture*> textures = {};
	std::vector<std::pair<std::vector<VkBuffer>, uint32_t>> buffers = {};

	std::string shaderName = "BF";

	VkDescriptorSetLayout descriptorSetLayout = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
	uint32_t pipelineLayoutIndex = 0;
	VkDescriptorPool descriptorPool = nullptr;
	std::vector<VkDescriptorSet> descriptorSets = {};
	GraphicsPass* pass = nullptr;

	void createMaterial() {
		createDescriptorPool(shaderName, pass);
		createDescriptorSets(shaderName, pass);
		cleaned = false;
	}

	void createDescriptorPool(std::string, GraphicsPass*);
	void createDescriptorSets(std::string, GraphicsPass*);

	bool cleaned = true;

	~Material() {
		cleanup();
	}
};

class MaterialTemplate {
public:
	MaterialTemplate(std::string sName, GraphicsPass* bPass) {
		shaderName = sName;
		boundPass = bPass;
		
		std::vector<shaderIOValue>* IOvals = &boundPass->shaderDatas[boundPass->pipelineMap.at(shaderName)].IO;

		for (shaderIOValue IOval : *IOvals) {
			if (IOval.type == 0) {
				textureMap.insert({ IOval.name, nullptr });
				textureOrder.push_back(IOval.name);
			}
		}
	}

	std::vector<std::string> listChannels() const {
		return textureOrder;
	}

	void setTexture(std::string textureChannel, Texture* newTexture) {
		if (textureMap.count(textureChannel) == 0) {
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
		if (textureOrder.size() > 0) {
			for (std::string channel : textureOrder) {
				materialTextures.push_back(textureMap.at(channel));
			}
		}

		Material* newMat = new Material(materialTextures, shaderName, boundPass);
		return newMat;
	}

	Material* createFlatMaterial() {
		std::vector<Texture*> materialTextures{};
		if (textureOrder.size() > 0) {
			for (std::string channel : textureOrder) {
				materialTextures.push_back(textureMap.at(channel));
			}
		}
		Material* flatMat = new Material(materialTextures, "Flat", boundPass);
		return flatMat;
	}

	Material* updateMaterial(Material* mat) {
		bool isValid = true;
		for (auto mapElem : textureMap) {
			if (mapElem.second == nullptr) {
				isValid = false;
				break;
			}
		}
		if (mat->textures.size() < textureOrder.size()) {
			isValid = false;
		}
		if (!isValid) {
			throw std::runtime_error("Failed to create new material; insufficient textures available");
		}
		std::vector<Texture*> materialTextures{};
		int index = 0;
		for (std::string channel : textureOrder) {
			mat->textures[index] = textureMap.at(channel);
		}
		mat->init(false);
		return mat;
	}

	std::string shaderName = "";
	GraphicsPass* boundPass;

	std::vector<std::string> textureOrder{};
	std::map<std::string, Texture*> textureMap{};
};

#endif
