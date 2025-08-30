#ifndef TOMOG
#define TOMOG

#include"ImageProcessor.h"

class Tomographer {
public:
	// The assumption is that the normal and the template exist elsewhere
	// This class acts to modify a pre-existing surface Normal
	cv:: Mat computedNormal;
	bool alignRequired = true;
	cv::Mat *alignTemplate = nullptr;

	cv::Size outdims;

	void add_image(std::string, float phi, float theta);

	void calculate_normal();

	void clearData() {
		images.clear();
		vectors.clear();
	}
private:
	std::vector<cv::Mat> images;
	std::vector<std::vector<float>> vectors;
};

#endif