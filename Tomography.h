#ifndef TOMOG
#define TOMOG

#include"ImageProcessor.h"

class Tomographer {
public:
	// The assumption is that the normal and the template exist elsewhere
	// This class acts to modify a pre-existing surface Normal
	Texture *surfaceNormal = nullptr;
	bool alignRequired = false;
	Mat *alignTemplate = nullptr;

	void add_image(std::string, float phi, float theta);
private:
	std::vector<cv::Mat> images;
	std::vector<std::vector<float>> vectors;
	void calculate_normal();
};

#endif