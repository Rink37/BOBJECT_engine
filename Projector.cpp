#include"Projector.h"
#include"include/ShaderDataType.h"
#include"include/SeamFix_Colour.h"
#include"include/ProjectorAlphaWrite.h"

void Projector::setCamera(Camera* cam) {
	view = cam->view;
	pos = cam->pos;
	center = cam->center;
	fov = cam->fov;
	xrot = cam->xrot;
	yrot = cam->yrot;
	distance = cam->getInstance().distance;
	proj = glm::perspective(glm::radians(fov), static_cast<float>(tex->texWidth) / static_cast<float>(tex->texHeight), 0.1f, 25.0f);
	makeProjectorMarker();
	makeBackgroundMarker();
	ppo.pos = glm::vec4(pos, 1.0f);
	updatePositionBuffer();
}

void Projector::shareCamera(Camera* cam) {
	cam->xrot = xrot;
	cam->yrot = yrot;
	cam->center = center;
	cam->fov = fov;
	cam->getInstance().distance = distance;
}

void Projector::createMarkerVertices(float distance, Vertex& v1, Vertex& v2, Vertex& v3, Vertex& v4) {
	glm::vec3 centralPos = pos + glm::normalize(center - pos) * distance;
	float markerHeight = distance * tan(glm::radians(fov * 0.5f));
	float markerWidth = markerHeight * tex->texWidth / tex->texHeight;
	glm::mat3 rotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(yrot), glm::vec3(0.0f, 1.0f, 0.0f))) * glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(xrot), glm::vec3(0.0f, 0.0f, 1.0f)));

	glm::vec3 p1 = glm::vec3(0.0f, -markerWidth, -markerHeight);
	glm::vec3 p2 = glm::vec3(0.0f, markerWidth, -markerHeight);
	glm::vec3 p3 = glm::vec3(0.0f, markerWidth, markerHeight);
	glm::vec3 p4 = glm::vec3(0.0f, -markerWidth, markerHeight);
	v1.pos = glm::vec3(p1 * rotationMatrix) + centralPos;
	v2.pos = glm::vec3(p2 * rotationMatrix) + centralPos;
	v3.pos = glm::vec3(p3 * rotationMatrix) + centralPos;
	v4.pos = glm::vec3(p4 * rotationMatrix) + centralPos;
	v1.normal = glm::normalize(pos - v1.pos);
	v2.normal = glm::normalize(pos - v2.pos);
	v3.normal = glm::normalize(pos - v3.pos);
	v4.normal = glm::normalize(pos - v4.pos);
	v1.texCoord = glm::vec2(0.0f, 1.0f);
	v2.texCoord = glm::vec2(1.0f, 1.0f);
	v3.texCoord = glm::vec2(1.0f, 0.0f);
	v4.texCoord = glm::vec2(0.0f, 0.0f);
}

void Projector::makeProjectorMarker() {
	float distance = 0.11f;
	if (projectorMarker != nullptr) {
		projectorMarker->cleanup();
		delete projectorMarker;
	}
	projectorMarker = new Mesh();
	Vertex v1{};
	Vertex v2{};
	Vertex v3{};
	Vertex v4{};
	Vertex v5{};
	v5.pos = glm::vec3(pos);
	v5.normal = glm::normalize(pos - center);
	v5.texCoord = glm::vec2(0.0f, 0.0f);
	createMarkerVertices(distance, v1, v2, v3, v4);
	projectorMarker->vertices.push_back(v1);
	projectorMarker->vertices.push_back(v2);
	projectorMarker->vertices.push_back(v3);
	projectorMarker->vertices.push_back(v4);
	projectorMarker->vertices.push_back(v5);
	projectorMarker->indices = std::vector<uint32_t>{ 0, 1, 4, 4, 1, 0, 1, 2, 4, 4, 2, 1, 2, 3, 4, 4, 3, 2, 3, 0, 4, 4, 0, 3 };
	projectorMarker->setup();
}

void Projector::makeBackgroundMarker() {
	float distance = 24.9f;
	if (backgroundRect != nullptr) {
		backgroundRect->cleanup();
		delete backgroundRect;
	}
	backgroundRect = new Mesh();
	Vertex v1{};
	Vertex v2{};
	Vertex v3{};
	Vertex v4{};
	createMarkerVertices(distance, v1, v2, v3, v4);
	backgroundRect->vertices.push_back(v1);
	backgroundRect->vertices.push_back(v2);
	backgroundRect->vertices.push_back(v3);
	backgroundRect->vertices.push_back(v4);
	backgroundRect->indices = std::vector<uint32_t>{ 0, 1, 2, 2, 3, 0 };
	backgroundRect->setup();
}

void Projector::cleanup() {
	vkDeviceWaitIdle(Engine::get()->device);
	if (reprojectionMesh != nullptr) {
		reprojectionMesh->cleanup();
		delete reprojectionMesh;
		reprojectionMesh = nullptr;
	}
	if (shellMesh != nullptr) {
		shellMesh->cleanup();
		delete shellMesh;
		shellMesh = nullptr;
	}
	if (projectorMarker != nullptr) {
		projectorMarker->cleanup();
		delete projectorMarker;
		projectorMarker = nullptr;
	}
	if (backgroundRect != nullptr) {
		backgroundRect->cleanup();
		delete backgroundRect;
		backgroundRect = nullptr;
	}
	if (shadedProjection != nullptr) {
		shadedProjection->cleanup();
		delete shadedProjection;
		shadedProjection = nullptr;
	}
	if (flatProjection != nullptr) {
		flatProjection->cleanup();
		delete flatProjection;
		flatProjection = nullptr;
	}
	if (backgroundMat != nullptr) {
		backgroundMat->cleanup();
		delete backgroundMat;
		backgroundMat = nullptr;
	}
	if (positionBuffer != nullptr) {
		vkDestroyBuffer(Engine::get()->device, positionBuffer, nullptr);
		vkFreeMemory(Engine::get()->device, positionBufferMemory, nullptr);
	}
}

