#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include <iomanip>
#include <bits/stdc++.h> // No idea why this is claimed to be an error - it works fine

using namespace std;

static std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}
	size_t fileSize = (size_t)file.tellg();
	vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

void loadAndWriteShaders(string basepath, string shadername, bool wireframe){
    string vertPath = basepath + shadername + string("vert.spv");
	string fragPath = basepath + shadername + string("frag.spv");
	vector<char> vertData = readFile(vertPath);
	vector<char> fragData = readFile(fragPath);
	string capShaderName = shadername;
    std::transform(capShaderName.begin(), capShaderName.end(), capShaderName.begin(), ::toupper);
	ofstream out(basepath+shadername+string(".h"));
	out << string("#ifndef ")+capShaderName+string("DATA\n");
	out << string("#define ")+capShaderName+string("DATA\n");

	out << string("#include \"ShaderDataType.h\"\n\n");
	out << string("const std::vector<unsigned char> ")+shadername+string("vertData = { ");
	for (size_t i = 0; i!= vertData.size(); i++){
		out << "0x" << hex << setw(2) << setfill('0') << (int)(unsigned char) vertData[i];
		if (i < vertData.size()-1){
			out << ", ";
		}
	}
	out << string(" };\n\n");
	out << string("const std::vector<unsigned char> ")+shadername+string("fragData = { ");
	for (size_t i = 0; i!= fragData.size(); i++){
		out << "0x" << hex << setw(2) << setfill('0') << (int)(unsigned char) fragData[i];
		if (i < fragData.size()-1){
			out << ", ";
		}
	}
	out << string(" };\n\n");
	if (wireframe){
		out << string("const bool ")+shadername+string("Wireframe = true;\n\n");
	} else {
		out << string("const bool ")+shadername+string("Wireframe = false;\n\n");
	}

	out << string("#endif\n\n");

	out << string("#ifndef ") + capShaderName + string("SHADER\n");
	out << string("#define ") + capShaderName + string("SHADER shaderData( ")+shadername+string("fragData, ")+shadername+string("vertData, ")+shadername+string("Wireframe )\n");
	out << string("#endif\n");
	out.close();
}

int main(){
	string basepath = string("C:/Users/robda/Documents/VulkanRenderer/shaders/");
	loadAndWriteShaders(basepath, string("Flat"), 0);
	loadAndWriteShaders(basepath, string("BF"), 0);
	loadAndWriteShaders(basepath, string("UI"), 0);
	loadAndWriteShaders(basepath, string("UV"), 1);
	loadAndWriteShaders(basepath, string("W"), 1);
	return 0;
};