#include"Meshes.h"
#include"Textures.h"
#include"CameraController.h"

class Projector {
public:
	Projector(Texture* t, Mesh* m) {
		tex = t;
		mesh = m;
	}

	void setCamera(Camera*);

	void calculateUVs();

	Texture* tex;
	Mesh* mesh;
	std::vector<glm::vec2> projectedUVs{};

	glm::mat4 proj{};
	glm::mat4 view{};
	glm::vec3 pos{};
	glm::vec3 center{};
	float fov = 0.0f;
};