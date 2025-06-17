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

void Mesh::createTexCoordIndexBuffer() {
	Engine* engine = Engine::get();

	VkDeviceSize bufferSize = sizeof(uniqueTexindices[0]) * uniqueTexindices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	engine->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(engine->device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, uniqueTexindices.data(), (size_t)bufferSize);
	vkUnmapMemory(engine->device, stagingBufferMemory);

	engine->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texCoordIndexBuffer, texCoordIndexBufferMemory);

	engine->copyBuffer(stagingBuffer, texCoordIndexBuffer, bufferSize);

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

	vertex.pos = { -xsc + xp, yp - ysc, 0.0f };
	vertex.normal = { 0.0f, 0.0f, 0.0f };
	vertex.texCoord = { 0.0f, 0.0f };
	vertices.push_back(vertex);
	vertex.pos = { xsc + xp, yp - ysc, 0.0f };
	vertex.texCoord = { 1.0f, 0.0f };
	vertices.push_back(vertex);
	vertex.pos = { xsc + xp, yp + ysc, 0.0f };
	vertex.texCoord = { 1.0f, 1.0f };
	vertices.push_back(vertex);
	vertex.pos = { -xsc + xp, yp + ysc, 0.0f };
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
		unordered_map<glm::vec2, uint32_t> uniqueCoords{};

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

				// We also want to find any vertices which share a texture coord with a vertex that has a different position

				if (uniqueCoords.count(vertex.texCoord) == 0) {
					uniqueCoords[vertex.texCoord] = static_cast<uint32_t>(vertices.size());
				}

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);

				if (vertices[uniqueCoords[vertex.texCoord]].pos == vertices[uniqueVertices[vertex]].pos) {
					uniqueTexindices.push_back(uniqueVertices[vertex]);
				}

			}
		}
		computeTangents();
		return true;
	}
	return false;
}

void StaticMesh::computeTangents() {
	// First we initialise all the tangents and bitangents
	for (Vertex vert : vertices) {
		vert.tangent = glm::vec4(0, 0, 0, 0);
		vert.biTangent = glm::vec3(0, 0, 0);
	}

	// Then we calculate the tangents and bitangents described by the plane of each triangle
	for (size_t i = 0; i != indices.size(); i+=3) {

		size_t i0 = indices[i + 0];
		size_t i1 = indices[i + 1];
		size_t i2 = indices[i + 2];
		
		glm::vec3& v0 = vertices[i0].pos;
		glm::vec3& v1 = vertices[i1].pos;
		glm::vec3& v2 = vertices[i2].pos;

		glm::vec2& uv0 = vertices[i0].texCoord;
		glm::vec2& uv1 = vertices[i1].texCoord;
		glm::vec2& uv2 = vertices[i2].texCoord;

		glm::vec3 deltaPos1 = v1 - v0;
		glm::vec3 deltaPos2 = v2 - v0;

		glm::vec2 deltaUV1 = uv1 - uv0;
		glm::vec2 deltaUV2 = uv2 - uv0;

		float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
		glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
		glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
		glm::vec4 fourTan = glm::vec4(tangent, 1.0);

		vertices[i0].tangent += fourTan;
		vertices[i1].tangent += fourTan;
		vertices[i2].tangent += fourTan;

		vertices[i0].biTangent += bitangent;
		vertices[i1].biTangent += bitangent;
		vertices[i2].biTangent += bitangent;
	}

	// Finally we compute the normalized tangent vectors as well as the facing direction
	// w describes whether the normals need to be inverted or not (for vertices that have been mirrored and share UV coordinates)

	for (size_t i = 0; i != vertices.size(); i++) {
		glm::vec3 n = vertices[i].normal;
		glm::vec3 t0 = vertices[i].tangent;
		glm::vec3 t1 = vertices[i].biTangent;

		glm::vec3 t = t0 - (n * dot(n, t0));
		t = normalize(t); // Disabling this normalization will scale the vectors based on the scale of the vertices (perhaps this is more ideal?)

		glm::vec3 c = cross(n, t0);
		float w = (dot(c, t1) < 0) ? -1.0f : 1.0f;
		vertices[i].tangent = glm::vec4(t.x, t.y, t.z, w);

		t = cross(glm::vec3(vertices[i].tangent.x, vertices[i].tangent.y, vertices[i].tangent.z), vertices[i].normal);
		vertices[i].biTangent = t;
	}
}