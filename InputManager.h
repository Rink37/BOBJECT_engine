#pragma once

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
