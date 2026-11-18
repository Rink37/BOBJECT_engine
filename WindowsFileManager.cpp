#include "WindowsFileManager.h"

HWND hwnd;

std::string winFile::OpenFileDialog() {
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
		return std::string(ofn.lpstrFile);
	}
	std::string fail =  "fail";
	return fail;
}

std::string winFile::SaveFileDialog() {
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
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

	if (GetSaveFileName(&ofn) == TRUE) {
		return std::string(ofn.lpstrFile);
	}
	std::string fail = "fail";
	return fail;
}

std::string winFile::getBaseName(std::string filePath) {
	// Returns the substring which is found after the last path separator in the filePath
	std::string del = "\\";
	std::string basename = filePath;
	auto pos = basename.find(del); // Find each delimeter in the path and erase all characters up to each pos at each iteration
	while (pos != std::string::npos) {
		basename.erase(0, pos + del.length());
		pos = basename.find(del);
	}
	return basename;
}

std::string winFile::getExtension(std::string fileName) {
	// Returns the substring which is found after the last "." in the fileName
	std::string del = ".";
	std::string ext = fileName;
	auto pos = ext.find(del); // Find each delimeter in the path and erase all characters up to each pos at each iteration
	while (pos != std::string::npos) {
		ext.erase(0, pos + del.length());
		pos = ext.find(del);
	}
	return ext;
};

void winFile::stripExtension(std::string& fileName) {
	// Modifies the string in place so that the last "." and substring after this character are removed
	std::string ext = winFile::getExtension(fileName);
	fileName.erase(fileName.end() - ext.size() - 1, fileName.end());
};