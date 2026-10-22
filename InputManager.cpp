#include "windows.h"
#include<iostream>
#include<vector>
#include "GLFW/glfw3.h"

#include "InputManager.h"

using namespace std;

void KeyManager::addBinding(int key, const Callback& callback) {
	updateKey(key, callback);
}

void KeyManager::addBinding(int key, const Callback& callback, int eventType) {
	if (eventType == HOLD_EVENT && !pollRequired) {
		pollRequired = true;
	}
	updateKey(key, eventType, callback);
}

void KeyManager::addBinding(int key, const Callback& callback, std::vector<int> requirements) {
	updateKey(key, requirements, callback);
}

void KeyManager::addBinding(int key, const Callback& callback, int eventType, std::vector<int> requirements) {
	updateKey(key, eventType, requirements, callback);
}

void KeyManager::pollRepeatEvents() {
	if (!pollRequired) {
		return;
	}
	for (std::map<int, KeyState>::iterator it = _KeyStates.begin(); it != _KeyStates.end(); ++it) {
		int key = it->first;
		if (_KeyStates[key].callbacks.contains(HOLD_EVENT)) {
			checkForEvent(key, HOLD_EVENT);
		}
	}
}

void KeyManager::checkForEvent(int key, int eventType) {
	if (_KeyStates[key].callbacks.count(eventType) > 0) {
		for (std::pair<std::vector<int>, std::function<void()>> elem : _KeyStates[key].callbacks.at(eventType)) {
			if (_KeyStates[key].context.size() > 0 && _KeyStates[key].context != elem.first) {
				continue;
			}
			bool conditionsMet = true;
			for (int code : elem.first) {
				if (!_KeyStates[code].isKeyDown) {
					conditionsMet = false;
					break;
				}
			}
			if (conditionsMet) {
				elem.second();
				if (eventType == PRESS_EVENT) {
					_KeyStates[key].context = elem.first;
				} else if(eventType == RELEASE_EVENT) {
					_KeyStates[key].context.clear();
				}
				return;
			}
		}
	}
	if (eventType == RELEASE_EVENT) {
		_KeyStates[key].context.clear();
	}
}

void KeyManager::setIsKeyDown(int key, int action) {
	std::map<int, KeyState>::iterator it = _KeyStates.find(key);
	if (it == _KeyStates.end()) {
		return;
	}
	if (action != GLFW_REPEAT) {
		_KeyStates[key].setIsKeyDown(action != GLFW_RELEASE);
		int eventType = NO_EVENT;
		if (action == GLFW_RELEASE) {
			eventType = RELEASE_EVENT;
		}
		else {
			eventType = PRESS_EVENT;
		}
		checkForEvent(key, eventType);
	}
}

void KeyManager::updateKey(int keyCode, std::function<void()> callback) {
	updateKey(keyCode, PRESS_EVENT, callback);
}

void KeyManager::updateKey(int keyCode, int eventType, std::function<void()> callback) {
	updateKey(keyCode, eventType, std::vector<int>{}, callback);
}

void KeyManager::updateKey(int keyCode, std::vector<int> requirement, std::function<void()> callback) {
	updateKey(keyCode, PRESS_EVENT, requirement, callback);
}

void KeyManager::updateKey(int keyCode, int eventType, std::vector<int> requirement, std::function<void()> callback) {
	for (auto requiredKey : requirement) {
		if (_KeyStates.count(requiredKey) == 0) {
			KeyState requiredKeyState;
			_KeyStates.insert({ requiredKey, requiredKeyState });
		}
	}
	std::map<int, KeyState>::iterator it = _KeyStates.find(keyCode);
	if (it == _KeyStates.end()) {
		KeyState keyState;
		keyState.callbacks.insert({ eventType, std::vector<std::pair<std::vector<int>, std::function<void()>>>{ {requirement, callback} } });
		keyState.sortCallbacks();
		_KeyStates.insert({ keyCode, keyState });
	}
	else {
		KeyState* keyState = &_KeyStates.at(keyCode);
		std::map<int, std::vector<std::pair<std::vector<int>, std::function<void()>>>>::iterator state_it = keyState->callbacks.find(eventType);
		if (state_it == keyState->callbacks.end()) {
			keyState->callbacks.insert({ eventType, std::vector < std::pair<std::vector<int>, std::function<void()>>>{ {requirement, callback} } });
		}
		else {
			keyState->callbacks.at(eventType).push_back({ requirement, callback });
		}
		keyState->sortCallbacks();
	}
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

void TypeManager::initCallbacks(GLFWwindow* window) {
	glfwSetKeyCallback(window, callback);
}

void TypeManager::callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) {
		for (TypeManager* typeManager : _instances) {
			typeManager->shiftHeld = (action != GLFW_RELEASE);
		}
		return;
	}
	if (key == GLFW_KEY_CAPS_LOCK && action == GLFW_PRESS) {
		for (TypeManager* typeManager : _instances) {
			typeManager->shiftHeld = !typeManager->shiftHeld;
		}
		return;
	}
	if (action == GLFW_PRESS) {
		for (TypeManager* typeManager : _instances) {
			if (!typeManager->listening) {
				continue;
			}
			if (typeManager->typeCallback != nullptr) {
				char cKey = static_cast<char>(key);
				if (!typeManager->shiftHeld) {
					if (GLFW_KEY_A <= key && key <= GLFW_KEY_Z) {
						cKey += 32;
					}
				}
				else if (upperCaseSpecial.count(cKey) != 0) {
					cKey = upperCaseSpecial.at(cKey);
				}
				typeManager->typeCallback(cKey);
			}
		}
	}
}

void MouseManager::initCallbacks(GLFWwindow* window) {
	glfwSetMouseButtonCallback(window, callback);
	glfwSetCursorEnterCallback(window, cursorExitCallback);
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

void MouseManager::cursorExitCallback(GLFWwindow* window, int entered) {
	if (!entered) {
		for (MouseManager* mouseManager : _instances) {
			mouseManager->updateMouseState(GLFW_MOUSE_BUTTON_RIGHT, false, mouseManager->mouse.xpos, mouseManager->mouse.ypos);
			mouseManager->updateMouseState(GLFW_MOUSE_BUTTON_LEFT, false, mouseManager->mouse.xpos, mouseManager->mouse.ypos);
		}
	}
}

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