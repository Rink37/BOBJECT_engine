#ifndef STUDIOSESSION
#define STUDIOSESSION

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

	uint8_t webcamSettings = 0;
	float webcamAspectRatio = 0.0f;

	void packWebcamSettings(uint8_t rotationState, uint8_t webcamIndex){
		// rotation State has a max. value of 3, so only needs two bits
		// This leaves 6 bits in the packed settings for the webcam index, meaning we can have a max. webcam index of 64
		webcamSettings = rotationState & 3;
		uint8_t shiftedWebIdx = (webcamIndex & 63) << 2;
		webcamSettings = webcamSettings | shiftedWebIdx;
	}

	void unpackWebcamSettings(uint8_t& rotationState, uint8_t& webcamIndex) {
		rotationState = webcamSettings & 3;
		webcamIndex = webcamSettings >> 2;
	}
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

		currentStudio.webcamSettings = 0;
		currentStudio.webcamAspectRatio = 0.0f;
	}
private:
	static session* sessionInstance;
	session() = default;
	~session() = default;
};

#endif