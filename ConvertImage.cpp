#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <bits/stdc++.h> // No idea why this is claimed to be an error - it works fine
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

void loadAndWriteFile(string path){
    int texWidth;
    int texHeight;
    int texChannels;
    string location = path.substr(0, path.find_last_of("/\\"));
    string name = path.substr(path.find_last_of("/\\")+1);
    string::size_type const p(name.find_last_of('.'));
    string nameNoExt = name.substr(0, p);
    string capNameNoExt = nameNoExt;
    std::transform(capNameNoExt.begin(), capNameNoExt.end(), capNameNoExt.begin(), ::toupper);
    unsigned char* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    string outname = location+string("\\headers\\")+nameNoExt+string(".h");
    cout << outname << endl;
    ofstream out(outname.c_str());
    out << string("#include \"ImageDataType.h\"\n");
    out << string("\n");
    //out << string("struct ")+nameNoExt+string(" : public imageData {\n");
    out << string("const int ")+nameNoExt+string("Width = ");
    out << texWidth;
    out << ";\n\n";
    out << string("const int ")+nameNoExt+string("Height = ");
    out << texHeight;
    out << ";\n\n";
    out << string("const int ")+nameNoExt+string("Channels = ");
    out << texChannels;
    out << ";\n\n";
    out << string("unsigned char ")+nameNoExt+string("Bytes[] = { ");
    size_t imagesize = texWidth*texHeight*texChannels;
    for (size_t i = 0; i!= imagesize; i++){
        out << "0x" << hex << setw(2) << setfill('0') << (int)(unsigned char) pixels[i];
        if (i < imagesize-1){
            out << ", ";
        }
    }
    out << string(" };\n\n");

    out << string("#ifndef ")+capNameNoExt+string("\n");
    out << string("#define ")+capNameNoExt+string(" imageData(")+nameNoExt+string("Width, ")+nameNoExt+string("Height, ")+nameNoExt+string("Channels, ")+nameNoExt+string("Bytes )\n\n");
    //out << string("};\n\n");

    out << string("#endif");
    out.close();
}

int main(int argc, char* argv[]){
    if (argc == 1){
        string fname = string("WireframeButton.png");
        loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("LoadButton.png");
        loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("PauseButton.png");
        loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("PlayButton.png");
        loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("RenderedButton.png");
        loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("SettingsButton.png");
        loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("TestCheckboxButton.png");
        loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("UnrenderedButton.png");
        loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        return 0;
    }
    loadAndWriteFile(string(argv[1]));
    return 0;
}