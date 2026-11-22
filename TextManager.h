#ifndef TEXT_MANAGER
#define TEXT_MANAGER

#include"Textures.h"
#include"include/UIText.h"
#include"Materials.h"
#include"Meshes.h"

#include"Bobject_VKMem.h"

#define SPACE_CHAR 32
#define TAB_CHAR 9
#define NEWLINE_CHAR 10

class font {
public:
	font();

	void getCorners(int, float&, float&, float&, float&) const;

	void getAdvanceWidth(int, float&);

	const float getSqAxisRatio() const {
		return static_cast<float>(cellHeight) / static_cast<float>(cellWidth);
	}

	void cleanup() {
		if (!cleaned){
			fontAtlas->cleanup();
			fontMat->cleanup();
			cleaned = true;
		}
	}
	
	Texture* fontAtlas;
	Material* fontMat;
private:
	int colCount = 11;
	int rowCount = 9;

	int cellWidth = 31;
	int cellHeight = 35;

	bool cleaned = false;
};

class fontMesh : public Mesh {
public:
	void setVisibility(bool state) {
		isVisible = state;
	}

	void cleanup() {
		if (cleaned) {
			return;
		}

		VkDevice device = Core::getDevice();

		vkDeviceWaitIdle(device);

		VKMem::freeBuffer(indexBuffer, indexBufferMemory);
		VKMem::freeBuffer(vertexBuffer, vertexBufferMemory);

		delete vBuffer;
		vBuffer = nullptr;

		delete iBuffer;
		iBuffer = nullptr;

		cleaned = true;
	}

	void* vBuffer = nullptr;
	void* iBuffer = nullptr;
	VkDeviceSize vBuffSize = 0;
	VkDeviceSize iBuffSize = 0;

	void createVertexBuffer();
	void updateVertexBuffer();

	void createIndexBuffer();
	void updateIndexBuffer();

	bool isVisible = true;

	uint8_t charCount = 0; // Defines the number of non-blank chars; used to allocate the correct max buffer size.
};

#endif