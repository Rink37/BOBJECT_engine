#ifndef STUDIOSESSION
#define STUDIOSESSION

#include <cstdint>
#include<string>
#include<vector>
#include<fstream>
#include<iostream>

struct studioData {
	uint8_t calibrationSettings[6] = { 0, 0, 0, 255, 255, 255};

	std::string defaultModelPath = "None";
	std::string defaultImagePath = "None";

	std::string diffusePath = "None";
	std::string OSPath = "None";
	std::string TSPath = "None";

	std::vector<std::string> modelPaths;
};

class session {
public:
	static session* get() {
		if (nullptr == sessionInstance) sessionInstance = new session;
		return sessionInstance;
	}
	session(const session&) = delete;
	session& operator=(const session&) = delete;
	static void destruct() {
		delete sessionInstance;
		sessionInstance = nullptr;
	}

	std::string studioName;
	studioData currentStudio;

	void saveStudio(std::string);
	void loadStudio(std::string);
	
	void clearStudio() {
		currentStudio.calibrationSettings[0] = 0;
		currentStudio.calibrationSettings[1] = 0;
		currentStudio.calibrationSettings[2] = 0;
		currentStudio.calibrationSettings[3] = 255;
		currentStudio.calibrationSettings[4] = 255;
		currentStudio.calibrationSettings[5] = 255;

		currentStudio.defaultModelPath = "None";
		currentStudio.defaultImagePath = "None";

		currentStudio.diffusePath = "None";
		currentStudio.OSPath = "None";
		currentStudio.TSPath = "None";

		currentStudio.modelPaths.clear();
	}
private:
	static session* sessionInstance;
	session() = default;
	~session() = default;
};

#endif
