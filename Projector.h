#include"Meshes.h"
#include"Textures.h"
#include"CameraController.h"
#include"Bobject_Engine.h"

class Projector {
public:
	Projector(Texture* t, Mesh* m, std::string tN, std::function<void(std::string, Texture*)> saveFnc, GraphicsPass* gP) {
		tex = t;
		refMesh = m;
		for (uint32_t i = 0; i != refMesh->vertices.size(); i++) {
			originalUVs.push_back(refMesh->vertices[i].texCoord);
		}
		saveTextureFnc = saveFnc;
		texName = tN;

		shadedProjection = new Material(std::vector<Texture*>{tex}, "Affine_BF", gP);
		flatProjection = new Material(std::vector<Texture*>{tex}, "Affine_Flat", gP);
	}

	void cleanup();

	std::function<void(std::string, Texture*)> saveTextureFnc = nullptr;

	void setCamera(Camera*);
	void shareCamera(Camera*);

	void calculateUVs();

	void createMarkerVertices(float, Vertex&, Vertex&, Vertex&, Vertex&);
	void makeProjectorMarker();
	void makeBackgroundMarker();

	Texture* tex;
	std::string texName{};
	Mesh* refMesh;
	Mesh* shellMesh = nullptr;
	Mesh* reprojectionMesh = nullptr;

	Mesh* backgroundRect = nullptr;
	Mesh* projectorMarker = nullptr;

	Material* shadedProjection = nullptr;
	Material* flatProjection = nullptr;

	std::vector<glm::vec2> originalUVs{};
	std::vector<glm::vec2> projectedUVs{};

	glm::mat4 proj{};
	glm::mat4 view{};
	glm::vec3 pos{};
	glm::vec3 center{};
	float xrot = 45.0f;
	float yrot = 10.0f;
	float distance = 5.0f;
	float fov = 0.0f;

	void reprojectImage();
	void createReprojectionMesh();

private:
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
			cleaned = true;
		}

		bool cleaned = true;
	};

	void prepMap(OverlayMap*);
	void prepareColourDescriptor(OverlayMap*);
	void createTexWritePipeline(OverlayMap*);

	VkCommandBuffer drawColourMap(VkCommandBuffer, OverlayMap*);
};