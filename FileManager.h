#pragma once
#include <string>

class FileManager {
public:
  static std::string OpenFileDialog(const char *defaultPath = "",
                                    const char *filterPatterns = "*.txt");
  static std::string SaveFileDialog(const char *defaultPath = "",
                                    const char *filterPatterns = "*.txt");
  static std::string SelectFolderDialog(const char *defaultPath = "");
};
