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

	void cleanup(){
		for (Texture* tex : textures) {
			tex->cleanup();
		}
		vkDestroyDescriptorPool(Engine::get()->device, this->descriptorPool, nullptr);
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
};

#endif
