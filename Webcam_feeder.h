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

	void updateAspectRatio(float);

	void setRotation(bool);
	void setRotation(uint8_t);

	void fetchFromCamera();
	
	void getFrame();
	void asyncFrameFetch();

	void calibrateCornerFilter();
	void getCorners(bool);
	void updateCorners();

	//void updateFrames();

	cv::Mat webcamFrame;

	cv::Point2f cropCorners[4];
	cv::Point2f targetCorners[4];

	void loadFilter();

	void saveFilter();

	void findWebcams();

	void switchWebcam(bool);
	void switchWebcam(int);

	bool shouldUpdate = true;
	bool isUpdating = true;

	bool isValid = true;

	bool shouldCrop = false;

	float sizeRatio = 1.41f;

	uint8_t rotationState = 0;
	uint8_t camIndex = 0;
private:

	std::vector<uint8_t> webcamIds{};

	cv::Mat warp;

	cv::VideoCapture cap;

	uint8_t filter[6] = { 0, 0, 0, 255, 255, 255 };

	uint32_t baseWidth, baseHeight = 0;

	uint32_t targetWidth = 0;
	uint32_t targetHeight = 0;
	uint32_t targetDim = 0;
	cv::Point topCorner = cv::Point(0, 0);
	
	cv::Mat RotationMatrix;
};

#endif

