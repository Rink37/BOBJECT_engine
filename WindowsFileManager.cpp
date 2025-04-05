#include <windows.h>
#include <string.h>
#include <iostream>

#include "WindowsFileManager.h"

using namespace std;

HWND hwnd;

string winFile::OpenFileDialog() {
	OPENFILENAME ofn = { 0 };
	TCHAR szFile[260] = { 0 };
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn) == TRUE) {
		return string(ofn.lpstrFile);
	}
	string fail =  "fail";
	return fail;
}