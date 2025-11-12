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

void MouseManager::initCallbacks(GLFWwindow* window) {
	glfwSetMouseButtonCallback(window, callback);
	for (MouseManager* mouseManager : _instances) {
		mouseManager->windowArea = window;
	}
}

void MouseManager::callback(GLFWwindow* window, int button, int action, int mods) {
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	for (MouseManager* mouseManager : _instances) {
		mouseManager->updateMouseState(button, action != GLFW_RELEASE, xpos, ypos);
	}
};

void MouseManager::updateMouseState(int key, bool state, double xpos, double ypos) {
	int clickCode = MOUSE_HOVER;
	if (key == GLFW_MOUSE_BUTTON_LEFT) {
		mouse.isLMBDown = state;
		if (state) {
			clickCode = LMB_PRESS;
		}
		else {
			clickCode = LMB_RELEASE;
		}
	}
	else if (key == GLFW_MOUSE_BUTTON_RIGHT) {
		mouse.isRMBDown = state;
		if (state) {
			clickCode = RMB_PRESS;
		}
		else {
			clickCode = RMB_RELEASE;
		}
	}
	mouse.xpos = xpos;
	mouse.ypos = ypos;
	checkClickEvents(clickCode);
}

size_t MouseManager::addClickListener(const Listener& listener) {
	_ClickListeners.push_back(listener);
	return _ClickListeners.size() - 1;
}

size_t MouseManager::addPositionListener(const Listener& listener) {
	_PositionListeners.push_back(listener);
	return _PositionListeners.size() - 1;
}

void MouseManager::removeClickListener(size_t index) {
	_ClickListeners.erase(_ClickListeners.begin() + index);
}

void MouseManager::removePositionListener(size_t index) {
	_PositionListeners.erase(_PositionListeners.begin() + index);
}

void MouseManager::checkClickEvents(int clickCode) {
	for (auto listener : _ClickListeners) {
		if (listener(mouse.xpos, mouse.ypos, clickCode)) {
			break;
		};
	}
}

void MouseManager::checkPositionEvents() {
	glfwGetCursorPos(windowArea, &mouse.xpos, &mouse.ypos);
	int posCode = MOUSE_HOVER;
	if (mouse.isLMBDown) {
		posCode = LMB_HOLD;
	}
	else if (mouse.isRMBDown) {
		posCode = RMB_HOLD;
	}

	for (auto listener : _PositionListeners) {
		if (listener(mouse.xpos, mouse.ypos, posCode)) {
			break;
		};
	}
}