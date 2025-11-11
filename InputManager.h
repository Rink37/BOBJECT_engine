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

#define RMB_PRESS 0
#define RMB_HOLD 1
#define RMB_RELEASE 2
#define LMB_PRESS 3
#define LMB_HOLD 4
#define LMB_RELEASE 5
#define MOUSE_HOVER 6

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

struct Mouse {
	bool isRMBDown = false;
	bool isLMBDown = false;

	double xpos, ypos;
};

class MouseManager {
public:
	MouseManager() {
		MouseManager::_instances.push_back(this);
	}

	using Listener = std::function<bool(double, double, int)>;

	int addClickListener(const Listener&);

	int addPositionListener(const Listener&);

	void removeClickListener(const Listener&);
	void removePositionListener(const Listener&);

	void checkClickEvents(int);
	void checkPositionEvents();
	
	void initCallbacks(GLFWwindow* window);

	void updateMouseState(int, bool, double, double);
private:
	GLFWwindow* windowArea = nullptr;

	std::vector<Listener> _ClickListeners; // These care only about press/release events and where they occur
	std::vector<Listener> _PositionListeners; // These care only about where the mouse is and what state each key is in

	static std::vector<MouseManager*> _instances;

	static void callback(GLFWwindow*, int, int, int);

	Mouse mouse;
};

#endif