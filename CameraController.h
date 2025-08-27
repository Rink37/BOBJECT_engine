<<<<<<< HEAD
#ifndef CAMERA_CONTROLLER
#define CAMERA_CONTROLLER

#include "InputManager.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtx/hash.hpp>

#include<algorithm>
#include<iostream>

#include "math.h"

class Camera {
public:
	glm::vec3 pos;
	glm::vec3 initCenter;
	glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::mat4 view;
	float defaultDistance = 10.0f;
	float distance = 5.0f;
	float initxrot;
	float inityrot;
	float xrot = 45.0f;
	float yrot = 0.0f;
	double initXpos, initYpos;
	double activeXpos, activeYpos;
	float fov = 45.0f;
	float defaultVelocity = 0.01f;
	float velocity;
	float radius = glm::length(pos);
	double defaultscale = pow(glm::length(pos), 2);
	bool isPanning = false;
	bool isRotating = false;

	void updateCamera(GLFWwindow* window);

	static Camera& getInstance() {
		static Camera instance;
		return instance;
	}

	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
		getInstance().distance = std::clamp(static_cast<float>(getInstance().distance - yoffset), 1.0f, 100.0f);
	};
};

=======
#ifndef CAMERA_CONTROLLER
#define CAMERA_CONTROLLER

#include "InputManager.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtx/hash.hpp>

#include<algorithm>
#include<iostream>

#include "math.h"

class Camera {
public:
	glm::vec3 pos;
	glm::vec3 initCenter;
	glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::mat4 view;
	float defaultDistance = 10.0f;
	float distance = 5.0f;
	float initxrot;
	float inityrot;
	float xrot = 45.0f;
	float yrot = 0.0f;
	double initXpos, initYpos;
	double activeXpos, activeYpos;
	float fov = 45.0f;
	float defaultVelocity = 0.01f;
	float velocity;
	float radius = glm::length(pos);
	double defaultscale = pow(glm::length(pos), 2);
	bool isPanning = false;
	bool isRotating = false;

	void updateCamera(GLFWwindow* window);

	static Camera& getInstance() {
		static Camera instance;
		return instance;
	}

	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
		getInstance().distance = std::clamp(static_cast<float>(getInstance().distance - yoffset), 1.0f, 100.0f);
	};
};

>>>>>>> 65e49fd884fc33b59605b3036ff7b8ff8393947b
#endif