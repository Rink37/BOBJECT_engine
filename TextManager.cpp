#include"TextManager.h"

#include"include/coolvetica_sdf.h"

font::font() {
	imageData fontData = COOLVETICA_SDF;
	fontAtlas = new imageTexture(&fontData);
	fontMat = new Material(fontAtlas);
}

void font::getCorners(int character, float& left, float& right, float& top, float& bottom) {
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

void fontMesh::UpdateVertices(float xp, float yp, float xsc, float ysc) {
	vertices.clear();

	Vertex vertex{};

	float left, right, top, bottom;
	fontRef->getCorners(unicodeCharacter, left, right, top, bottom);

	std::cout << left << " " << right << " " << top << " " << bottom << std::endl;

	vertex.pos = { -xsc + xp, yp - ysc, 0.0f };
	vertex.normal = { 0.0f, 0.0f, 0.0f };
	vertex.texCoord = { left, top };
	vertices.push_back(vertex);
	vertex.pos = { xsc + xp, yp - ysc, 0.0f };
	vertex.texCoord = { right, top };
	vertices.push_back(vertex);
	vertex.pos = { xsc + xp, yp + ysc, 0.0f };
	vertex.texCoord = { right, bottom };
	vertices.push_back(vertex);
	vertex.pos = { -xsc + xp, yp + ysc, 0.0f };
	vertex.texCoord = { left, bottom };
	vertices.push_back(vertex);

	if (vBuffer == nullptr) {
		setup();
	}
	else {
		updateVertexBuffer();
	}
}

void fontMesh::createVertexBuffer() {

	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	Engine::get()->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(Engine::get()->device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(Engine::get()->device, stagingBufferMemory);

	Engine::get()->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, vertexBuffer, vertexBufferMemory);

	Engine::get()->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	vkMapMemory(Engine::get()->device, vertexBufferMemory, 0, bufferSize, 0, &vBuffer);

	vkDestroyBuffer(Engine::get()->device, stagingBuffer, nullptr);
	vkFreeMemory(Engine::get()->device, stagingBufferMemory, nullptr);
}

void fontMesh::updateVertexBuffer() {
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	memcpy(vBuffer, vertices.data(), (size_t)bufferSize);
}