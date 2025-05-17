#include"Meshes.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include"tiny_obj_loader.h"
#include"WindowsFileManager.h"

using namespace std;

void Mesh::createVertexBuffer() {
	Engine* engine = Engine::get();

	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	engine->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(engine->device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(engine->device, stagingBufferMemory);

	engine->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

	engine->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(engine->device, stagingBuffer, nullptr);
	vkFreeMemory(engine->device, stagingBufferMemory, nullptr);
}

void Mesh::createIndexBuffer() {
	Engine* engine = Engine::get();

	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	engine->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(engine->device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(engine->device, stagingBufferMemory);

	engine->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

	engine->copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(engine->device, stagingBuffer, nullptr);
	vkFreeMemory(engine->device, stagingBufferMemory, nullptr);
}

const void Mesh::cleanup() {
	Engine* engine = Engine::get();

	vkDestroyBuffer(engine->device, indexBuffer, nullptr);
	vkFreeMemory(engine->device, indexBufferMemory, nullptr);

	vkDestroyBuffer(engine->device, vertexBuffer, nullptr);
	vkFreeMemory(engine->device, vertexBufferMemory, nullptr);
}

void UIMesh::UpdateVertices(float xp, float yp, float xsc, float ysc) {
	vertices.clear();

	Vertex vertex{};

	vertex.pos = { -xsc + xp, -ysc - yp, 0.0f };
	vertex.normal = { 0.0f, 0.0f, 0.0f };
	vertex.texCoord = { 0.0f, 0.0f };
	vertices.push_back(vertex);
	vertex.pos = { xsc + xp, -ysc - yp, 0.0f };
	vertex.texCoord = { 1.0f, 0.0f };
	vertices.push_back(vertex);
	vertex.pos = { xsc + xp, ysc - yp, 0.0f };
	vertex.texCoord = { 1.0f, 1.0f };
	vertices.push_back(vertex);
	vertex.pos = { -xsc + xp, ysc - yp, 0.0f };
	vertex.texCoord = { 0.0f, 1.0f };
	vertices.push_back(vertex);

	if (vBuffer == nullptr) {
		createVertexBuffer();
		createIndexBuffer();
	}
	else {
		updateVertexBuffer();
	}
}

void UIMesh::createVertexBuffer() {

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

void UIMesh::updateVertexBuffer() {
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	memcpy(vBuffer, vertices.data(), (size_t)bufferSize);
}

StaticMesh::StaticMesh() {
	loadModel();
	createVertexBuffer();
	createIndexBuffer();
}

bool StaticMesh::loadModel() {
	tinyobj::attrib_t attrib;
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string warn, err;

	string testMODEL_PATH;

	try {
		testMODEL_PATH = winFile::OpenFileDialog();
	}
	catch (...) {
		return false;
	}


	if (tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, testMODEL_PATH.c_str())) {
		unordered_map<Vertex, uint32_t> uniqueVertices{};

		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex{};

				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};

				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};


				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);
			}
		}
		return true;
	}
	return false;
}