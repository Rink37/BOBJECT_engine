#ifndef MESHDEFS
#define MESHDEFS

#include"Bobject_Engine.h"

struct Mesh {
	std::vector<Vertex> vertices;
	
	VkBuffer vertexBuffer{};
	VkDeviceMemory vertexBufferMemory{};

	std::vector<uint32_t> indices;

	VkBuffer indexBuffer{};
	VkDeviceMemory indexBufferMemory{};

	std::vector<uint32_t> uniqueTexindices{};

	VkBuffer texCoordIndexBuffer{};
	VkDeviceMemory texCoordIndexBufferMemory{};

	virtual void createVertexBuffer();
	virtual void computeTangents() {};
	void createIndexBuffer();

	void createTexCoordIndexBuffer();

	const void cleanup();
};

class UIMesh : public Mesh {
public:
	UIMesh() {
		indices = { 0, 3, 2, 2, 1, 0 };
		//UpdateVertices(0.0f, 0.0f, 1.0f, 1.0f);
	}

	void* vBuffer = nullptr;

	void UpdateVertices(float, float, float, float);

	void createVertexBuffer();
	void updateVertexBuffer();
};

class StaticMesh : public Mesh {
public:
	StaticMesh() {};
	StaticMesh(std::string);
	void computeTangents();
private:
	bool loadModel(std::string);
};

#endif
