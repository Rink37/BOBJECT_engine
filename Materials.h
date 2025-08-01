#ifndef MATERIALS
#define MATERIALS

#include"Bobject_Engine.h"
#include"Textures.h"


class Material {
public:
	Material() = default;

	Material(Texture* defaultTex) {
		textures.push_back(defaultTex);
		createMaterial();
	}

	void init(Texture* defaultTex) {
		if (!cleaned) {
			cleanupDescriptor();
			cleaned = false;
			textures.clear();
		}
		textures.push_back(defaultTex);
		createMaterial();
	}

	void init(Texture* defaultTex, Texture* normalTex) {
		if (!cleaned) {
			cleanupDescriptor();
			cleaned = false;
			textures.clear();
		}
		textures.push_back(defaultTex);
		textures.push_back(normalTex);
		createMaterial();
	}

	void init(Material &matPtr) {
		if (!cleaned) {
			cleanupDescriptor();
			cleaned = false;
			textures.clear();
		}
		for (Texture* initTex : matPtr.textures) {
			textures.push_back(initTex);
		}
		createMaterial();
	}

	~Material() {
		cleanupDescriptor();
	}

	void cleanup(){
		if (cleaned) {
			return;
		}
		for (Texture* tex : textures) {
			if (tex != webcamTexture::get() && tex != nullptr) {
				tex->cleanup();
			}
		}
		vkDestroyDescriptorPool(Engine::get()->device, this->descriptorPool, nullptr);
		cleaned = true;
	}

	void cleanupDescriptor() {
		if (cleaned) {
			return;
		}
		vkDestroyDescriptorPool(Engine::get()->device, this->descriptorPool, nullptr);
		cleaned = true;
	}

	std::vector<Texture*> textures;

	VkDescriptorPool descriptorPool = nullptr;
	std::vector<VkDescriptorSet> descriptorSets;

	void createMaterial() {
		createDescriptorPool();
		createDescriptorSets();
	}

	void createDescriptorPool();
	void createDescriptorSets();

	bool cleaned = false;
};

#endif
