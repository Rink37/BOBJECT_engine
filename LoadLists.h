#ifndef LOADLIST
#define LOADLIST

#include"Bobject_Engine.h"
#include"Textures.h"
#include"Materials.cpp"

struct LoadList {
	Texture* getPtr(Texture* tex) {
		textures.emplace_back(tex);
		return textures[textures.size() - 1].get();
	}

	Material* getPtr(Material* tex) {
		materials.emplace_back(tex);
		return materials[materials.size() - 1].get();
	}

private:
	std::vector<std::unique_ptr<Texture>> textures;
	std::vector<std::unique_ptr<Material>> materials;
};

#endif