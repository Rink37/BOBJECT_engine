#ifndef WEBCAM
#define WEBCAM

#pragma once

#include <iostream>
#include<opencv2/opencv.hpp>

class Webcam {
public:
	Webcam();
	
	void getFrame();
	void getFramerate();
	bool shouldUpdate();

	cv::Mat webcamFrame;

	double capfps;
	std::chrono::steady_clock::time_point starttime;
private:
	cv::VideoCapture cap;
};

#endif

// TODO: Reference additional headers your program requires here.
