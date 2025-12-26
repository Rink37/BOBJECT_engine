#ifndef WEBCAM
#define WEBCAM
#include<iostream>
#include<opencv2/opencv.hpp>
#include"StudioSession.h"

class Webcam {
public:
	Webcam();

	Webcam(uint8_t);

	void cleanup() {
		if (cap.isOpened()) {
			cap.release();
		}
		webcamFrame.release();
	}
	
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
	bool isUpdating = true;

	bool isValid = true;
private:
	cv::Mat warp;

	cv::VideoCapture cap;

	uint8_t filter[6] = { 0, 0, 0, 255, 255, 255 };
	float sizeRatio = 1.41f;

	uint8_t frameInterval = 5;
	uint8_t frameIntCount = 0;

	uint32_t targetWidth = 0;
	uint32_t targetHeight = 0;

	uint8_t camIndex = 0;

};

#endif

