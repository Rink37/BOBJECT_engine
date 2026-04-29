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
	}

	void* vBuffer = nullptr;

	void UpdateVertices(float, float, float, float);

	void createVertexBuffer();
	void updateVertexBuffer();

	int unicodeCharacter = 0;

	font* fontRef;
};

#endif