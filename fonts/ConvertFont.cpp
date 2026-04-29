#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <vector>
#include <bits/stdc++.h> // No idea why this is claimed to be an error - it works fine
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

void loadAndWriteFile(string path, int colCount, int rowCount, int cellWidth, int cellHeight){
    int texWidth;
    int texHeight;
    int texChannels;
    string location = path.substr(0, path.find_last_of("/\\"));
    string name = path.substr(path.find_last_of("/\\")+1);
    string::size_type const p(name.find_last_of('.'));
    string nameNoExt = name.substr(0, p);
    string binName = nameNoExt + string("_advances.bin");
    string capNameNoExt = nameNoExt;
    std::transform(capNameNoExt.begin(), capNameNoExt.end(), capNameNoExt.begin(), ::toupper);
    unsigned char* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    ifstream advances;
    advances.open(location+string("\\")+binName, ios::binary | ios::in); 
    string outname = location+string("\\headers\\")+nameNoExt+string(".h");
    cout << outname << endl;
    cout << location + string("\\") + binName << endl;
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
    out << string("const float")+nameNoExt+string("Advances[] = {");
    for (int i = 0; i != 94; i++){
        float advance;
        advances.read((char*)(&advance), sizeof(float));
        cout << advance << endl;
        if (i < 93){
            out << to_string(advance) + string("f, ");
        } else {
            out << to_string(advance);
        }
    }
    out << "};\n\n";
    out << string("unsigned char ")+nameNoExt+string("Bytes[] = { ");
    //cout << texChannels << endl;
    size_t imagesize = texWidth*texHeight*4;
    for (size_t i = 0; i!= imagesize; i += 5-texChannels){
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

int main(int argc, char* argv[]){
    loadAndWriteFile("C:\\Users\\robda\\Documents\\Trad_Painter\\fonts\\coolvetica_sdf.png", 11, 9, 46, 53);
    return 0;
}