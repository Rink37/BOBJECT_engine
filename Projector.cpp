#include"Projector.h"

void Projector::setCamera(Camera* cam) {
	view = cam->view;
	pos = cam->pos;
	center = cam->center;
	fov = cam->fov;
	proj = glm::perspective(glm::radians(fov), static_cast<float>(tex->texWidth) / static_cast<float>(tex->texHeight), 0.1f, 25.0f);
}

void Projector::calculateUVs() {
	projectedUVs.clear();
	//std::cout << tex->texWidth << " " << tex->texHeight << std::endl;
	//std::cout << Engine::get()->windowWidth << " " << Engine::get()->windowHeight << std::endl;
	for (uint32_t i = 0; i != mesh->vertices.size(); i++) {
		glm::vec4 projectedPoint = proj * view * glm::vec4(mesh->vertices[i].pos, 1.0f);
		projectedUVs.push_back(glm::vec2(projectedPoint.x / (2.0f * projectedPoint.w) + 0.5f, 0.5f - projectedPoint.y / (2.0f * projectedPoint.w)));
	}
	for (uint32_t i = 0; i != mesh->vertices.size(); i++) {
		mesh->vertices[i].texCoord = projectedUVs[i];
	}
	mesh->setup();
}