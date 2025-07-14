#include"StudioSession.h"

using namespace std;

void session::saveStudio(string name) {
	ofstream out(name.c_str(), ios::binary);
	
	for (int k = 0; k != 6; k++) {
		out.write((char*)&currentStudio.calibrationSettings[k], sizeof(uint8_t));
	}
	
	uint32_t modelPathSize = currentStudio.defaultModelPath.size();
	out.write((char*)&modelPathSize, sizeof(uint32_t));
	out.write(&currentStudio.defaultModelPath[0], modelPathSize);
	
	uint32_t imagePathSize = currentStudio.defaultImagePath.size();
	out.write((char*)&imagePathSize, sizeof(uint32_t));
	out.write(&currentStudio.defaultImagePath[0], imagePathSize);

	uint32_t diffPathSize = 0;
	uint32_t OSPathSize = 0;
	uint32_t TSPathSize = 0;

	if (currentStudio.diffusePath == "None") {
		diffPathSize = 0;
		out.write((char*)&diffPathSize, sizeof(uint32_t));
	}
	else {
		diffPathSize = currentStudio.diffusePath.size();
		out.write((char*)&diffPathSize, sizeof(uint32_t));
		out.write(&currentStudio.diffusePath[0], diffPathSize);
	}

	if (currentStudio.OSPath == "None") {
		OSPathSize = 0;
		out.write((char*)&OSPathSize, sizeof(uint32_t));
	}
	else {
		OSPathSize = currentStudio.OSPath.size();
		out.write((char*)&OSPathSize, sizeof(uint32_t));
		out.write(&currentStudio.OSPath[0], OSPathSize);
	}

	if (currentStudio.TSPath == "None") {
		TSPathSize = 0;
		out.write((char*)&TSPathSize, sizeof(uint32_t));
	}
	else {
		TSPathSize = currentStudio.TSPath.size();
		out.write((char*)&TSPathSize, sizeof(uint32_t));
		out.write(&currentStudio.TSPath[0], TSPathSize);
	}

	uint32_t modelNumber = currentStudio.modelPaths.size();
	out.write((char*)&modelNumber, sizeof(uint32_t));
	for (uint32_t i = 0; i != modelNumber; i++) {
		uint32_t modelPathSize = currentStudio.modelPaths[i].size();
		out.write((char*)&modelPathSize, sizeof(uint32_t));
		out.write(&currentStudio.modelPaths[i][0], modelPathSize);
	}
	out.close();
	cout << "File written" << endl;
}

void session::loadStudio(string name) {
	try {
		ifstream in(name.c_str(), ios::binary);
		//if (!in.good()) {
		//	cout << "File not found" << endl;
		//	return;
		//}
		cout << "File found" << endl;
		
		for (int k = 0; k != 6; k++) {
			in.read((char*)&currentStudio.calibrationSettings[k], sizeof(uint8_t));
		}

		uint32_t modelPathSize = 0;
		in.read((char*)&modelPathSize, sizeof(uint32_t));
		currentStudio.defaultModelPath.clear();
		currentStudio.defaultModelPath.resize(modelPathSize);
		in.read(&currentStudio.defaultModelPath[0], modelPathSize);

		uint32_t imagePathSize = 0;
		in.read((char*)&imagePathSize, sizeof(uint32_t));
		currentStudio.defaultImagePath.clear();
		currentStudio.defaultImagePath.resize(imagePathSize);
		in.read(&currentStudio.defaultImagePath[0], imagePathSize);

		uint32_t diffusePathSize = 0;
		in.read((char*)&diffusePathSize, sizeof(uint32_t));
		if (diffusePathSize == 0) {
			currentStudio.diffusePath = "None";
		}
		else {
			currentStudio.diffusePath.clear();
			currentStudio.diffusePath.resize(diffusePathSize);
			in.read(&currentStudio.diffusePath[0], diffusePathSize);
		}

		uint32_t OSPathSize = 0;
		in.read((char*)&OSPathSize, sizeof(uint32_t));
		if (OSPathSize == 0) {
			currentStudio.OSPath = "None";
		}
		else {
			currentStudio.OSPath.clear();
			currentStudio.OSPath.resize(OSPathSize);
			in.read(&currentStudio.OSPath[0], OSPathSize);
		}

		uint32_t TSPathSize = 0;
		in.read((char*)&TSPathSize, sizeof(uint32_t));
		if (TSPathSize == 0) {
			currentStudio.TSPath = "None";
		}
		else {
			currentStudio.TSPath.clear();
			currentStudio.TSPath.resize(TSPathSize);
			in.read(&currentStudio.TSPath[0], TSPathSize);
		}

		uint32_t modelNumber = 0;
		in.read((char*)&modelNumber, sizeof(uint32_t));
		for (uint32_t i = 0; i != modelNumber; i++) {
			uint32_t modelPathSize = 0;
			string modelPath;
			in.read((char*)&modelPathSize, sizeof(uint32_t));
			modelPath.resize(modelPathSize);
			in.read(&modelPath[0], modelPathSize);
			currentStudio.modelPaths.push_back(modelPath);
			cout << modelPath << endl;
		}
		in.close();
	}
	catch (...) {
		cout << "Savefile invalid" << endl;
	}	
}