void Projector::calculateUVs() {
	projectedUVs.clear();
	if (shellMesh == nullptr) {
		shellMesh = new Mesh();
	}
	shellMesh->vertices.clear();
	for (uint32_t i = 0; i != refMesh->vertices.size(); i++) {
		glm::vec4 projectedPoint = proj * view * glm::vec4(refMesh->vertices[i].pos, 1.0f);
		projectedUVs.push_back(glm::vec2(projectedPoint.x / (2.0f * projectedPoint.w) + 0.5f, 0.5f - projectedPoint.y / (2.0f * projectedPoint.w)));
	}
	for (uint32_t i = 0; i != refMesh->vertices.size(); i++) {
		shellMesh->vertices.push_back(refMesh->vertices[i]);
		shellMesh->vertices[i].pos = shellMesh->vertices[i].pos + shellMesh->vertices[i].normal * 0.0005f;
		shellMesh->vertices[i].texCoord = projectedUVs[i];
	}
	for (uint32_t i = 0; i < refMesh->indices.size() - 2; i += 3) {
		glm::vec3 n0 = shellMesh->vertices[refMesh->indices[i]].normal;
		glm::vec3 n1 = shellMesh->vertices[refMesh->indices[i + 1]].normal;
		glm::vec3 n2 = shellMesh->vertices[refMesh->indices[i + 2]].normal;
		glm::vec3 avgNormal = (n0 + n1 + n2) / 3.0f;

		glm::vec3 p0 = shellMesh->vertices[refMesh->indices[i]].pos;
		glm::vec3 p1 = shellMesh->vertices[refMesh->indices[i + 1]].pos;
		glm::vec3 p2 = shellMesh->vertices[refMesh->indices[i + 2]].pos;
		glm::vec3 avgPos = (n0 + n1 + n2) / 3.0f;

		glm::vec3 pointingVec = pos - avgPos;
		float dp0 = glm::dot(n0, glm::normalize(pos - p0));
		float dp1 = glm::dot(n1, glm::normalize(pos - p1));
		float dp2 = glm::dot(n2, glm::normalize(pos - p2));

		if (acos(abs(dp0)) <= 1.963f || acos(abs(dp1)) <= 1.963f || acos(abs(dp2)) <= 1.963f) {
			shellMesh->indices.push_back(refMesh->indices[i]);
			shellMesh->indices.push_back(refMesh->indices[i+1]);
			shellMesh->indices.push_back(refMesh->indices[i+2]);
		}
	}
	shellMesh->setup();
}

void Projector::createPositionBuffer() {
	VkDeviceSize bufferSize = sizeof(ProjectorPositionObject);

	Engine::get()->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, positionBuffer, positionBufferMemory);
	vkMapMemory(Engine::get()->device, positionBufferMemory, 0, bufferSize, 0, &positionBufferMapped);
}

void Projector::updatePositionBuffer() {
	memcpy(positionBufferMapped, &ppo, sizeof(ppo));
}

void Projector::reprojectImage() {
	createReprojectionMesh();
	textureWriter reprojector{ tex, new SEAMFIX_COLOURSHADER, false };
	std::vector<std::pair<std::vector<VkBuffer>, uint32_t>> projectorBuffers{ std::pair<std::vector<VkBuffer>, uint32_t> {Engine::get()->uniformBuffers, sizeof(UniformBufferObject)}, std::pair<std::vector<VkBuffer>, uint32_t>{std::vector<VkBuffer>{positionBuffer}, sizeof(ProjectorPositionObject)} };
	textureWriter alphaWriter{ tex, new PROJECTORALPHAWRITESHADER, std::vector<Texture*>{}, projectorBuffers, false };
	Texture* resTex = reprojector.drawMap(std::vector<Mesh*>{reprojectionMesh});
	Texture* alphaTex = alphaWriter.drawMap(std::vector<Mesh*>{refMesh});
	reprojector.cleanup();
	alphaWriter.cleanup();
	if (saveTextureFnc != nullptr) {
		saveTextureFnc(texName + std::string("_projected"), resTex);
		saveTextureFnc(texName + std::string("_projectedAlpha"), alphaTex);
	}
}

void Projector::createReprojectionMesh() {
	if (reprojectionMesh != nullptr) {
		reprojectionMesh->cleanup();
		delete reprojectionMesh;
	}
	reprojectionMesh = new Mesh();
	for (uint32_t i = 0; i != refMesh->vertices.size(); i++) {
		Vertex vertex{};
		vertex.pos = glm::vec3(originalUVs[i], 0.0f);
		vertex.texCoord = projectedUVs[i];
		reprojectionMesh->vertices.push_back(vertex);
	}
	reprojectionMesh->indices = refMesh->indices;
	reprojectionMesh->setup();
}