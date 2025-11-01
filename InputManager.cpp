#include "windows.h"
#include<iostream>
#include<vector>
#include "GLFW/glfw3.h"

#include "InputManager.h"

using namespace std;

void KeyManager::addBinding(int key, const Callback& callback, int eventType) {
	if (eventType == HOLD_EVENT && !pollRequired) {
		pollRequired = true;
	}
	std::map<int, KeyState>::iterator it = _KeyStates.find(key);
	if (it == _KeyStates.end()) {
		addKey(key);
	}
	_KeyStates[key].eventType = eventType;
	_Callbacks.insert({ key, callback });
}

void KeyManager::addBinding(int key, const Callback& callback) {
	std::map<int, KeyState>::iterator it = _KeyStates.find(key);
	if (it == _KeyStates.end()) {
		addKey(key);
	}
	_KeyStates[key].eventType = PRESS_EVENT;
	_Callbacks.insert({ key, callback });
}

void KeyManager::pollRepeatEvents() {
	if (!pollRequired) {
		return;
	}
	for (std::map<int, KeyState>::iterator it = _KeyStates.begin(); it != _KeyStates.end(); ++it) {
		int key = it->first;
		if (_KeyStates[key].eventType == HOLD_EVENT && _KeyStates[key].isKeyDown) {
			_Callbacks[key]();
		}
	}
}

void KeyManager::checkForEvent(int key) {
	switch (_KeyStates[key].eventType) {
	case PRESS_EVENT:
		if (_KeyStates[key].isKeyDown) {
			_Callbacks[key]();
		}
		break;
	case RELEASE_EVENT:
		if (!_KeyStates[key].isKeyDown) {
			_Callbacks[key]();
		}
		break;
	default:
		break;
	}
}

void KeyManager::setIsKeyDown(int key, int action) {
	std::map<int, KeyState>::iterator it = _KeyStates.find(key);
	if (it != _KeyStates.end() && action != GLFW_REPEAT) {
		_KeyStates[key].setIsKeyDown(action != GLFW_RELEASE);
	}
	checkForEvent(key);
}

void KeyManager::addKey(int key) {
	KeyState keystate;
	_KeyStates.insert({ key, keystate });
}

void KeyManager::initCallbacks(GLFWwindow* window) {
	glfwSetKeyCallback(window, callback);
}

void KeyManager::callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action != GLFW_REPEAT) {
		for (KeyManager* keyManager : _instances) {
			keyManager->setIsKeyDown(key, action != GLFW_RELEASE);
		}	
	}
}
