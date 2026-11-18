#ifndef WIN_FILE_MANAGER
#define WIN_FILE_MANAGER

#include <windows.h>
#include <string>
#include <iostream>

namespace winFile {
	std::string OpenFileDialog();
	std::string SaveFileDialog();

	std::string getBaseName(std::string);
	std::string getExtension(std::string);
	void stripExtension(std::string&);
};
#endif