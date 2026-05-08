#ifndef MESHDEFS
#define MESHDEFS

#include"Bobject_Engine.h"
#include"Materials.h"

struct Mesh {
	std::vector<Vertex> vertices;
	
	VkBuffer vertexBuffer{};
	VkDeviceMemory vertexBufferMemory{};

	std::vector<uint32_t> indices;

	VkBuffer indexBuffer{};
	VkDeviceMemory indexBufferMemory{};

	std::vector<uint32_t> uniqueTexindices{};

	virtual void setup() {
		createVertexBuffer();
		createIndexBuffer();
		cleaned = false;
	}

	virtual void createVertexBuffer();
	virtual void computeTangents() {};
	void createIndexBuffer();

	const void cleanup();
	bool cleaned = true;
};

class UIMesh : public Mesh {
public:
	UIMesh() {
		indices = { 0, 3, 2, 2, 1, 0 };
	}

	void* vBuffer = nullptr;

	void UpdateVertices(float, float, float, float, float zp = 0.0f);

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

class PlaneMesh : public Mesh {
public:
	PlaneMesh(uint32_t width, uint32_t height) {
		aspectRatio = static_cast<float>(height) / static_cast<float>(width);

		constructMesh();
		computeTangents();
		createVertexBuffer();
		createIndexBuffer();

		cleaned = false;
	}

	float aspectRatio = 0.0f;
	float size = 1.0f;

private:
	void constructMesh();
	void computeTangents();
};

class StaticObject {
public:
	StaticObject(std::string name) {
		mesh = new StaticMesh(name);
	}

	bool isVisible = true;
	bool isWireframeVisible = true;
	StaticMesh* mesh = nullptr;
	Material* mat = nullptr;

	std::string shaderName = "BF";
};

class PlaneObject {
public:
	PlaneObject(uint32_t width, uint32_t height) {
		mesh = new PlaneMesh(width, height);
	}

	bool isVisible = true;
	bool isWireframeVisible = true;
	PlaneMesh* mesh = nullptr;
	Material* mat = nullptr;
};

#endif
