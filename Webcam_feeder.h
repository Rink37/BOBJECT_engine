#ifndef WEBCAM
#define WEBCAM
#include <iostream>
#include<opencv2/opencv.hpp>

class Webcam {
public:
	Webcam();
	
	void getFrame();

	void calibrateCornerFilter();
	void getCorners();
	void updateCorners();

	void updateFrames();

	cv::Mat webcamFrame;

	cv::Point2f cropCorners[4];
	cv::Point2f targetCorners[4];

	bool shouldUpdate = true;
private:
	cv::VideoCapture cap;

	int filter[8] = { 0 };
	float sizeRatio = 1;

	uint32_t targetWidth;
	uint32_t targetHeight;

	bool isUpdating = true;
};

#endif

// TODO: Reference additional headers your program requires here.
