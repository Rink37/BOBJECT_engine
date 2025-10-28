#include "FileManager.h"
#include "tinyfiledialogs.h"
#include <string>

std::string FileManager::OpenFileDialog(const char* defaultPath, const char* filterPatterns) {
    const char* filterDesc = "All Files";
    const char* result = tinyfd_openFileDialog(
        "Open File",           // dialog title
        defaultPath,           // default path
        1,                     // number of filter patterns
        &filterPatterns,       // filter patterns
        filterDesc,           // filter description
        0                      // allow multiple selects?
    );
    
    return result ? std::string(result) : "fail";
}

std::string FileManager::SaveFileDialog(const char* defaultPath, const char* filterPatterns) {
    const char* filterDesc = "All Files";
    const char* result = tinyfd_saveFileDialog(
        "Save File",           // dialog title
        defaultPath,           // default path
        1,                     // number of filter patterns
        &filterPatterns,       // filter patterns
        filterDesc            // filter description
    );
    
    return result ? std::string(result) : "fail";
}

std::string FileManager::SelectFolderDialog(const char* defaultPath) {
    const char* result = tinyfd_selectFolderDialog(
        "Select Folder",       // dialog title
        defaultPath            // default path
    );
    
    return result ? std::string(result) : "fail";
}
