#ifndef LOADLIST
#define LOADLIST

#include"Textures.h"
#include"Materials.h"

struct LoadList {
	Texture* getPtr(Texture* tex, std::string name) {
		if (checkForTexture(name)) {
			tex->cleanup();
			delete tex;
			return textures.at(textureMap.at(name)).get();
		}
		textures.emplace_back(tex);
		textureMap.insert({ name, static_cast<int>(textures.size() - 1) });
		return textures.at(textures.size() - 1).get();
	}

	Texture* replacePtr(Texture* tex, std::string name) {
		if (checkForTexture(name)) {
			deleteTexture(name);
		}
		return getPtr(tex, name);
	}

	void cleanup(std::string name) {
		if (checkForTexture(name)) {
			textures.at(textureMap.at(name)).get()->cleanup();
		}
		if (checkForMaterial(name)) {
			materials.at(materialMap.at(name)).get()->cleanup();
		}
	}

	Material* getPtr(Material* tex, std::string name) {
		if (checkForMaterial(name)) {
			tex->cleanupDescriptor();
			delete tex;
			return materials.at(materialMap.at(name)).get();
		}
		materials.emplace_back(tex);
		materialMap.insert({ name, static_cast<int>(materials.size() - 1) });
		return materials.at(materials.size() - 1).get();
	}

	Material* replacePtr(Material* tex, std::string name) {
		if (checkForMaterial(name)) {
			deleteMaterial(name);
		}
		return getPtr(tex, name);
	}

	Texture* getTexture(std::string name) {
		if (checkForTexture(name)) {
			return textures.at(textureMap.at(name)).get();
		}
		return nullptr;
	}

	void deleteTexture(std::string name) {
		if (!checkForTexture(name)) {
			return;
		}
		int index = textureMap.at(name);
		textures.at(index)->cleanup();
		textures.erase(textures.begin() + index);
		textureMap.erase(name);
		for (auto const& [key, val] : textureMap) {
			if (val > index) {
				textureMap.at(key)--;
			}
		}
	}

	Material* getMaterial(std::string name) {
		if (checkForMaterial(name)) {
			return materials.at(materialMap.at(name)).get();
		}
		return nullptr;
	}

	void deleteMaterial (std::string name) {
		if (!checkForMaterial(name)) {
			return;
		}
		int index = materialMap.at(name);
		materials.at(index)->cleanupDescriptor();
		materials.erase(materials.begin() + index);
		materialMap.erase(name);
		for (auto const& [key, val] : materialMap) {
			if (val > index) {
				materialMap.at(key)--;
			}
		}
	}

	bool checkForTexture(std::string name) {
		return textureMap.find(name) != textureMap.end();
	}

	bool checkForMaterial(std::string name) {
		return materialMap.find(name) != materialMap.end();
	}

	void empty() {
		for (size_t i = 0; i != textures.size(); i++) {
			textures.at(i).get()->cleanup();
		}
		textures.clear();
		for (size_t i = 0; i != materials.size(); i++) {
			materials.at(i).get()->cleanupDescriptor();
		}
		materials.clear();

		textureMap.clear();
		materialMap.clear();
	}

private:
	std::map<std::string, int> textureMap = {};
	std::map<std::string, int> materialMap = {};

	std::vector<std::unique_ptr<Texture>> textures = {};
	std::vector<std::unique_ptr<Material>> materials = {};
};

#endif