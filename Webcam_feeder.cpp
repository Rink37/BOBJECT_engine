#include <iostream>
#include <chrono>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include "Webcam_feeder.h"

using namespace std;
using namespace cv;

Webcam::Webcam() {
	findWebcams();
	if (webcamIds.size() == 0) {
		isValid = false;
		return;
	}
	cap.open(webcamIds[camIndex], CAP_DSHOW);
	if (!cap.isOpened()) {
		isValid = false;
		return;
	}
	for (int i = 0; i != 4; i++) {
		cropCorners[i] = Point2f(0, 0);
	}
	cap >> webcamFrame;
	targetHeight = webcamFrame.size().height;
	targetWidth = static_cast<uint32_t>(webcamFrame.size().height * sizeRatio);

	baseWidth = webcamFrame.size().width;
	baseHeight = webcamFrame.size().height;

	targetCorners[0] = Point2f(0, 0);
	targetCorners[1] = Point2f(0, targetHeight);
	targetCorners[2] = Point2f(targetWidth, 0);
	targetCorners[3] = Point2f(targetWidth, targetHeight);
} 

Webcam::Webcam(uint8_t idx) {
	findWebcams();
	if (webcamIds.size() == 0) {
		isValid = false;
		return;
	}
	camIndex = idx;
	cap.open(webcamIds[camIndex], CAP_DSHOW);
	if (!cap.isOpened()) {
		isValid = false;
		return;
	}
	for (int i = 0; i != 4; i++) {
		cropCorners[i] = Point2f(0, 0);
	}
	cap >> webcamFrame;
	targetHeight = webcamFrame.size().height;
	targetWidth = static_cast<uint32_t>(webcamFrame.size().height * sizeRatio);

	baseWidth = webcamFrame.size().width;
	baseHeight = webcamFrame.size().height;
	targetDim = (baseWidth > baseHeight) ? baseWidth : baseHeight;
	
	targetCorners[0] = Point2f(0, 0);
	targetCorners[1] = Point2f(0, targetHeight);
	targetCorners[2] = Point2f(targetWidth, 0);
	targetCorners[3] = Point2f(targetWidth, targetHeight);
}

void Webcam::switchWebcam(bool direction) {
	if (webcamIds.size() == 0) {
		return;
	}
	if (direction) {
		camIndex++;
		camIndex %= webcamIds.size();
	}
	else {
		if (camIndex > 0) {
			camIndex--;
		}
		else {
			camIndex = webcamIds.size()-1;
		}
	}
	switchWebcam(camIndex);
}

void Webcam::switchWebcam(int index) {
	if (webcamIds.size() == 0) {
		return;
	}
	index %= webcamIds.size();
	cap.release();
	webcamFrame.release();

	camIndex = index;
	
	cap.open(webcamIds[index]);
	if (!cap.isOpened()) {
		isValid = false;
		return;
	}
	
	std::cout << "Switched to camera " << index << std::endl;
	
	for (int i = 0; i != 4; i++) {
		cropCorners[i] = Point2f(0, 0);
	}
	RotationMatrix.release();
	if (rotationState == 1 || rotationState == 3) {
		sizeRatio = 1.0f / sizeRatio;
	}
	rotationState = 0;
	topCorner = Point(0, 0);
	
	cap >> webcamFrame;
	targetHeight = webcamFrame.size().height;
	targetWidth = static_cast<uint32_t>(webcamFrame.size().height * sizeRatio);
	
	baseWidth = webcamFrame.size().width;
	baseHeight = webcamFrame.size().height;
	targetDim = (baseWidth > baseHeight) ? baseWidth : baseHeight;

	targetCorners[0] = Point2f(0, 0);
	targetCorners[1] = Point2f(0, targetHeight);
	targetCorners[2] = Point2f(targetWidth, 0);
	targetCorners[3] = Point2f(targetWidth, targetHeight);
	shouldCrop = false;
}

void Webcam::fetchFromCamera() {
	if (RotationMatrix.empty()) {
		cap >> webcamFrame;
	}
	else {
		Mat frame;
		cap >> frame;
		warpAffine(frame, frame, RotationMatrix, Size(targetDim, targetDim));
		Mat ROI(frame, Rect(topCorner.x, topCorner.y, baseWidth, baseHeight));
		webcamFrame.release();
		ROI.copyTo(webcamFrame);
		frame.release();
	}
}

