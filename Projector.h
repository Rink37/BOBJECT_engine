#include"Meshes.h"
#include"Textures.h"
#include"CameraController.h"
#include"Bobject_Engine.h"

class Projector {
public:
	Projector(Texture* t, Mesh* m) {
		tex = t;
		mesh = m;
		for (uint32_t i = 0; i != mesh->vertices.size(); i++) {
			originalUVs.push_back(mesh->vertices[i].texCoord);
		}
	}

	void setCamera(Camera*);
	void shareCamera(Camera*);

	void calculateUVs();

	Texture* tex;
	Mesh* mesh;
	Mesh* reprojectionMesh = nullptr;
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