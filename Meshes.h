#ifndef MESHDEFS
#define MESHDEFS

#include"Bobject_Engine.h"
#include"Materials.h"

struct SeamStrip {
	std::vector<uint32_t> leftIndices{};
	std::vector<uint32_t> rightIndices{};
};

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

	void createSeamMeshes(cv::Mat, SeamStrip&);
	void findAdjacentStrips(cv::Mat);
	void getStripChain(SeamStrip&, uint32_t, std::vector<std::array<uint32_t, 2>>&, std::vector<uint32_t>&);

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

	void setMat(Material* m, std::string shader) {
		mat = m;
		shaderName = shader;
	}

	bool isVisible = true;
	bool isWireframeVisible = true;
	StaticMesh* mesh = nullptr;
	Material* mat = nullptr;
	Material* unlitMat = nullptr;

	std::string objectName = "";
	std::string materialName = "Webcam Material";
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
