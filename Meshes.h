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

struct SeamStrip {
	std::vector<uint32_t> leftIndices{};
	std::vector<uint32_t> rightIndices{};

	Mesh leftMesh{};
	Mesh leftAlphaMesh{};

	Mesh rightMesh{};
	Mesh rightAlphaMesh{};
};

class SeamFixer {
public:
	SeamFixer(Mesh* mesh) {
		target = mesh;
	}

	struct OverlayMap {
		VkFramebuffer frameBuffer = nullptr;
		VkRenderPass renderPass = nullptr;
		Texture* colour = nullptr;
		VkDescriptorPool descriptorPool = nullptr;
		VkDescriptorSet descriptorSet = nullptr;
		VkDescriptorSetLayout descriptorSetLayout = nullptr;
		VkPipelineLayout pipelineLayout = nullptr;
		VkPipeline pipeline = nullptr;

		void cleanup() {
			vkDeviceWaitIdle(Engine::get()->device);
			if (colour != nullptr) {
				colour->cleanup();
				delete colour;
			}
			if (descriptorSetLayout != nullptr) {
				vkDestroyDescriptorSetLayout(Engine::get()->device, descriptorSetLayout, nullptr);
				vkDestroyDescriptorPool(Engine::get()->device, descriptorPool, nullptr);
			}
			if (pipeline != nullptr) {
				vkDestroyPipeline(Engine::get()->device, pipeline, nullptr);
				vkDestroyPipelineLayout(Engine::get()->device, pipelineLayout, nullptr);
			}
			if (renderPass != nullptr) {
				vkDestroyRenderPass(Engine::get()->device, renderPass, nullptr);
			}
			if (frameBuffer != nullptr) {
				vkDestroyFramebuffer(Engine::get()->device, frameBuffer, nullptr);
			}
		}
	};

	void createSeamMeshes(cv::Mat, SeamStrip&);
	void findAdjacentStrips(cv::Mat);
	void getStripChain(SeamStrip&, uint32_t, std::vector<std::array<uint32_t, 2>>&, std::vector<uint32_t>&);

	void alphaOverRight() {
		drawRightMap();
		drawRightAlpha();
		alphaOverMap(true);
	}

	void alphaOverLeft() {
		drawLeftMap();
		drawLeftAlpha();
		alphaOverMap(false);
	}

	void drawRightMap() {
		prepMap(false, true);
		prepareColourDescriptor(true);
		createTexWritePipeline(true);
		VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
		commandBuffer = drawColourMap(commandBuffer, true);
		Engine::get()->endSingleTimeCommands(commandBuffer);

		rightMap.colour->getCVMat();
		cv::imshow("Drawn Map", rightMap.colour->texMat);
		cv::waitKey(0);
	}

	void drawLeftMap() {
		prepMap(false, false);
		prepareColourDescriptor(false);
		createTexWritePipeline(false);
		VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
		commandBuffer = drawColourMap(commandBuffer, false);
		Engine::get()->endSingleTimeCommands(commandBuffer);

		leftMap.colour->getCVMat();
		cv::imshow("Drawn Map", leftMap.colour->texMat);
		cv::waitKey(0);
	}

	void drawRightAlpha() {
		prepMap(true, true);
		createAlphaWritePipeline(true);
		VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
		commandBuffer = drawAlphaMap(commandBuffer, true);
		Engine::get()->endSingleTimeCommands(commandBuffer);

		rightAlpha.colour->getCVMat();
		cv::imshow("Drawn Map", rightAlpha.colour->texMat);
		cv::waitKey(0);
	}

	void drawLeftAlpha() {
		prepMap(true, false);
		createAlphaWritePipeline(false);
		VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
		commandBuffer = drawAlphaMap(commandBuffer, false);
		Engine::get()->endSingleTimeCommands(commandBuffer);

		leftAlpha.colour->getCVMat();
		cv::imshow("Drawn Map", leftAlpha.colour->texMat);
		cv::waitKey(0);
	}

	void cleanup() {
		leftMap.cleanup();
		leftAlpha.cleanup();
		rightMap.cleanup();
		rightAlpha.cleanup();

		imageTex->cleanup();

		for (SeamStrip strip : seamStrips) {
			strip.leftAlphaMesh.cleanup();
			strip.leftMesh.cleanup();
			strip.rightAlphaMesh.cleanup();
			strip.rightMesh.cleanup();
		}
	}
private:
	Mesh* target = nullptr;

	OverlayMap leftMap{};
	OverlayMap leftAlpha{};
	OverlayMap rightMap{};
	OverlayMap rightAlpha{};

	Texture* imageTex = nullptr;

	std::vector<OverlayMap*> maps{ &leftMap, &rightMap };
	std::vector<OverlayMap*> alphaMaps{ &leftAlpha, &rightAlpha };

	std::vector<SeamStrip> seamStrips{};

	uint32_t width = 0;
	uint32_t height = 0;

	void sortSeamIndices(SeamStrip&);
	void prepMap(bool, bool);
	void prepareColourDescriptor(bool);
	void createTexWritePipeline(bool);
	void createAlphaWritePipeline(bool);
	VkCommandBuffer drawColourMap(VkCommandBuffer, bool);
	VkCommandBuffer drawAlphaMap(VkCommandBuffer, bool);

	void alphaOverMap(bool);
};


#endif
