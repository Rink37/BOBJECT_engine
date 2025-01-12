#pragma once

#ifndef CAMERA_CONTROLLER
#define CAMERA_CONTROLLER

#include "InputManager.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtx/hash.hpp>

#include "math.h"

class Camera {
public:
	glm::vec3 pos = glm::vec3(2.0f, 2.0f, 2.0f);
	glm::mat4 view;
	float fov = 45.0f;
	float velocity = 0.005f;
	float radius = glm::length(pos);
	float defaultscale = pow(glm::length(pos), 2);

	void updateCamera();
};

#endif