void Webcam::setRotation(uint8_t state) {
	rotationState = state;

	std::cout << "Rotating to state " << static_cast<int>(rotationState) << std::endl;

	float angle = 0.0f;
	targetDim = (baseWidth > baseHeight) ? baseWidth : baseHeight;
	uint32_t smallDimension = (baseWidth < baseHeight) ? baseWidth : baseHeight;
	switch (rotationState) {
	case 0:
		angle = 0.0f;
		topCorner = Point(0, 0);
		break;
	case 1:
		angle = 90.0f;
		topCorner = Point(0, 0);
		break;
	case 2:
		angle = 180.0f;
		topCorner = Point(0, targetDim - smallDimension);
		break;
	case 3:
		angle = 270.0f;
		topCorner = Point(targetDim - smallDimension, 0);
		break;
	default:
		angle = 0.0f;
		topCorner = Point(0, 0);
		break;
	}

	RotationMatrix = getRotationMatrix2D(Point2f(targetDim / 2, targetDim / 2), angle, 1.0f);
	webcamFrame.release();
	for (int i = 0; i != 4; i++) {
		cropCorners[i] = Point2f(0, 0);
	}

	if (angle == 0.0f) {
		RotationMatrix.release();
		cap >> webcamFrame;
	}
	else {
		Mat frame;
		cap >> frame;
		warpAffine(frame, frame, RotationMatrix, Size(targetDim, targetDim));
		Mat ROI(frame, Rect(topCorner.x, topCorner.y, baseHeight, baseWidth));
		ROI.copyTo(webcamFrame);
		frame.release();
	}
	
	sizeRatio = 1.0f / sizeRatio;

	targetHeight = webcamFrame.size().height;
	targetWidth = static_cast<uint32_t>(webcamFrame.size().height * sizeRatio);

	baseWidth = webcamFrame.size().width;
	baseHeight = webcamFrame.size().height;

	targetCorners[0] = Point2f(0, 0);
	targetCorners[1] = Point2f(0, targetHeight);
	targetCorners[2] = Point2f(targetWidth, 0);
	targetCorners[3] = Point2f(targetWidth, targetHeight);
	shouldCrop = false;
}

void Webcam::setRotation(bool direction) {
	if (!direction) {
		rotationState++;
		rotationState %= 4;
	}
	else {
		if (rotationState > 0) {
			rotationState--;
		}
		else {
			rotationState = 3;
		}
	}
	setRotation(rotationState);
}

void Webcam::findWebcams() {
	for (uint8_t i = 0; i != 5; i++) {
		VideoCapture testWebcam;
		testWebcam.open(i);
		if (testWebcam.isOpened()) {
			webcamIds.push_back(i);
		}
		testWebcam.release();
	}
	std::cout << "Number of webcams found = " << webcamIds.size() << std::endl;
	std::cout << "Webcam IDs = ";
	for (size_t i = 0; i != webcamIds.size(); i++) {
		std::cout << static_cast<int>(webcamIds[i]) << " ";
	}
	std::cout << std::endl;
}

void Webcam::loadFilter() {
	uint8_t defaultCalibSettings[6] = { 0, 0, 0, 255, 255, 255 };
	bool isNotDefault = true; // If the loaded settings are default then we don't want to perform corner cropping
	for (int k = 0; k != 6; k++) {
		filter[k] = session::get()->currentStudio.calibrationSettings[k];
		if (isNotDefault) {
			isNotDefault = (filter[k] == defaultCalibSettings[k]);
		}
	}
	if (!isNotDefault) {
		getCorners(false);
	}
}

void Webcam::saveFilter() {
	for (int k = 0; k != 6; k++) {
		session::get()->currentStudio.calibrationSettings[k] = filter[k];
	}
}

