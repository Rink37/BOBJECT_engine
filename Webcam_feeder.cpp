// Webcam_feeder.cpp : Defines the entry point for the application.
//

#include <iostream>
#include<chrono>
#include <opencv2/opencv.hpp>
#include "Webcam_feeder.h"

using namespace std;
using namespace cv;

void displayWebcam() {
	Mat webcamFrame;
	namedWindow("Webcam viewer");
	VideoCapture cap(0);
	if (!cap.isOpened()) {
		cout << "No camera detected" << endl;
		system("pause");
	}
	while (true) {
		cap >> webcamFrame;
		if (webcamFrame.empty()) {
			break;
		}
		imshow("Webcam viewer", webcamFrame);
		char c = (char)waitKey(25); //Waits for us to press 'Esc'
		if (c == 27) {
			break;
		}
	}
	cap.release();
}

Webcam::Webcam() {
	cap.open(0, CAP_DSHOW);
	capfps = 0;
	starttime = chrono::high_resolution_clock::now();
} 

void Webcam::getFramerate() {
	int Numreps = 50;
	int i = 0;
	auto start = chrono::high_resolution_clock::now();
	while (i < Numreps) {
		cap >> webcamFrame;
		i++;
	}
	auto end = chrono::high_resolution_clock::now();

	capfps = static_cast<float>(Numreps) / static_cast<float>(chrono::duration_cast<chrono::seconds>(end - start).count());
	cout << "Webcam fps = " << capfps << endl;
}

bool Webcam::shouldUpdate() {
	bool update = false;
	if (capfps == 0) {
		getFramerate();
	}
	auto currentTime = chrono::high_resolution_clock::now();
	int timeInMs = chrono::duration_cast<chrono::milliseconds>(currentTime - starttime).count();
	if (timeInMs <= 1000 / capfps || timeInMs > 1000 / (capfps - 1)) {
		update = true;
		starttime = chrono::high_resolution_clock::now();
	}
	return update;
}

void Webcam::getFrame() {
	if (!cap.isOpened()) {
		cout << "Camera error" << endl;
	}
 	cap >> webcamFrame;
}

int displayMaskedWebcam(int arr[6]) {
	Mat webcamFrame;
	namedWindow("Calibrated webcam viewer");
	VideoCapture cap(0);
	if (!cap.isOpened()) {
		cout << "No camera detected" << endl;
		system("pause");
		return -1;
	}
	while (true) {
		cap >> webcamFrame;
		if (webcamFrame.empty()) {
			break;
		}
		blur(webcamFrame, webcamFrame, Size(5, 5));
		inRange(webcamFrame, Scalar(arr[0], arr[1], arr[2]), Scalar(arr[3], arr[4], arr[5]), webcamFrame);
		imshow("Calibrated webcam viewer", webcamFrame);
		char c = (char)waitKey(25); //Waits for us to press 'Esc', then exits
		if (c == 27) {
			break;
		}
	}
	cap.release();
	return 0;
}

static void nothing(int, void*) {
	//Trackbar requires a function with these arguments, but we're not using it for anything
}

int* calibrateMask(int out[6]) {
	string windowName = "Colour calibration window";
	namedWindow(windowName);
	// Initialise GUI trackbars
	createTrackbar("Bmin", windowName, 0, 255, nothing);
	createTrackbar("Bmax", windowName, 0, 255, nothing);
	setTrackbarPos("Bmax", windowName, 255);
	createTrackbar("Gmin", windowName, 0, 255, nothing);
	createTrackbar("Gmax", windowName, 0, 255, nothing);
	setTrackbarPos("Gmax", windowName, 255);
	createTrackbar("Rmin", windowName, 0, 255, nothing);
	createTrackbar("Rmax", windowName, 0, 255, nothing);
	setTrackbarPos("Rmax", windowName, 255);
	int bmin = 0;
	int bmax = 255;
	int gmin = 0;
	int gmax = 255;
	int rmin = 0;
	int rmax = 255;
	// Set color filtering parameters before the loop - we don't want to define them within the loop
	Mat frame;
	VideoCapture cap(0);
	if (!cap.isOpened()) {
		cout << "No camera detected" << endl;
		system("pause");
		return out;
	}
	while (true) {
		// Get values of trackbars and use them to define te value of colour filtering parameters
		bmin = getTrackbarPos("Bmin", windowName);
		bmax = getTrackbarPos("Bmax", windowName);
		gmin = getTrackbarPos("Gmin", windowName);
		gmax = getTrackbarPos("Gmax", windowName);
		rmin = getTrackbarPos("Rmin", windowName);
		rmax = getTrackbarPos("Rmax", windowName);
		// Retrieve the frame from cap
		cap >> frame;
		if (frame.empty()) {
			break;
		}
		blur(frame, frame, Size(5, 5)); // I've found that including a bit of blur gives much more solid corner detection and removes some unwanted artifacts
		inRange(frame, Scalar(bmin, gmin, rmin), Scalar(bmax, gmax, rmax), frame); // Create a filter mask by colour
		imshow(windowName, frame);//Show the frame
		char c = (char)waitKey(25); //Waits for us to press 'Esc', then exits
		if (c == 27) {
			break;
		}
	}
	cap.release();

	destroyWindow(windowName);

	out[0] = bmin;
	out[1] = gmin;
	out[2] = rmin;
	out[3] = bmax;
	out[4] = gmax;
	out[5] = rmax;
	return out; // return an updated mask parameters array which can be used in following functions. 
}

//int main()
//{
//	int colourMask[6] = { 0, 0, 0, 255, 255, 255 }; // Generate the widest range colour calibration settings

	// We don't currently use the target directory or file name but this is what we will update when we are testing actual tracking
//	string targetdir = "C:\\Users\\robda\\Documents\\Projects\\Blender_oilpainter\\Blender files\\temp\\";
//	string filename = "webcam_frame.jpeg";
	// Mat image = imread(targetdir + filename);

//	calibrateMask(colourMask); // GUI which lets us specify ideal colour ranges

//	return displayMaskedWebcam(colourMask); // Currently as far as the program goes - we just display the masked webcam view 
//}
