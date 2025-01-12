#pragma once

#ifndef INPUT_MANAGER
#define INPUT_MANAGER

#include <iostream>
#include<vector>
#include "windows.h"

class Key {
	public:
		char keyChar;
		int keyCode;

		void init(char);

		bool getState();
};

class bindSet {
private:
	std::vector<Key> keybinds;
public:
	void addKeybind(char);
	void getKeyStates();
};

namespace dir {
	int getHorizontalAxis();
	int getVerticalAxis();
	int getForwardAxis();
};

namespace keycommands {
	bool reloadModel();
	bool addModel();
};

#endif