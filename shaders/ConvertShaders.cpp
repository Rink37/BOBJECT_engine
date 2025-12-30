#include<iostream>
#include<filesystem>
#include<fstream>
#include<string>
#include<vector>
#include<map>
#include<iomanip>
#include<Windows.h>
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

std::string readHelperFile(const std::string& filename, const std::string& shadername){
	//std::cout << filename << std::endl;
	std::ifstream file(filename);
	if(!file.is_open()){
		throw std::runtime_error("failed to open file");
	}
	std::string line;
	int inputCount = 0;
	int outputCount = 0;
	std::map<std::string, int> inputs;
	std::map<std::string, int> outputs;

	std::string bindingMapCode = std::string("const std::map<std::string, int> ") + shadername + ("BindingMap{");
	std::string bindingDirCode = std::string("const std::vector<bool> ") + shadername + ("BindingDirections{");
	bool commaNeeded = false;
	while(getline(file, line)){
		bool in = false;
		regex del(" ");
		sregex_token_iterator it(line.begin(), line.end(), del, -1);
		sregex_token_iterator end;

		//std::cout << *it << std::endl;
		if (*it == "IN"){
			inputCount++;
			in = true;
		} else {
			outputCount++;
			in = false;
		}
		++it;
		if (in){
			int type = stoi(*it);
			++it;
			std::string inputName = *it;
			if (commaNeeded){
				bindingMapCode += std::string(", ");
				bindingDirCode += std::string(", ");
			}
			bindingMapCode += std::string("{") + inputName + std::string(", ") + std::to_string(type) + std::string("}");
			bindingDirCode += std::string("true");
			commaNeeded = true;
			inputs.insert({*it, type});
		} else {
			int type = stoi(*it);
			//std::cout << type << std::endl;
			++it;
			std::string inputName = *it;
			if (commaNeeded){
				bindingMapCode += std::string(", ");
				bindingDirCode += std::string(", ");
			}
			bindingMapCode += std::string("{") + inputName + std::string(", ") + std::to_string(type) + std::string("}");
			bindingDirCode += std::string("false");
			commaNeeded = true;
			outputs.insert({*it, type});
		}
	}
	file.close();
	bindingMapCode += std::string("};\n");
	bindingDirCode += std::string("};\n");

	std::string helperCode = bindingMapCode + bindingDirCode;
	//std::cout << helperCode << std::endl;
	return helperCode;
}

