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
private:
	static session* sessionInstance;
	session() = default;
	~session() = default;
};

#endif