#ifndef MATERIALS
#define MATERIALS

#include"Bobject_Engine.h"
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

	//Material(Texture* defaultTex, bool isUI) {
	//	isUIMat = isUI;
	//	textures.push_back(defaultTex);
	//	createMaterial();
	//}

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
		vkDestroyDescriptorPool(Engine::get()->device, this->descriptorPool, nullptr);
		this->descriptorPool = nullptr;
	}

	void cleanupDescriptor() {
		if (cleaned || this->descriptorPool == nullptr) {
			return;
		}
		vkDestroyDescriptorPool(Engine::get()->device, this->descriptorPool, nullptr);
		this->descriptorPool = nullptr;
	}

	std::vector<Texture*> textures = {};

	VkDescriptorPool descriptorPool = nullptr;
	std::vector<VkDescriptorSet> descriptorSets = {};

	void createMaterial() {
		createDescriptorPool();
		createDescriptorSets();
		cleaned = false;
	}

	void createDescriptorPool();
	void createDescriptorSets();

	bool cleaned = true;
	bool isUIMat = false;

	~Material() {
		cleanupDescriptor();
	}
};


#endif
