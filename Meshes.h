#ifndef MESHDEFS
#define MESHDEFS

#include"Bobject_VKMem.h"
#include"Bobject_Core.h"

#include<array>
//#include"Materials.h"

#define NORM_SIZE 1.5f

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;
	glm::vec4 tangent;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3>getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, normal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}

	static std::array<VkVertexInputAttributeDescription, 4>getCompleteAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, normal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, tangent);

		return attributeDescriptions;
	}

	bool operator== (const Vertex& other) const {
		return pos == other.pos && normal == other.normal && texCoord == other.texCoord;
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

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

	void updateSwizzle();

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
	StaticMesh(std::string, uint8_t);
	StaticMesh(std::string, uint8_t, float&);
	void computeTangents();

	void resizeMesh(float);

	BoundingBox bbox;
private:
	bool loadModel(std::string, uint8_t);
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

#endif