void Webcam::updateAspectRatio(float ratio) {
	sizeRatio = ratio;
	targetHeight = webcamFrame.size().height;
	targetWidth = static_cast<uint32_t>(webcamFrame.size().height * sizeRatio);
	
	targetCorners[0] = Point2f(0, 0);
	targetCorners[1] = Point2f(0, targetHeight);
	targetCorners[2] = Point2f(targetWidth, 0);
	targetCorners[3] = Point2f(targetWidth, targetHeight);
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
		fetchFromCamera();
		if (shouldCrop) {
			updateCorners();
			warp = getPerspectiveTransform(cropCorners, targetCorners);
			warpPerspective(webcamFrame, webcamFrame, warp, Size(targetWidth, targetHeight));
		}
		else {
			resize(webcamFrame, webcamFrame, Size(targetWidth, targetHeight));
		}
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

static float dist(Point2f A, Point2f B) {
	return sqrt(pow(A.x - B.x, 2) + pow(A.y - B.y, 2));
}

void Webcam::getCorners(bool show) {
	//Mat frame;
	Mat mask;
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	fetchFromCamera();
	Point2f corners[4] = {Point2f(0, 0), Point2f(0, webcamFrame.size[0]), Point2f(webcamFrame.size[1], 0), Point2f(webcamFrame.size[1], webcamFrame.size[0])};

	string windowName = "Live webcam view";

	if (show) {
		namedWindow(windowName);
	}
	
	float cX = 0;
	float cY = 0;
	int selectIndex = 0;
	if (!cap.isOpened()) {
		cout << "No camera detected" << endl;
		system("pause");
	}

	bool masked = false;
	bool circle = false;
	bool cropped = false;

	while (true) {
		//cap >> frame;
		fetchFromCamera();
		if (webcamFrame.empty()) {
			break;
		}
		blur(webcamFrame, mask, Size(5, 5));
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
		break;
	}
	
	if (show) {
		while (true) {
			updateCorners();
			fetchFromCamera();
			//cap >> frame;
			if (cropped) {
				warp = getPerspectiveTransform(cropCorners, targetCorners);
				warpPerspective(webcamFrame, webcamFrame, warp, Size(targetWidth, targetHeight));
			}
			if (masked) {
				blur(webcamFrame, webcamFrame, Size(5, 5));
				inRange(webcamFrame, Scalar(filter[0], filter[1], filter[2]), Scalar(filter[3], filter[4], filter[5]), webcamFrame);
				cvtColor(webcamFrame, webcamFrame, COLOR_GRAY2RGB);
			}
			if (circle) {
				for (int i = 0; i < 4; i++) {
					cv::circle(webcamFrame, corners[i], 10, Scalar(255, 0, 0), 8, 0);
				}
			}
			imshow(windowName, webcamFrame);
			char c = (char)waitKey(5);
			if (c == 27) { // ASCII code for Esc
				cv::destroyWindow(windowName);
				break;
			}
			if (c == 109) { // ASCII code for m
				masked = !masked;
			}
			if (c == 99) { // ASCII code for c
				circle = !circle;
			}
			if (c == 97) { // ASCII code for a
				cropped = !cropped;
			}
			if (getWindowProperty(windowName, WND_PROP_VISIBLE) < 1) {
				break;
			}
		}
	}
	cropCorners[0] = corners[0];
	cropCorners[1] = corners[1];
	cropCorners[2] = corners[2];
	cropCorners[3] = corners[3];

	shouldCrop = true;
}

void Webcam::updateCorners() {
	Mat frame;
	resize(webcamFrame, frame, Size(), 0.25f, 0.25f);
	Mat cropArea;
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	
	float cX = 0;
	float cY = 0;

	float rectHalf = 10.0f;

	float smoothness = 0.04f; // Equivalent to 1/25

	float imgWidth = static_cast<float>(frame.cols);
	float imgHeight = static_cast<float>(frame.rows);

	float cropWidth, cropHeight;

	float l, r, t, b;

	for (int i = 0; i != 4; i++) {
		
		t = clamp((cropCorners[i].y) * 0.25f - rectHalf , 0.0f, imgHeight);
		b = clamp((cropCorners[i].y) * 0.25f + rectHalf, 0.0f, imgHeight);
		l = clamp((cropCorners[i].x) * 0.25f - rectHalf , 0.0f, imgWidth);
		r = clamp((cropCorners[i].x) * 0.25f + rectHalf, 0.0f, imgWidth);

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
					// Here 24 is substituted for (1/smoothness - 1) since hard-coding is much more efficient than division
					cropCorners[i] = Point2f(((cropCorners[i].x)*(24.0f) + l*4.0f + cX) * smoothness, ((cropCorners[i].y)*(24.0f) + t*4.0f + cY) * smoothness);
				}
			}
		}
	}
}