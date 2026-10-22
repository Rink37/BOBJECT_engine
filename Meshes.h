#ifndef MESHDEFS
#define MESHDEFS

#include"Bobject_Engine.h"
#include"Materials.h"

#define NORM_SIZE 1.5f

struct BoundingBox{
	std::pair<float, float> x_range{ 0.0f, 0.0f };
	std::pair<float, float> y_range{ 0.0f, 0.0f };
	std::pair<float, float> z_range{ 0.0f, 0.0f };

	glm::vec3 size{ 0.0f, 0.0f, 0.0f };
	glm::vec3 position{ 0.0f, 0.0f, 0.0f };

	void clearRanges() {
		x_range.first = 0.0f;
		x_range.second = 0.0f;
		y_range.first = 0.0f;
		y_range.second = 0.0f;
		z_range.first = 0.0f;
		z_range.second = 0.0f;
	}

	void updateRanges(glm::vec3 vertPos) {
		x_range.first = (vertPos.x < x_range.first) ? vertPos.x : x_range.first;
		x_range.second = (vertPos.x > x_range.second) ? vertPos.x : x_range.second;
		y_range.first = (vertPos.y < y_range.first) ? vertPos.y : y_range.first;
		y_range.second = (vertPos.y > y_range.second) ? vertPos.y : y_range.second;
		z_range.first = (vertPos.z < z_range.first) ? vertPos.z : z_range.first;
		z_range.second = (vertPos.z > z_range.second) ? vertPos.z : z_range.second;
	}

	void getPosition() {
		position.x = (x_range.first + x_range.second) / 2.0f;
		position.y = (y_range.first + y_range.second) / 2.0f;
		position.z = (z_range.first + z_range.second) / 2.0f;
	}

	void getSize() {
		size.x = abs(x_range.second - x_range.first);
		size.y = abs(y_range.second - y_range.first);
		size.z = abs(z_range.second - z_range.first);
	}

	float getNormFactor() {
		float factor = 1.0f;
		float maxSize = (size.x > size.y) ? size.x : size.y;
		maxSize = (size.z > maxSize) ? size.z : maxSize;
		if (maxSize > 0.0f) {
			factor = NORM_SIZE / maxSize;
		}
		return factor;
	}
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
		if (!cleaned) {
			cleanup();
		}
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
	StaticMesh(std::string, float&);
	void computeTangents();

	void resizeMesh(float);

	BoundingBox bbox;
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
	StaticObject(std::string name, float& scaleFac) {
		mesh = new StaticMesh(name, scaleFac);
	}

	void setMat(Material* m, std::string shader) {
		mat = m;
		unlitMat = m;
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
