#ifndef LOADLIST
#define LOADLIST

#include"Textures.h"
#include"Materials.h"
#include"TextManager.h"

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
		std::vector<int> replaceIndices{};
		if (checkForTexture(name)) {
			if (textureAssociator.count(name) > 0) {
				for (std::string matName : textureAssociator.at(name)) {
					int index = materialMap.at(matName);
					auto it = find(materials[index].get()->textures.begin(), materials[index].get()->textures.end(), getTexture(name));
					replaceIndices.push_back(it - materials[index].get()->textures.begin());
				}
			}
			deleteTexture(name);
		}
		Texture* replacedTex = getPtr(tex, name);
		if (textureAssociator.count(name) > 0) {
			for (int i = 0; i != textureAssociator.at(name).size(); i++) {

				std::string matName = textureAssociator.at(name)[i];
				std::cout << "Updating material " << matName << " since it contains texture " << name << " at index " << replaceIndices[i] << std::endl;
				int index = materialMap.at(matName);
				materials[index].get()->textures[replaceIndices[i]] = replacedTex;
				materials[index].get()->init(false);
			}
		}
		return replacedTex;
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
		for (auto elem : textureMap) {
			auto it = find(tex->textures.begin(), tex->textures.end(), textures[elem.second].get());
			if (it != tex->textures.end()) {
				if (textureAssociator.count(elem.first) == 0) {
					textureAssociator.insert({ elem.first, std::vector<std::string>{name} });
				}
				else {
					textureAssociator.at(elem.first).push_back(name);
				}
			}
		}
		auto it = find(tex->textures.begin(), tex->textures.end(), webcamTexture::get());
		if (it != tex->textures.end()) {
			if (textureAssociator.count("Webcam View") == 0) {
				textureAssociator.insert({ "Webcam View", std::vector<std::string>{name}});
			}
			else {
				textureAssociator.at("Webcam View").push_back(name);
			}
		}
		materials.emplace_back(tex);
		materialMap.insert({ name, static_cast<int>(materials.size() - 1) });
		return materials.at(materials.size() - 1).get();
	}

	Material* getPtr(imageData* imgData, std::string name) {
		if (checkForMaterial(name + "Mat")) {
			return findMatPtr(name + "Mat");
		}
		else {
			Texture* tex = nullptr;
			if (checkForTexture(name + "Tex")) {
				tex = findTexPtr(name + "Tex");
			}
			else {
				tex = getPtr(new imageTexture(imgData), name + "Tex");
			}
			return getPtr(new Material(tex), name + "Mat");
		}
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
		std::cout << "Material " << name << " not found" << std::endl;
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

	Texture* findTexPtr(std::string name) {
		if (checkForTexture(name)) {
			return textures.at(textureMap.at(name)).get();
		}
		return nullptr;
	}

	Material* findMatPtr(std::string name) {
		if (checkForMaterial(name)) {
			return materials.at(materialMap.at(name)).get();
		}
		return nullptr;
	}

	void empty() {
		textures.clear(); // destructor replicates above operation
		materials.clear(); // destructor replicates above operation

		textureMap.clear();
		materialMap.clear();

		if (LLFont != nullptr) {
			LLFont->cleanup();
			delete LLFont;
		}
	}

	void listTextures(std::vector<std::string>& list) {
		for (auto elem : textureMap) {
			list.push_back(elem.first);
		}
	}

	void listTexturesInMat(std::string matName, std::vector<std::string>& list) {
		std::vector<Texture*> mTextures = getMaterial(matName)->textures;
		list.clear();
		for (uint32_t i = 0; i != mTextures.size(); i++) {
			list.push_back("");
		}
		for (auto elem : textureMap) {
			Texture* tex = textures[elem.second].get();
			auto it = find(mTextures.begin(), mTextures.end(), tex);
			if (it != mTextures.end()) {
				list.at(it - mTextures.begin()) = elem.first;
			}
		}
		auto it = find(mTextures.begin(), mTextures.end(), webcamTexture::get());
		if (it != mTextures.end()) {
			list.at(it - mTextures.begin()) = "Webcam View";
		}
	}

	void listMaterials(std::vector <std::string>& list) {
		for (auto elem : materialMap) {
			list.push_back(elem.first);
		}
	}

	void listMaterialsWithTex(std::string texName, std::vector<std::string>& list) {
		if (textureAssociator.count(texName) == 0) {
			std::cout << "No materials use texture " << texName << std::endl;
			return;
		}
		list = textureAssociator.at(texName);
	}

	font* getFont() {
		if (LLFont == nullptr) {
			LLFont = new font();
		}
		return LLFont;
	}

private:
	std::map<std::string, int> textureMap = {};
	std::map<std::string, int> materialMap = {};

	std::vector<std::unique_ptr<Texture>> textures = {};
	std::vector<std::unique_ptr<Material>> materials = {};

	std::map<std::string, std::vector<std::string>> textureAssociator{};

	font* LLFont = nullptr;
};

#endif