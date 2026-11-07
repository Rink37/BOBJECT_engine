#ifndef NORMALGEN
#define NORMALGEN

#include"Bobject_Engine.h"
#include"LoadLists.h"
#include"Meshes.h"
#include"Textures.h"
#include"ImageProcessor.h"
#include"include/ShaderDataType.h"
#include"include/NormalGenerator.h"

#define DEFAULTMAPDIM 1024
#define MAP_COLOUR_FORMAT VK_FORMAT_R8G8B8A8_UNORM

class NormalGen {
public:
	NormalGen(LoadList* assets) {
		loadList = assets;
	}

	Texture* refTex = nullptr;
	textureWriter* OStexGenerator = nullptr;

	Texture* refOS = nullptr;
	textureWriter* OStoTSConverter = nullptr;
	
	Texture* refTS = nullptr;
	textureWriter* TStoOSConverter = nullptr;

	void setupOSExtractor() {
		prepGenerateOSMap();
	};

	void cleanupGenOS();

	VkCommandBuffer drawOSMap(VkCommandBuffer, Mesh*);

	void copyOSImage(Texture*);
	void copyTSImage(Texture*);

	void setupTSConverter() {
		prepareTSMap();
	};

	void setupOSConverter() {
		prepareOSMap();
	};

	VkCommandBuffer convertOStoTS(VkCommandBuffer, Mesh*);
	VkCommandBuffer convertTStoOS(VkCommandBuffer, Mesh*);

	void cleanupOStoTS();
	void cleanupTStoOS();

private:
	LoadList* loadList = nullptr;

	void prepGenerateOSMap();
	void prepareTSMap();
	void prepareOSMap();	
};

#endif