void loadAndWriteShaders(string basepath, string shadername, bool wireframe, string outRoot){
    string compPath = basepath + shadername + string("Comp.spv");
	string vertPath = basepath + shadername + string("Vert.spv");
	string fragPath = basepath + shadername + string("Frag.spv");
	string helperPath = basepath + shadername + string("Helper.txt");
	vector<char> vertData;
	vector<char> fragData;
	vector<char> compData;
	try{
		vertData = readFile(vertPath);
		fragData = readFile(fragPath);
		std:: cout << "Vert and frag found" << std::endl;
	} catch(std::runtime_error){
		//std::cout << "vert and frag not found" << std::endl;
	} 
	try{
		compData = readFile(compPath);
		std::cout << "Comp found" << std::endl;
	} catch(std::runtime_error){
		//std::cout << "comp not found" << std::endl;
	}

	string capShaderName = shadername;
    std::transform(capShaderName.begin(), capShaderName.end(), capShaderName.begin(), ::toupper);
	ofstream out(outRoot+shadername+string(".h"));
	out << string("#ifndef ")+capShaderName+string("DATA\n");
	out << string("#define ")+capShaderName+string("DATA\n");

	out << string("#include \"ShaderDataType.h\"\n\n");
	if (vertData.size() > 0){
		out << string("const std::vector<unsigned char> ")+shadername+string("vertData = { ");
		for (size_t i = 0; i!= vertData.size(); i++){
			out << "0x" << hex << setw(2) << setfill('0') << (int)(unsigned char) vertData[i];
			if (i < vertData.size()-1){
				out << ", ";
			}
		}
		out << string(" };\n\n");
	}
	if (fragData.size() > 0){
		out << string("const std::vector<unsigned char> ")+shadername+string("fragData = { ");
		for (size_t i = 0; i!= fragData.size(); i++){
			out << "0x" << hex << setw(2) << setfill('0') << (int)(unsigned char) fragData[i];
			if (i < fragData.size()-1){
				out << ", ";
			}
		}
		out << string(" };\n\n");
	} 
	if (compData.size() > 0){
		out << string("const std::vector<unsigned char> ")+shadername+string("compData = { ");
		for (size_t i = 0; i!= compData.size(); i++){
			out << "0x" << hex << setw(2) << setfill('0') << (int)(unsigned char) compData[i];
			if (i < compData.size()-1){
				out << ", ";
			}
		}
		out << string(" };\n\n");
	}
	if (wireframe){
		out << string("const bool ")+shadername+string("Wireframe = true;\n\n");
	} else {
		out << string("const bool ")+shadername+string("Wireframe = false;\n\n");
	}

	bool isHelperAvailable = false;
	try{
		out << readHelperFile(helperPath, shadername) + std::string("\n");
		isHelperAvailable = true;
	} catch(std::runtime_error){
		std::cout << "No helper file found" << std::endl;
	}

	out << string("#endif\n\n");

	out << string("#ifndef ") + capShaderName + string("SHADER\n");
	if (compData.size() == 0){
		out << string("#define ") + capShaderName + string("SHADER shaderData( ")+shadername+string("fragData, ")+shadername+string("vertData, ")+shadername+string("Wireframe )\n");
	} else {
		if (isHelperAvailable){
			out << string("#define ") + capShaderName + string("SHADER shaderData( ")+shadername+string("compData, ")+shadername+string("Wireframe, ") + shadername + string("BindingMap, ") + shadername + string("BindingDirections )\n");
		} else {
			out << string("#define ") + capShaderName + string("SHADER shaderData( ")+shadername+string("compData, ")+shadername+string("Wireframe )\n");
		}
	}
	
	out << string("#endif\n");
	out.close();
}

int main(){
	char rootChar[MAX_PATH];
	GetModuleFileName(NULL, rootChar, MAX_PATH);
	std::filesystem::path rootPth{rootChar};

	string rootPath{rootPth.parent_path().parent_path().string()};
	string basepath = rootPath + string("/computeShaders/");
	string outRoot = rootPath + string("/include/");
	
	loadAndWriteShaders(basepath, string("Kuwahara"), 0, outRoot);
	loadAndWriteShaders(basepath, string("SobelX"), 0, outRoot);
	loadAndWriteShaders(basepath, string("SobelY"), 0, outRoot);
	loadAndWriteShaders(basepath, string("GaussBlurX"), 0, outRoot);
	loadAndWriteShaders(basepath, string("GaussBlurY"), 0, outRoot);
	loadAndWriteShaders(basepath, string("SobelCombined"), 0, outRoot);
	loadAndWriteShaders(basepath, string("ReferenceKuwahara"), 0, outRoot);
	loadAndWriteShaders(basepath, string("Averager"), 0, outRoot);
	loadAndWriteShaders(basepath, string("BoxAverager"), 0, outRoot);
	loadAndWriteShaders(basepath, string("GradRemap"), 0, outRoot);
	loadAndWriteShaders(basepath, string("OSToTSConverter"), 0, outRoot);
	loadAndWriteShaders(basepath, string("TSToOSConverter"), 0, outRoot);
	loadAndWriteShaders(basepath, string("OS_EdgeFill"), 0, outRoot);
	loadAndWriteShaders(basepath, string("inPlaceTest"), 0, outRoot);
	basepath = rootPath+string("/shaders/");
	loadAndWriteShaders(basepath, string("TS_BF"), 0, outRoot);
	loadAndWriteShaders(basepath, string("OS_BF"), 0, outRoot);
	loadAndWriteShaders(basepath, string("BF"), 0, outRoot);
	loadAndWriteShaders(basepath, string("UIGray"), 0, outRoot);
	loadAndWriteShaders(basepath, string("UI"), 0, outRoot);
	cout << "Done" << endl;
	return 0;
}