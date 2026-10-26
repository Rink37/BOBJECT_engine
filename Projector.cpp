#include"Projector.h"

void Projector::setCamera(Camera* cam) {
	view = cam->view;
	pos = cam->pos;
	center = cam->center;
	fov = cam->fov;
	proj = glm::perspective(glm::radians(fov), tex->texWidth / (float)tex->texHeight, 0.1f, 25.0f);
}

void Projector::calculateUVs() {
	projectedUVs.clear();
	for (uint32_t i = 0; i != mesh->vertices.size(); i++) {
		glm::vec4 projectedPoint = proj * view * glm::vec4(mesh->vertices[i].pos, 1.0f);
		projectedUVs.push_back(projectedPoint);
	}
}