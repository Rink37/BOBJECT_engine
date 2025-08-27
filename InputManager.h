//#include "Bobject_Engine.h"

#ifndef INPUT_MANAGER
#define INPUT_MANAGER

#include <GLFW/glfw3.h>

#include <iostream>
#include <map>
#include <vector>
#include "windows.h"

class KeyInput {
public:
	KeyInput(std::vector<int> keysToMonitor); // Creates a KeyInput instance which monitors state of keysToMonitor
	~KeyInput(); // Remove this instance from the list of instances

	void addMonitoredKey(int key); // Adds a key to the set of keys monitored by the KeyInput

	bool getIsKeyDown(int key); // Check state of specified key

	bool getIsEnabled() { return _isEnabled; }
	void setIsEnabled(bool value) { _isEnabled = value; }

private:
	void setIsKeyDown(int key, bool isDown); // Updates key states using GLFW callback

	std::map<int, bool>_keys; // Maps keys to their current active states

	bool _isEnabled; // Used to check if the active Keyboard Input instance is enabled

public:
	static void setupKeyInputs(GLFWwindow* window); // Call at engine initialisation, required to setup keyboard

private:
	static void callback(GLFWwindow* window, int key, int scancode, int action, int mods); // GLFW callback

	static std::vector<KeyInput*> _instances;
};

#endif