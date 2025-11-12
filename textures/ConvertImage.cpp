#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <vector>
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
    out << string("#ifndef ")+capNameNoExt+string("BYTES\n");
    out << string("#define ")+capNameNoExt+string("BYTES\n");
    out << string("#include \"ImageDataType.h\"\n");
    out << string("\n");
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
    out << string("#endif\n");
    out << string("#ifndef ")+capNameNoExt+string("\n");
    out << string("#define ")+capNameNoExt+string(" imageData(")+nameNoExt+string("Width, ")+nameNoExt+string("Height, ")+nameNoExt+string("Channels, ")+nameNoExt+string("Bytes )\n\n");

    out << string("#endif");
    out.close();
}

void writeSingle(string outhname, string outcppname, string name, string location){
    ofstream outh(outhname.c_str(), std::ios_base::app);
    ofstream outcpp(outcppname.c_str(), std::ios_base::app);
    int texWidth;
    int texHeight;
    int texChannels;
    string::size_type const p(name.find_last_of('.'));
    string nameNoExt = name.substr(0, p);
    string capNameNoExt = nameNoExt;
    string path = location + name;
    cout << path << endl;
    std::transform(capNameNoExt.begin(), capNameNoExt.end(), capNameNoExt.begin(), ::toupper);
    unsigned char* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    outcpp << string("const int ")+nameNoExt+string("Width = ");
    outcpp << texWidth;
    outcpp << ";\n\n";
    outcpp << string("const int ")+nameNoExt+string("Height = ");
    outcpp << texHeight;
    outcpp << ";\n\n";
    outcpp << string("const int ")+nameNoExt+string("Channels = ");
    outcpp << texChannels;
    outcpp << ";\n\n";
    size_t imagesize = texWidth*texHeight*4;
    outcpp << string("unsigned char ")+nameNoExt+string("Bytes[] = { ");
    cout << texChannels << endl;
    for (size_t i = 0; i!= imagesize; i += 5-texChannels){
        outcpp << "0x" << hex << setw(2) << setfill('0') << (int)(unsigned char) pixels[i];
        if (i < imagesize-1){
            outcpp << ", ";
        }
    }
    outcpp << string(" };\n\n");

    outh << string("extern const int ")+nameNoExt+string("Width;\n");
    outh << string("extern const int ")+nameNoExt+string("Height;\n");
    outh << string("extern const int ")+nameNoExt+string("Channels;\n");
    outh << string("extern unsigned char ")+nameNoExt+string("Bytes[];\n\n");
    outh << string("#define ")+capNameNoExt+string(" imageData(")+nameNoExt+string("Width, ")+nameNoExt+string("Height, ")+nameNoExt+string("Channels, ")+nameNoExt+string("Bytes )\n\n");
    delete[] pixels;
    outh.close();
    outcpp.close();
}

void loadAndWriteFiles(string location, vector<string> fnames){
    string outhname = location+string("\\headers\\BakedImages.h");
    string outcppname = location+string("\\headers\\BakedImages.cpp");
    ofstream outh(outhname.c_str());
    ofstream outcpp(outcppname.c_str());

    outh << string("#ifndef BAKED_IMAGES\n");
    outh << string("#define BAKED_IMAGES\n\n");
    outh << string("#include \"ImageDataType.h\"\n");
    outcpp << string("#include\"BakedImages.h\"\n\n");

    outh.close();
    outcpp.close();

    for (size_t i = 0; i != fnames.size(); i++){
        writeSingle(outhname, outcppname, fnames[i], location);
    }
    ofstream outh2(outhname.c_str(), std::ios_base::app);
    outh2 << string("#endif");
    outh2.close();
    //outcpp.close();
}

int main(int argc, char* argv[]){
    if (argc == 1){
        string base = string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\");
        vector<string> fnames;
        //string fname = string("Webcam_frame.jpeg");
        //fnames.push_back(fname);
        string fname = string("LoadButton.png");
        fnames.push_back(fname);
        //loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("WireframeButton.png");
        fnames.push_back(fname);
        //loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("PauseButton.png");
        fnames.push_back(fname);
        //loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("PlayButton.png");
        fnames.push_back(fname);
        //loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("RenderedButton.png");
        fnames.push_back(fname);
        //loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("SettingsButton.png");
        fnames.push_back(fname);
        //loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("TestCheckboxButton.png");
        fnames.push_back(fname);
        //loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("UnrenderedButton.png");
        fnames.push_back(fname);
        //loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("D2NButton.png");
        fnames.push_back(fname);
        //loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("DiffuseText.png");
        fnames.push_back(fname);
        //loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("NormalText.png");
        fnames.push_back(fname);
        //loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("OpenButton.png");
        fnames.push_back(fname);
        //loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("PlusButton.png");
        fnames.push_back(fname);
        //loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("SaveButton.png");
        fnames.push_back(fname);
        //loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("TangentSpace.png");
        fnames.push_back(fname);
        //loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("OSButton.png");
        fnames.push_back(fname);
        //loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("WebcamOffButton.png");
        fnames.push_back(fname);
        //loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("WebcamOnButton.png");
        fnames.push_back(fname);
        //loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("WebcamViewButton.png");
        fnames.push_back(fname);
        //loadAndWriteFile(string("C:\\Users\\robda\\Documents\\VulkanRenderer\\textures\\")+fname);
        fname = string("CancelButton.png");
        fnames.push_back(fname);
        fname = string("FinishButton.png");
        fnames.push_back(fname);
        fname = string("EdgeSharpnessText.png");
        fnames.push_back(fname);
        fname = string("SearchSizeText.png");
        fnames.push_back(fname);
        fname = string("StrokeFlatnessText.png");
        fnames.push_back(fname);
        fname = string("FlattenThresholdText.png");
        fnames.push_back(fname);
        fname = string("NoiseRemovalText.png");
        fnames.push_back(fname);
        
        loadAndWriteFiles(base, fnames);
        return 0;
    }
    loadAndWriteFile(string(argv[1]));
    return 0;
}