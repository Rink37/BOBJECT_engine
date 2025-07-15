#ifndef MATERIALS
#define MATERIALS

#include"Bobject_Engine.h"
#include"Textures.h"


class Material {
public:
	Material(Texture* defaultTex) {
		textures.push_back(defaultTex);
		createMaterial();
	}

	Material(Texture* defaultTex, Texture* normalTex) {
		textures.push_back(defaultTex);
		textures.push_back(normalTex);
		createMaterial();
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
		vkDestroyDescriptorPool(Engine::get()->device, this->descriptorPool, nullptr);
		cleaned = true;
	}

	std::vector<Texture*> textures;

	VkDescriptorPool descriptorPool;
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
