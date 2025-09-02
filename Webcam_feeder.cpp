#include <iostream>
#include <chrono>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include "Webcam_feeder.h"

using namespace std;
using namespace cv;

Webcam::Webcam() {
	cap.open(camIndex, CAP_DSHOW);
	if (!cap.isOpened()) {
		isValid = false;
		return;
	}
	for (int i = 0; i != 4; i++) {
		cropCorners[i] = Point2f(0, 0);
	}
	getFrame();
	targetHeight = webcamFrame.size().height;
	targetWidth = static_cast<uint32_t>(webcamFrame.size().height * sizeRatio);
	targetCorners[0] = Point2f(0, 0);
	targetCorners[1] = Point2f(0, targetHeight);
	targetCorners[2] = Point2f(targetWidth, 0);
	targetCorners[3] = Point2f(targetWidth, targetHeight);
} 

Webcam::Webcam(uint8_t idx) {
	camIndex = idx;
	cap.open(camIndex, CAP_DSHOW);
	if (!cap.isOpened()) {
		isValid = false;
		return;
	}
	for (int i = 0; i != 4; i++) {
		cropCorners[i] = Point2f(0, 0);
	}
	getFrame();
	targetHeight = webcamFrame.size().height;
	targetWidth = static_cast<uint32_t>(webcamFrame.size().height * sizeRatio);
	targetCorners[0] = Point2f(0, 0);
	targetCorners[1] = Point2f(0, targetHeight);
	targetCorners[2] = Point2f(targetWidth, 0);
	targetCorners[3] = Point2f(targetWidth, targetHeight);
}

void Webcam::loadFilter() {
	for (int k = 0; k != 6; k++) {
		filter[k] = session::get()->currentStudio.calibrationSettings[k];
	}
	getCorners(false);
}

void Webcam::saveFilter() {
	for (int k = 0; k != 6; k++) {
		session::get()->currentStudio.calibrationSettings[k]  = filter[k];
	}
}

void Webcam::updateFrames() {
	while (1) {
		getFrame();
	}
}

void Webcam::getFrame() {
	if (isUpdating) {
		if (!cap.isOpened()) {
			isValid = false;
		}
		cap >> webcamFrame;
		updateCorners();
		Mat warp = getPerspectiveTransform(cropCorners, targetCorners);
		warpPerspective(webcamFrame, webcamFrame, warp, Size(targetWidth, targetHeight));
		if (!shouldUpdate) {
			isUpdating = false;
		}
	}
	else if (shouldUpdate) {
		isUpdating = true;
	}
}

static void nothing(int, void*) {
	//Trackbar requires a function with these arguments, but we're not using it for anything
}


void Webcam::calibrateCornerFilter() {
	bool storedShouldUpdate = shouldUpdate;
	shouldUpdate = true;
	string windowName = "Calibrate colour filtering";
	namedWindow(windowName);
	// Initialise GUI trackbars
	createTrackbar("Bmin", windowName, 0, 255, nothing);
	createTrackbar("Bmax", windowName, 0, 255, nothing);
	setTrackbarPos("Bmin", windowName, filter[0]);
	setTrackbarPos("Bmax", windowName, filter[3]);
	createTrackbar("Gmin", windowName, 0, 255, nothing);
	createTrackbar("Gmax", windowName, 0, 255, nothing);
	setTrackbarPos("Gmin", windowName, filter[1]);
	setTrackbarPos("Gmax", windowName, filter[4]);
	createTrackbar("Rmin", windowName, 0, 255, nothing);
	createTrackbar("Rmax", windowName, 0, 255, nothing);
	setTrackbarPos("Rmin", windowName, filter[2]);
	setTrackbarPos("Rmax", windowName, filter[5]);
	int bmin = 0;
	int bmax = 255;
	int gmin = 0;
	int gmax = 255;
	int rmin = 0;
	int rmax = 255;
	// Set color filtering parameters before the loop - we don't want to define them within the loop
	Mat frame;
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
			cv::destroyWindow(windowName);
			break;
		}
		if (getWindowProperty(windowName, WND_PROP_VISIBLE) < 1) {
			break;
		}
	}

	filter[0] = bmin;
	filter[1] = gmin;
	filter[2] = rmin;
	filter[3] = bmax;
	filter[4] = gmax;
	filter[5] = rmax;
	
	shouldUpdate = storedShouldUpdate;

	saveFilter();

	targetCorners[0] = Point2f(0, 0);
	targetCorners[1] = Point2f(0, targetHeight);
	targetCorners[2] = Point2f(targetWidth, 0);
	targetCorners[3] = Point2f(targetWidth, targetHeight);
	getCorners(true);
}

