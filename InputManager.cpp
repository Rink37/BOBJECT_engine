#include "windows.h"
#include<iostream>
#include<vector>
#include "InputManager.h"

using namespace std;

void Key::init(char targetKeyChar) {
	Key::keyChar = targetKeyChar;
	Key::keyCode = (int)Key::keyChar;
}

bool Key::getState() {
	return (GetAsyncKeyState(Key::keyCode) & 0x8000);
}

void bindSet::addKeybind(char targetKeyChar) {
	Key keybind;
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