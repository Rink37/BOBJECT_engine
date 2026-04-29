#ifndef TEXT_MANAGER
#define TEXT_MANAGER

#include"Textures.h"
#include"include/UIText.h"
#include"Materials.h"
#include"Meshes.h"

class font {
public:
	font();

	void getCorners(int, float&, float&, float&, float&);

	void getAdvanceWidth(int, float&);

	const float getSqAxisRatio() {
		return static_cast<float>(cellHeight) / static_cast<float>(cellWidth);
	}

	void cleanup() {
		fontAtlas->cleanup();
		fontMat->cleanupDescriptor();
	}
	
	Texture* fontAtlas;
	Material* fontMat;
private:
	int colCount = 11;
	int rowCount = 9;

	int cellWidth = 46;
	int cellHeight = 53;
};

class fontMesh : public Mesh {
public:
	fontMesh(int character, font* meshFont) {
		indices = { 0, 3, 2, 2, 1, 0 };
		fontRef = meshFont;
		unicodeCharacter = character;
		sqAxisRatio = fontRef->getSqAxisRatio();
		fontRef->getAdvanceWidth(unicodeCharacter, advanceWidth);
	}

	void* vBuffer = nullptr;

	void UpdateVertices(float, float, float, float);

	void createVertexBuffer();
	void updateVertexBuffer();

	int unicodeCharacter = 0;
	float sqAxisRatio = 0.0f;
	float advanceWidth = 1.0f;

	bool isVisible = true;

	font* fontRef;
};

#endif