//int displayMaskedWebcam(int arr[6]) {
//	Mat webcamFrame;
//	namedWindow("Calibrated webcam viewer");
//	VideoCapture cap(0);
//	if (!cap.isOpened()) {
//		cout << "No camera detected" << endl;
//		system("pause");
//		return -1;
//	}
//	while (true) {
//		cap >> webcamFrame;
//		if (webcamFrame.empty()) {
//			break;
//		}
//		blur(webcamFrame, webcamFrame, Size(5, 5));
//		inRange(webcamFrame, Scalar(arr[0], arr[1], arr[2]), Scalar(arr[3], arr[4], arr[5]), webcamFrame);
//		imshow("Calibrated webcam viewer", webcamFrame);
//		char c = (char)waitKey(25); //Waits for us to press 'Esc', then exits
//		if (c == 27) {
//			break;
//		}
//	}
//	cap.release();
//	return 0;
//}

float dist(Point2f A, Point2f B) {
	return sqrt(pow(A.x - B.x, 2) + pow(A.y - B.y, 2));
}

void Webcam::getCorners(bool show) {
	Mat frame;
	Mat mask;
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	cap >> frame;
	Point2f corners[4] = {Point2f(0, 0), Point2f(0, frame.size[0]), Point2f(frame.size[1], 0), Point2f(frame.size[1], frame.size[0])};

	if (show) {
		namedWindow("Transformed Image");
	}
	
	float cX = 0;
	float cY = 0;
	int selectIndex = 0;
	if (!cap.isOpened()) {
		cout << "No camera detected" << endl;
		system("pause");
	}
	while (true) {
		cap >> frame;
		if (frame.empty()) {
			break;
		}
		blur(frame, mask, Size(5, 5));
		inRange(mask, Scalar(filter[0], filter[1], filter[2]), Scalar(filter[3], filter[4], filter[5]), mask);
		resize(mask, mask, Size(), 0.25, 0.25);
		findContours(mask, contours, hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE);
		vector<Point2f> coords;
		for (int i = 0; i < contours.size(); i++) {
			float area = contourArea(contours[i]);
			if (area < 500 && area>2) {
				Moments m = moments(contours[i]);
				if (m.m00 != 0) {
					cX = 4 * m.m10 / m.m00;
					cY = 4 * m.m01 / m.m00;
				}
				else {
					cout << area << endl;
					cX = 0;
					cY = 0;
				}
				coords.push_back(Point2f(cX, cY));
			}
		}
		int numCoords = coords.size();
		if (numCoords >= 4) {
			for (int i = 0; i < 4; i++) {
				vector<float> eucDists;
				for (int j = 0; j < numCoords; j++) {
					eucDists.push_back(dist(corners[i], coords[j]));
				}
				auto it = min_element(begin(eucDists), end(eucDists));
				selectIndex = distance(begin(eucDists), it);
				if (eucDists[selectIndex] < 1000) {
					corners[i] = coords[selectIndex];
					coords.erase(coords.begin() + selectIndex);
				}
				numCoords -= 1;
			}
		}
		if (show) {
			for (int i = 0; i < 4; i++) {
				cv::circle(frame, corners[i], 10, Scalar(255, 0, 0), 8, 0);
			}
			imshow("Transformed Image", frame);
			char c = (char)waitKey(25);
			if (c == 27) {
				cv::destroyWindow("Transformed Image");
				break;
			}
			if (getWindowProperty("Transformed Image", WND_PROP_VISIBLE) < 1) {
				break;
			}
		}
		else {
			break;
		}
	}
	cropCorners[0] = corners[0];
	cropCorners[1] = corners[1];
	cropCorners[2] = corners[2];
	cropCorners[3] = corners[3];
}

void Webcam::updateCorners() {
	Mat frame;
	resize(webcamFrame, frame, Size(), 0.25, 0.25);
	Mat cropArea;
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	
	float cX = 0;
	float cY = 0;

	float rectHalf = 20 / 2;

	float smoothness = 25.0f;

	float imgWidth = static_cast<float>(frame.cols);
	float imgHeight = static_cast<float>(frame.rows);

	float cropWidth, cropHeight;

	float l, r, t, b;

	for (int i = 0; i != 4; i++) {
		
		t = clamp((cropCorners[i].y)/4 - rectHalf , 0.0f, imgHeight);
		b = clamp((cropCorners[i].y)/4 + rectHalf, 0.0f, imgHeight);
		l = clamp((cropCorners[i].x)/4 - rectHalf , 0.0f, imgWidth);
		r = clamp((cropCorners[i].x)/4 + rectHalf, 0.0f, imgWidth);

		cropHeight = b - t;
		cropWidth = r - l;

		if (cropWidth > 0 && cropHeight > 1) {
			cropArea = frame(cv::Rect(l, t, cropWidth, cropHeight)).clone();

			blur(cropArea, cropArea, Size(5, 5));
			inRange(cropArea, Scalar(filter[0], filter[1], filter[2]), Scalar(filter[3], filter[4], filter[5]), cropArea);

			findContours(cropArea, contours, hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE);

			if (contours.size() >= 1) {
				Moments m = moments(contours[0]);
				if (m.m00 != 0) {
					cX = m.m10 / m.m00 * 4;
					cY = m.m01 / m.m00 * 4;
					cropCorners[i] = Point2f(((cropCorners[i].x)*(smoothness-1) + l*4 + cX)/smoothness, ((cropCorners[i].y)*(smoothness - 1) + t*4 + cY) / smoothness);
				}
			}
		}
	}
}