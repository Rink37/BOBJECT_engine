#ifndef INPUT_MANAGER
#define INPUT_MANAGER

#include <GLFW/glfw3.h>

#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
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

const std::map<char, char> upperCaseSpecial{ {49, 33}, {50, 34}, {52, 36}, { 53, 37 }, {54, 94}, {55, 38}, {56, 42}, {57, 40}, {48, 41}, {45, 95} };

struct KeyState {
public:
	bool isKeyDown = false;
	int eventType = NO_EVENT;

	void setIsKeyDown(int isDown) {
		isKeyDown = isDown;
	}

	void sortCallbacks() {
		for (auto it = callbacks.begin(); it != callbacks.end(); it++) {
			std::sort(it->second.begin(), it->second.end(), [](std::pair<std::vector<int>, std::function<void()>>a, std::pair<std::vector<int>, std::function<void()>>b) {return a.first.size() > b.first.size(); });
		}
	}

	std::map<int, std::vector<std::pair<std::vector<int>, std::function<void()>>>> callbacks{};
	// Callbacks are accessed by keycode and held keys
};

class KeyManager {
public:
	KeyManager() {
		KeyManager::_instances.push_back(this);
	}

	void initCallbacks(GLFWwindow* window);

	using Callback = std::function<void()>;

	void updateKey(int, std::function<void()>);
	void updateKey(int, int, std::function<void()>);
	void updateKey(int, int, std::vector<int>, std::function<void()>);
	void updateKey(int, std::vector<int>, std::function<void()>);

	void addBinding(int, const Callback&);
	void addBinding(int, const Callback&, int);
	void addBinding(int, const Callback&, std::vector<int>);
	void addBinding(int, const Callback&, int, std::vector<int>);

	void pollRepeatEvents();

	void checkForEvent(int, int);

	void setIsKeyDown(int, int);

private:
	bool pollRequired = false;

	std::map<int, KeyState> _KeyStates;

	static std::vector<KeyManager*> _instances;

	static void callback(GLFWwindow* window, int key, int scancode, int action, int mods);
};

class TypeManager {
public:
	TypeManager() {
		TypeManager::_instances.push_back(this);
		listening = false;
	}

	~TypeManager() {
		TypeManager::_instances.erase(find(TypeManager::_instances.begin(), TypeManager::_instances.end(), this));
	}

	void startListening() {
		listening = true;
	}

	void stopListening() {
		listening = false;
	}

	void setTypeCallback(std::function<void(char)> func) {
		typeCallback = func;
	}

	void initCallbacks(GLFWwindow* window);
private:
	static std::vector<TypeManager*> _instances;

	bool listening = false;

	bool shiftHeld = false;
	std::function<void(char)> typeCallback = nullptr;

	static void callback(GLFWwindow* window, int key, int scancode, int action, int mods);
};

struct Mouse {
	bool isRMBDown = false;
	bool isLMBDown = false;

	double xpos = 0.0f, ypos = 0.0f;
};

class MouseManager {
public:
	MouseManager() {
		MouseManager::_instances.push_back(this);
	}

	using Listener = std::function<bool(double, double, int)>;

	size_t addClickListener(const Listener&);

	size_t addPositionListener(const Listener&);

	void removeClickListener(size_t);
	void removePositionListener(size_t);

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

	static void cursorExitCallback(GLFWwindow*, int);

	Mouse mouse;
};

#endif