#ifndef WEBCAM
#define WEBCAM
#include<iostream>
#include<opencv2/opencv.hpp>
#include"StudioSession.h"

class Webcam {
public:
	Webcam();

	Webcam(uint8_t);

	~Webcam() {
		cap.release();
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

	bool isValid = true;
private:
	cv::VideoCapture cap;

	uint8_t filter[6] = { 0, 0, 0, 255, 255, 255 };
	float sizeRatio = 1.41f;

	uint32_t targetWidth = 0;
	uint32_t targetHeight = 0;

	uint8_t camIndex = 0;

	bool isUpdating = true;

};

#endif

