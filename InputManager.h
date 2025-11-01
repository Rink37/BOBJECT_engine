#ifndef INPUT_MANAGER
#define INPUT_MANAGER

#include <GLFW/glfw3.h>

#include <iostream>
#include <map>
#include <vector>
#include <functional>

#define NO_EVENT 0
#define PRESS_EVENT 1
#define RELEASE_EVENT 2
#define HOLD_EVENT 3

struct KeyState {
public:
	bool isKeyDown = false;
	int eventType = NO_EVENT;

	void setIsKeyDown(int isDown) {
		isKeyDown = isDown;
	}
};

class KeyManager {
public:
	KeyManager() {
		KeyManager::_instances.push_back(this);
	}

	void initCallbacks(GLFWwindow* window);

	using Callback = std::function<void()>;

	void addKey(int);

	void addBinding(int, const Callback&, int);

	void addBinding(int, const Callback&);

	void pollRepeatEvents();

	void checkForEvent(int);

	void setIsKeyDown(int, int);

private:
	bool pollRequired = false;

	std::map<int, Callback> _Callbacks;
	std::map<int, KeyState> _KeyStates;

	static std::vector<KeyManager*> _instances;

	static void callback(GLFWwindow* window, int key, int scancode, int action, int mods);
};

#endif