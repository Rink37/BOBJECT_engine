<<<<<<< HEAD
#include "windows.h"
#include<iostream>
#include<vector>
#include "GLFW/glfw3.h"

#include "InputManager.h"

using namespace std;

KeyInput::KeyInput(std::vector<int> keysToMonitor) : _isEnabled(true) {
	for (int key : keysToMonitor) {
		_keys[key] = false;
	}
	KeyInput::_instances.push_back(this);
}

KeyInput::~KeyInput() {
	_instances.erase(std::remove(_instances.begin(), _instances.end(), this), _instances.end());
}

void KeyInput::addMonitoredKey(int key) {
	_keys[key] = false;
}

bool KeyInput::getIsKeyDown(int key) {
	bool result = false;
	if (_isEnabled) {
		std::map<int, bool>::iterator it = _keys.find(key);
		if (it != _keys.end()) {
			result = _keys[key];
		}
	}
	return result;
}

void KeyInput::setIsKeyDown(int key, bool isDown) {
	std::map<int, bool>::iterator it = _keys.find(key);
	if (it != _keys.end()) {
		_keys[key] = isDown;
	}
}

void KeyInput::setupKeyInputs(GLFWwindow* window) {
	glfwSetKeyCallback(window, KeyInput::callback);
}

void KeyInput::callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	for (KeyInput* keyInput : _instances) {
		keyInput->setIsKeyDown(key, action != GLFW_RELEASE);
	}
}

=======
#include "windows.h"
#include<iostream>
#include<vector>
#include "GLFW/glfw3.h"

#include "InputManager.h"

using namespace std;

KeyInput::KeyInput(std::vector<int> keysToMonitor) : _isEnabled(true) {
	for (int key : keysToMonitor) {
		_keys[key] = false;
	}
	KeyInput::_instances.push_back(this);
}

KeyInput::~KeyInput() {
	_instances.erase(std::remove(_instances.begin(), _instances.end(), this), _instances.end());
}

void KeyInput::addMonitoredKey(int key) {
	_keys[key] = false;
}

bool KeyInput::getIsKeyDown(int key) {
	bool result = false;
	if (_isEnabled) {
		std::map<int, bool>::iterator it = _keys.find(key);
		if (it != _keys.end()) {
			result = _keys[key];
		}
	}
	return result;
}

void KeyInput::setIsKeyDown(int key, bool isDown) {
	std::map<int, bool>::iterator it = _keys.find(key);
	if (it != _keys.end()) {
		_keys[key] = isDown;
	}
}

void KeyInput::setupKeyInputs(GLFWwindow* window) {
	glfwSetKeyCallback(window, KeyInput::callback);
}

void KeyInput::callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	for (KeyInput* keyInput : _instances) {
		keyInput->setIsKeyDown(key, action != GLFW_RELEASE);
	}
}

>>>>>>> 65e49fd884fc33b59605b3036ff7b8ff8393947b
