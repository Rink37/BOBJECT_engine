#pragma once

#ifndef WIN_FILE_MANAGER
#define WIN_FILE_MANAGER

#include <windows.h>
#include <string.h>
#include <iostream>

namespace winFile {
	std::string OpenFileDialog();
	std::string SaveFileDialog();
};

#endif