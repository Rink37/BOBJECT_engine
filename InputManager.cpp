#include "windows.h"
#include<iostream>
#include<vector>
#include "GLFW/glfw3.h"

#include "InputManager.h"

using namespace std;

void Keybind::init(char targetKeyChar) {
	Keybind::keyChar = targetKeyChar;
	Keybind::keyCode = (int)Keybind::keyChar;
}

bool Keybind::getState() {
	return (GetAsyncKeyState(Keybind::keyCode) & 0x8000);
}

void bindSet::addKeybind(char targetKeyChar) {
	Keybind keybind;
	keybind.init(targetKeyChar);
	bindSet::keybinds.push_back(keybind);
}

void bindSet::getKeyStates() {
	for (int i = 0; i != bindSet::keybinds.size(); i++) {
		if (bindSet::keybinds[i].getState()) {
			cout << bindSet::keybinds[i].keyChar << " pressed!" << endl;
		}
	}
	Sleep(100);
}

int dir::getHorizontalAxis() {
	if (GetAsyncKeyState((int)'A') & 0x8000) {
		return -1;
	}
	else if (GetAsyncKeyState((int)'D') & 0x8000) {
		return 1;
	} 
	return 0;
}

int dir::getForwardAxis() {
	if (GetAsyncKeyState((int)'S') & 0x8000) {
		return -1;
	}
	else if (GetAsyncKeyState((int)'W') & 0x8000) {
		return 1;
	}
	return 0;
}

int dir::getVerticalAxis() {
	if (GetAsyncKeyState(VK_LCONTROL) & 0x8000) {
		return -1;
	}
	else if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
		return 1;
	}
	return 0;
}

bool keycommands::reloadModel() {
	if (GetAsyncKeyState((int)'R') & 0x8000) {
		return true;
	}
	else {
		return false;
	}
}

bool keycommands::addModel() {
	if (GetAsyncKeyState((int)'T') & 0x8000) {
		return true;
	}
	else {
		return false;
	}
}

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