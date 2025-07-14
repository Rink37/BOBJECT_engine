#ifndef WEBCAM
#define WEBCAM
#include<iostream>
#include<opencv2/opencv.hpp>
#include"StudioSession.h"

class Webcam {
public:
	Webcam();
	
	void getFrame();

	void calibrateCornerFilter();
	void getCorners(bool);
	void updateCorners();

	void updateFrames();

	cv::Mat webcamFrame;

	cv::Point2f cropCorners[4];
	cv::Point2f targetCorners[4];

	void loadFilter();

	void saveFilter();

	bool shouldUpdate = true;
private:
	cv::VideoCapture cap;

	uint8_t filter[6];
	float sizeRatio = 1.41f;

	uint32_t targetWidth;
	uint32_t targetHeight;

	bool isUpdating = true;
};

#endif

