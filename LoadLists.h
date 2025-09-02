#ifndef LOADLIST
#define LOADLIST

#include"Textures.h"
#include"Materials.h"

struct LoadList {
	Texture* getPtr(Texture* tex, std::string name) {
		if (checkForTexture(name)) {
			tex->cleanup();
			delete tex;
			return textures[textureMap.at(name)].get();
		}
		textures.emplace_back(tex);
		textureMap.insert({ name, static_cast<int>(textures.size() - 1) });
		return textures[textures.size() - 1].get();
	}

	Material* getPtr(Material* tex, std::string name) {
		if (checkForMaterial(name)) {
			tex->cleanupDescriptor();
			delete tex;
			return materials[materialMap.at(name)].get();
		}
		materials.emplace_back(tex);
		materialMap.insert({ name, static_cast<int>(materials.size() - 1) });
		return materials[materials.size() - 1].get();
	}

	Texture* getTexture(std::string name) {
		return textures[textureMap.at(name)].get();
	}

	Material* getMaterial(std::string name) {
		return materials[materialMap.at(name)].get();
	}

	bool checkForTexture(std::string name) {
		return textureMap.find(name) != textureMap.end();
	}

	bool checkForMaterial(std::string name) {
		return materialMap.find(name) != materialMap.end();
	}

	void empty() {
		for (size_t i = 0; i != textures.size(); i++) {
			//std::cout << textures[i].get() << std::endl;
			textures[i].get()->cleanup();
		}
		textures.clear();
		for (size_t i = 0; i != materials.size(); i++) {
			//std::cout << materials[i].get() << std::endl;
			materials[i].get()->cleanupDescriptor();
		}
		materials.clear();
		//for (auto& item : textureMap) {
		//	std::cout << item.first << " " << item.second << std::endl;
		//}
		textureMap.clear();
		//for (auto& item : materialMap) {
		//	std::cout << item.first << " " << item.second << std::endl;
		//}
		materialMap.clear();
	}

	//~LoadList() {
	//	if (textures.size() > 0) {
	//		textureMap.clear();
	//		textures.clear();
	//	}
	//	if (materials.size() > 0) {
	//		materialMap.clear();
	//		materials.clear();
	//	}
	//}

private:
	std::map<std::string, int> textureMap = {};
	std::map<std::string, int> materialMap = {};

	std::vector<std::unique_ptr<Texture>> textures = {};
	std::vector<std::unique_ptr<Material>> materials = {};
};

#endif