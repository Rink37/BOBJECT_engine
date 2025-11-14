#ifndef TOMOG
#define TOMOG

#include"ImageProcessor.h"
#include"Textures.h"
#include"LoadLists.h"

class Tomographer {
public:
	// The assumption is that the normal and the template exist elsewhere
	// This class acts to modify a pre-existing surface Normal
	cv::Mat computedNormal;
	cv::Mat computedDiffuse;
	bool alignRequired = true;
	cv::Mat *alignTemplate = nullptr;

	cv::Size outdims;

	void add_image(std::string, std::string);
	void add_lightVector(float phi, float theta);

	void calculate_normal();
	void calculate_diffuse();

	void setLoadList(LoadList* ll) {
		loadList = ll;
	}

	void clearData() {
		images.clear();
		vectors.clear();
		computedNormal.release();
		computedDiffuse.release();
	}

	void cleanup() {
		clearData();
	}

	LoadList* loadList = nullptr;

	std::vector<Texture*> images;
private:
	bool normalExists = false;
	std::vector<Texture*> originalImages;
	
	std::vector<std::vector<float>> vectors;
};

#endif