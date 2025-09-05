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

	Texture* replacePtr(Texture* tex, std::string name) {
		if (!checkForTexture(name)) {
			return getPtr(tex, name);
		}
		textures[textureMap.at(name)].get()->cleanup();
		textures[textureMap.at(name)] = std::make_unique<Texture>(*tex);
		return textures[textureMap.at(name)].get();
	}

	void cleanup(std::string name) {
		if (checkForTexture(name)) {
			textures[textureMap.at(name)].get()->cleanup();
		}
		if (checkForMaterial(name)) {
			materials[materialMap.at(name)].get()->cleanup();
		}
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

	Material* replacePtr(Material* tex, std::string name) {
		if (!checkForMaterial(name)) {
			return getPtr(tex, name);
		}
		materials[materialMap.at(name)].get()->cleanupDescriptor();
		materials[materialMap.at(name)] = std::make_unique<Material>(*tex);
		return materials[materialMap.at(name)].get();
	}

	Texture* getTexture(std::string name) {
		if (checkForTexture(name)) {
			return textures[textureMap.at(name)].get();
		}
		return nullptr;
	}

	void deleteTexture(std::string name) {
		if (!checkForTexture(name)) {
			return;
		}
		int index = textureMap.at(name);
		textures[index]->cleanup();
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
			return materials[materialMap.at(name)].get();
		}
		return nullptr;
	}

	void deleteMaterial (std::string name) {
		if (!checkForMaterial(name)) {
			return;
		}
		int index = materialMap.at(name);
		materials[index]->cleanupDescriptor();
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
			textures[i].get()->cleanup();
		}
		textures.clear();
		for (size_t i = 0; i != materials.size(); i++) {
			materials[i].get()->cleanupDescriptor();
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