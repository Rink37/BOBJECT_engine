#include"TextManager.h"

#include"include/coolvetica_sdf.h"
#include"include/coolvetica_msdf.h"

font::font() {
	imageData fontData = COOLVETICA_MSDF;
	fontAtlas = new imageTexture(&fontData, VK_FORMAT_R8G8B8A8_UNORM);
	fontMat = new Material(std::vector<Texture*>{fontAtlas}, "UIText", &Engine::get()->defaultPass, true);
}

void font::getCorners(int character, float& left, float& right, float& top, float& bottom) const {
	int minCharacter = 33;

	character -= 33;

	int colIndex = character % colCount;
	int rowIndex = character / colCount;

	int itop, ibottom, ileft, iright;

	itop = rowIndex * cellHeight;
	ileft = colIndex * cellWidth;

	ibottom = itop + cellHeight;
	iright = ileft + cellWidth;

	left = static_cast<float>(ileft) / static_cast<float>(fontAtlas->texWidth);
	right = static_cast<float>(iright) / static_cast<float>(fontAtlas->texWidth);

	top = static_cast<float>(itop) / static_cast<float>(fontAtlas->texHeight);
	bottom = static_cast<float>(ibottom) / static_cast<float>(fontAtlas->texHeight);
}

void font::getAdvanceWidth(int character, float& width) {
	switch (character) {
	case (TAB_CHAR):
		width = coolvetica_sdfAdvances[0] * 6.0f;
		break;
	case (SPACE_CHAR):
		width = coolvetica_sdfAdvances[0];
		break;
	case (NEWLINE_CHAR):
		width = 0.0f;
		break;
	default:
		character -= 33;
		width = coolvetica_sdfAdvances[character];
		break;
	}
}

void fontMesh::createVertexBuffer() {
	vBuffSize = sizeof(vertices[0]) * charCount * 4;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VKMem::createBuffer(vBuffSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
	VKMem::copyDataToBuffer(stagingBuffer, stagingBufferMemory, vertices.data(), vBuffSize);
	VKMem::createBuffer(vBuffSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, vertexBuffer, vertexBufferMemory);
	vkMapMemory(Core::getDevice(), vertexBufferMemory, 0, vBuffSize, 0, &vBuffer);
	VKMem::freeBuffer(stagingBuffer, stagingBufferMemory);
}

void fontMesh::updateVertexBuffer() {
	memcpy(vBuffer, vertices.data(), (size_t)vBuffSize);
}

void fontMesh::createIndexBuffer() {
	iBuffSize = sizeof(indices[0]) * charCount * 6;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VKMem::createBuffer(iBuffSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
	VKMem::copyDataToBuffer(stagingBuffer, stagingBufferMemory, indices.data(), iBuffSize);
	VKMem::createBuffer(iBuffSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, indexBuffer, indexBufferMemory);
	vkMapMemory(Core::getDevice(), indexBufferMemory, 0, iBuffSize, 0, &iBuffer);
	VKMem::freeBuffer(stagingBuffer, stagingBufferMemory);
}

void fontMesh::updateIndexBuffer() {
	memcpy(iBuffer, indices.data(), (size_t)iBuffSize);
}