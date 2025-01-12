#include "InputManager.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtx/hash.hpp>

#include "CameraController.h"

using namespace glm;

void Camera::updateCamera() {
	radius = glm::length(pos);
	float scalefac = pow(radius, 2) / defaultscale;
	view = lookAt(pos, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f));
	pos = pos + vec3(vec4(dir::getHorizontalAxis() * velocity * scalefac, dir::getVerticalAxis() * velocity * scalefac, dir::getForwardAxis() * velocity * -1.0f * scalefac, 0.0f) * view);
	view = lookAt(pos, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f));
}