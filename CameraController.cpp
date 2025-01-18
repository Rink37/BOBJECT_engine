#include "InputManager.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtx/hash.hpp>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include<GLFW/glfw3native.h>

#include <iostream>
#include <algorithm>

#include "CameraController.h"

using namespace glm;


void Camera::updateCamera(GLFWwindow* window) {
	radius = glm::length(pos);
	float scalefac = pow(radius, 2) / defaultscale;
	if (scalefac > 2) {
		scalefac = 2;
	}
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
	int shiftState = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT);
	if (state == GLFW_PRESS && shiftState != GLFW_PRESS) {
		if (!isRotating) {
			glfwGetCursorPos(window, &initXpos, &initYpos);
			initxrot = xrot;
			inityrot = yrot;
			isRotating = true;
		}
		glfwGetCursorPos(window, &activeXpos, &activeYpos);
		xrot = initxrot + static_cast<float>(activeXpos - initXpos);
		yrot = std::clamp(inityrot + static_cast<float>(activeYpos - initYpos), -80.0f, 80.0f);
	}
	else if (state == GLFW_PRESS && shiftState == GLFW_PRESS) {
		if (!isPanning) {
			glfwGetCursorPos(window, &initXpos, &initYpos);
			initCenter = center;
			isPanning = true;
		}
		velocity = defaultVelocity * distance/defaultDistance;
		glfwGetCursorPos(window, &activeXpos, &activeYpos);
		center = initCenter + vec3(0.0f, static_cast<float>(initXpos - activeXpos) * velocity, static_cast<float>(activeYpos - initYpos) * velocity) * mat3(glm::rotate(mat4(1.0f), glm::radians(yrot), glm::vec3(0.0f, 1.0f, 0.0f))) * mat3(glm::rotate(mat4(1.0f), glm::radians(xrot), glm::vec3(0.0f, 0.0f, 1.0f)));
	}
	else {
		if (isRotating) {
			isRotating = false;
		}
		if (isPanning) {
			isPanning = false;
		}
	}
	distance = pow(getInstance().distance, 2)/defaultDistance;
	pos = glm::vec3(distance, 0.0f, 0.0f) * mat3(glm::rotate(mat4(1.0f), glm::radians(yrot), glm::vec3(0.0f, 1.0f, 0.0f))) * mat3(glm::rotate(mat4(1.0f), glm::radians(xrot), glm::vec3(0.0f, 0.0f, 1.0f))) + center;
	view = lookAt(pos, center, vec3(0.0f, 0.0f, 1.0f));
}