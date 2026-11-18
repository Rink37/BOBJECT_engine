#include"Meshes.h"
#include"Textures.h"
#include"CameraController.h"
#include"Bobject_Engine.h"
#include"UIElements.h"
#include"ImageProcessor.h"
#include"include/BakeAlpha.h"

struct ProjectorPositionObject {
	alignas(16) glm::vec4 pos = glm::vec4(0.0f);
	alignas(4) float maxAngle = 1.5708f;
};

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
		
		createPositionBuffer();
		updatePositionBuffer();
		
		std::vector<std::pair<std::vector<VkBuffer>, uint32_t>> projectorBuffers{std::pair<std::vector<VkBuffer>, uint32_t> {Engine::get()->uniformBuffers, sizeof(UniformBufferObject)}, std::pair<std::vector<VkBuffer>, uint32_t>{std::vector<VkBuffer>{positionBuffer}, sizeof(ProjectorPositionObject)} };
		
		shadedProjection = new Material(std::vector<Texture*>{tex}, projectorBuffers, "Projector_BF", gP);
		flatProjection = new Material(std::vector<Texture*>{tex}, projectorBuffers, "Projector_Flat", gP);
		backgroundMat = new Material(tex, "Projector_Background");
	}

	void cleanup();

	std::function<void(std::string, Texture*)> saveTextureFnc = nullptr;

	void setCamera(Camera*);
	void shareCamera(Camera*);

	void calculateUVs();

	void createMarkerVertices(float, Vertex&, Vertex&, Vertex&, Vertex&);
	void makeProjectorMarker();
	void makeBackgroundMarker();

	void createPositionBuffer();
	void updatePositionBuffer();

	void updateMaxAngle(float);

	ProjectorPositionObject ppo{};

	VkBuffer positionBuffer = nullptr;
	void* positionBufferMapped = nullptr;
	VkDeviceMemory positionBufferMemory = nullptr;

	Texture* tex;
	std::string texName{};
	Mesh* refMesh;
	Mesh* shellMesh = nullptr;
	Mesh* reprojectionMesh = nullptr;

	Mesh* backgroundRect = nullptr;
	Mesh* projectorMarker = nullptr;

	Material* shadedProjection = nullptr;
	Material* flatProjection = nullptr;
	Material* backgroundMat = nullptr;

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
};