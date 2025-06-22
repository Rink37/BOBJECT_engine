#include"GenerateNormalMap.h"

using namespace std;
using namespace cv;

float phi(float x)
{
	double a1 = 0.254829592;
	double a2 = -0.284496736;
	double a3 = 1.421413741;
	double a4 = -1.453152027;
	double a5 = 1.061405429;
	double p = 0.3275911;

	int sign = 1;
	if (x < 0)
		sign = -1;
	x = fabs(x) / sqrt(2.0);

	double t = 1.0 / (1.0 + p * x);
	double y = 1.0 - (((((a5 * t + a4) * t) + a3) * t + a2) * t + a1) * t * exp(-x * x);

	return 0.5 * (1.0 + sign * y);
}

Vec3b avgColourFromMask(Mat srcImg, Mat mask) {
	Vec3f sumColour;
	int sum = 0;
	for (int x = 0; x != srcImg.rows; x++) {
		for (int y = 0; y != srcImg.cols; y++) {
			if (mask.at<uchar>(x, y) != 0 && srcImg.at<Vec3b>(x, y) != Vec3b(0, 0, 0)) {
				sumColour += static_cast<Vec3f>(srcImg.at<Vec3b>(x, y))/255.0f;
				sum++;
			}
		}
	}
	if (sum != 0) {
		return static_cast<Vec3b>(255 * sumColour / sum);
	}
	else {
		return Vec3b(0, 0, 0);
	}
}

Vec3f stdFromMask(Mat srcImg, Mat mask, Vec3b avgColour) {
	Vec3f currentColour;
	float c0 = 0;
	float c1 = 0;
	float c2 = 0;
	int sum = 0;
	for (int x = 0; x != srcImg.rows; x++) {
		for (int y = 0; y != srcImg.cols; y++) {
			if (mask.at<uchar>(x, y) != 0) {
				currentColour = static_cast<Vec3f>(srcImg.at<Vec3b>(x, y));
				c0 += powf(currentColour[0] - static_cast<float>(avgColour[0]), 2.0f);
				c1 += powf(currentColour[1] - static_cast<float>(avgColour[1]), 2.0f);
				c2 += powf(currentColour[1] - static_cast<float>(avgColour[2]), 2.0f);
				sum++;
			}
		}
	}
	c0 /= sum;
	c1 /= sum;
	c2 /= sum;
	return Vec3f(sqrt(c0), sqrt(c1), sqrt(c2));
}

float colourPval(Vec3b colour, Vec3b avgColour, Vec3f stdev) {
	Vec3f colourZ;
	float colourP = 0;
	colourZ = (static_cast<Vec3f>(colour) - static_cast<Vec3f>(avgColour));
	colourZ[0] /= stdev[0];
	colourZ[1] /= stdev[1];
	colourZ[2] /= stdev[2];
	colourP += 1 - phi(abs(colourZ[0]));
	colourP += 1 - phi(abs(colourZ[1]));
	colourP += 1 - phi(abs(colourZ[2]));
	colourP /= 1.5f;
	return colourP;
}

float pointDist(Point2f pointA, Point2f pointB){
	return norm( pointA - pointB );
}

float colourDist(Vec3b colourA, Vec3b colourB) {
	return norm( colourA - colourB );
}

Mat maskByValue(Mat input, int val) {
	Mat output = input.clone();
	for (int x = 0; x != input.rows; x++) {
		for (int y = 0; y != input.cols; y++) {
			if (static_cast<int>(input.at<int>(x, y)) == val) {
				output.at<int>(x, y) = 255;
			}
			else {
				output.at<int>(x, y) = 0;
			}
		}
	}
	return output;
}

void NormalGen::contextualConvertMap(Mat srcImg) {

	// Function attempts to map the object-space normal map so that each different colour in the input image appears as a flat plane on the result

	// Currently experiences an issue where colour filtering fails close to edges because the closest detected colour belongs to a different island
	// We need some method of segmenting the islands

	Mat srcImgSegment = srcImg.clone();
	Mat segmentMask;
	Mat segmentMask2;
	Mat srcMap;
	Mat outMap;
	Mat islands;
	Mat islandMarkers;
	int islandMarker = 0;
	vector<int> islandUniqueMarkers;

	resize(OSNormalMap, srcMap, Size(srcImg.cols, srcImg.rows), INTER_LINEAR);
	outMap = srcMap.clone();
	Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));

	cvtColor(srcMap, islands, COLOR_BGR2GRAY);
	threshold(islands, islands, 1, 255, THRESH_BINARY);

	int val;
	
	connectedComponents(islands, islandMarkers);
	
	for (int x = 0; x != islandMarkers.rows; x++) {
		for (int y = 0; y != islandMarkers.cols; y++) {
			val = islandMarkers.at<int>(x, y)+1;
			islandMarkers.at<int>(x, y) = val;
			if (val > 1 && (find(islandUniqueMarkers.begin(), islandUniqueMarkers.end(), val) == islandUniqueMarkers.end())) {
				islandUniqueMarkers.push_back(val);
			}
		}
	}
	
	islandMarkers.convertTo(islandMarkers, CV_32F);

	Mat mainOpening;
	Mat twoDimage;

	int K = 25;
	int attempts = 10;

	Mat label;
	Mat center;
	TermCriteria criteria(3, 10, 1.0);

	Mat maskedImage = srcImg.clone();
	Mat grayMask;
	Mat threshed;
	Mat opening;
	Mat sure_bg;
	Mat sure_fg;
	Mat dist_transform;
	Mat unknown;
	Mat markers;
	Mat target;
	Mat remaining;
	Mat contours;

	double minVal;
	double maxVal;
	Point minLoc;
	Point maxLoc;

	vector<Mat> totMarkers;
	vector<Point2f> centroids;
	vector<Vec3b> mapColours;
	vector<Vec3b> avgColours;
	vector<Vec3f> colourStdev;
	vector<vector<int>> totUniqueMarkers;

	vector<int> unique_markers;
	int um;
	int tot;

	vector<vector<int>> indexes;
	vector<int> tindexes;

	Rect bBox;
	Point2f centroid;

	Vec3b thisMapColour;
	Vec3b thisAvgColour;
	Vec3f thisColourStd;

	Vec3b closestMapColour;
	Vec3b closestAvgColour;

	vector<Point2f> tempCentroids;
	vector<Vec3b> tempMapColours;
	vector<Vec3b> tempAvgColours;

	Vec3f calculatedColour;

	float minDist;
	float currentDist;
	Point2f coord;

	float d;

	int closeIndex = 0;

	double matMax = 0;
	double matMin = 0;
	Point maxloc;
	Point minloc;
	float minProb;

	morphologyEx(srcImgSegment, mainOpening, MORPH_OPEN, kernel, Point(-1, -1), 2);

	for (int m = 0; m != islandUniqueMarkers.size(); m++) {
		islandMarker = islandUniqueMarkers[m];
		threshold(islandMarkers, segmentMask, islandMarker - 1, 255, THRESH_BINARY);
		threshold(islandMarkers, segmentMask2, islandMarker, 255, THRESH_BINARY);
		subtract(segmentMask, segmentMask2, segmentMask);
		segmentMask.convertTo(segmentMask, CV_8U);

		totMarkers.clear();
		centroids.clear();
		mapColours.clear();
		avgColours.clear();
		colourStdev.clear();
		totUniqueMarkers.clear();
		unique_markers.clear();
		indexes.clear();
		tot = 0;

		for (int x = 0; x != srcImgSegment.rows; x++) {
			for (int y = 0; y != srcImgSegment.cols; y++) {
				if (segmentMask.at<uchar>(x, y) != 0) {
					srcImgSegment.at<Vec3b>(x, y) = mainOpening.at<Vec3b>(x, y);
				}
				else {
					srcImgSegment.at<Vec3b>(x, y) = Vec3b(0, 0, 0);
				}
			}
		}

		twoDimage = mainOpening.reshape(1, mainOpening.rows * mainOpening.cols);
		twoDimage.convertTo(twoDimage, CV_32FC3);

		kmeans(twoDimage, K, label, criteria, attempts, NULL, center);
		center.convertTo(center, CV_8U);
		label = label.reshape(1, 1);
		label.convertTo(label, CV_8UC1);

		// Preparation loop

		for (int cluster = 0; cluster < center.rows; cluster++) {

			cout << cluster << endl;

			for (int x = 0; x != maskedImage.rows; x++) {
				for (int y = 0; y != maskedImage.cols; y++) {
					if (static_cast<int>(label.at<uchar>(0, x * maskedImage.cols + y)) == cluster) {
						maskedImage.at<Vec3b>(x, y) = srcImg.at<Vec3b>(x, y);
					}
					else {
						maskedImage.at<Vec3b>(x, y) = Vec3b(0, 0, 0);
					}
				}
			}

			cvtColor(maskedImage, grayMask, COLOR_BGR2GRAY);
			threshold(grayMask, threshed, 1, 255, THRESH_BINARY);

			opening = threshed.clone();

			dilate(opening, sure_bg, kernel, Point(-1, 1), 3);
			distanceTransform(opening, dist_transform, DIST_L2, 5);

			minMaxLoc(dist_transform, &minVal, &maxVal, &minLoc, &maxLoc);

			threshold(dist_transform, sure_fg, 0.1 * maxVal, 255, 0);

			sure_fg.convertTo(sure_fg, CV_8U);
			subtract(sure_bg, sure_fg, unknown);

			connectedComponents(sure_fg, markers);

			for (int x = 0; x != markers.rows; x++) {
				for (int y = 0; y != markers.cols; y++) {
					if (unknown.at<uchar>(x, y) == 255) {
						markers.at<int>(x, y) = 0;
					}
					else {
						markers.at<int>(x, y) = markers.at<int>(x, y) + 1;
					}
				}
			}

			watershed(mainOpening, markers);

			totMarkers.push_back(markers);

			unique_markers.clear();

			for (int x = 0; x != markers.rows; x++) {
				for (int y = 0; y != markers.cols; y++) {
					val = markers.at<int>(x, y);
					if (val > 1 && (find(unique_markers.begin(), unique_markers.end(), val) == unique_markers.end())) {
						unique_markers.push_back(val);
					}
				}
			}

			totUniqueMarkers.push_back(unique_markers);

			markers.convertTo(markers, CV_32F);

			tindexes.clear();

			for (int i = 0; i != unique_markers.size(); i++) {
				um = unique_markers[i];
				threshold(markers, target, um - 1, 255, THRESH_BINARY);
				threshold(markers, remaining, um, 255, THRESH_BINARY);
				subtract(target, remaining, target);
				target.convertTo(target, CV_8U);

				bBox = boundingRect(target);
				centroid.x = static_cast<float>(bBox.x) + (static_cast<float>(bBox.width) / 2.0f);
				centroid.y = static_cast<float>(bBox.y) + (static_cast<float>(bBox.height) / 2.0f);

				if (avgColourFromMask(srcImgSegment, target) != Vec3b(0, 0, 0) && bBox.width * bBox.height > 50) {
					//cout << bBox.width * bBox.height << endl;
					
					centroids.push_back(centroid);

					mapColours.push_back(avgColourFromMask(srcMap, target));
					avgColours.push_back(avgColourFromMask(srcImg, target));
					colourStdev.push_back(stdFromMask(srcImg, target, avgColours[tot]));

					tindexes.push_back(tot);
					tot++;
				}
				else {
					tindexes.push_back(-1);
				}
			}
			indexes.push_back(tindexes);
		}

		std::cout << "Generation arrays prepared (but not validated)" << endl;

		// Generation loop

		// This step is very slow - some elements could potentially be promoted to GPU operations

		for (int cluster = 0; cluster < center.rows; cluster++) {
			cout << cluster << endl;

			markers = totMarkers[cluster];
			markers.convertTo(markers, CV_32F);
			unique_markers = totUniqueMarkers[cluster];

			tindexes = indexes[cluster];

			for (int i = 0; i != unique_markers.size(); i++) {
				um = unique_markers[i];
				if (tindexes[i] == -1) {
					continue;
				}
				thisMapColour = mapColours[tindexes[i]];
				thisAvgColour = avgColours[tindexes[i]];
				thisColourStd = colourStdev[tindexes[i]];

				threshold(markers, target, um - 1, 255, THRESH_BINARY);
				threshold(markers, remaining, um, 255, THRESH_BINARY);
				subtract(target, remaining, target);
				GaussianBlur(target, target, Size(11, 11), 5.0, 5.0);

				tempCentroids.clear();
				copy(centroids.begin(), centroids.end(), back_inserter(tempCentroids));
				tempCentroids.erase(tempCentroids.begin() + tindexes[i]);

				tempMapColours.clear();
				copy(mapColours.begin(), mapColours.end(), back_inserter(tempMapColours));
				tempMapColours.erase(tempMapColours.begin() + tindexes[i]);

				tempAvgColours.clear();
				copy(avgColours.begin(), avgColours.end(), back_inserter(tempAvgColours));
				tempAvgColours.erase(tempAvgColours.begin() + tindexes[i]);

				for (int x = 0; x != target.rows; x++) {
					for (int y = 0; y != target.cols; y++) {
						if (srcImgSegment.at<Vec3b>(x, y) == Vec3b(0, 0, 0)) {
							continue;
						}
						closeIndex = -1;
						if (target.at<float>(x, y) > 0.1) {
							coord.x = static_cast<float>(y);
							coord.y = static_cast<float>(x);
							minDist = 10000.0f;
							for (int j = 0; j != tempCentroids.size(); j++) {
								currentDist = pointDist(coord, tempCentroids[j]);
								if (currentDist < minDist) {
									minDist = currentDist;
									closeIndex = j;
								}
							}
							if (closeIndex == -1) {
								continue;
							}
							closestMapColour = tempMapColours[closeIndex];
							closestAvgColour = tempAvgColours[closeIndex];
							minProb = colourPval(closestAvgColour, thisAvgColour, thisColourStd);
							d = (colourPval(srcImg.at<Vec3b>(x, y), thisAvgColour, thisColourStd) - minProb) / (1 - minProb);
							if (d > 1.0f) {
								d = 1.0f;
							}
							if (d < 0.0f) {
								d = 0.0f;
							}
							d = 1 - d;
							d *= d;
							d = 1 - d;
							calculatedColour = (static_cast<Vec3f>(thisMapColour) * d + static_cast<Vec3f>(outMap.at<Vec3b>(x, y)) * (1 - d));
							//calculatedColour = (static_cast<Vec3f>(thisMapColour) * d + static_cast<Vec3f>(closestMapColour) * (1 - d));
							outMap.at<Vec3b>(x, y) = static_cast<Vec3b>(calculatedColour);
						}
					}
				}
			}
		}
		//cv::namedWindow("Output");
		//while (true) {
		//	imshow("Output", outMap);
		//	char c = (char)waitKey(25); //Waits for us to press 'Esc', then exits
		//	if (c == 27) {
		//		cv::destroyWindow("Output");
		//		break;
		//	}
		//	if (getWindowProperty("Output", WND_PROP_VISIBLE) < 1) {
		//		break;
		//	}
		//}
	}

	cv::namedWindow("Output");
	while (true) {
		imshow("Output", outMap);
		char c = (char)waitKey(25); //Waits for us to press 'Esc', then exits
		if (c == 27) {
			cv::destroyWindow("Output");
			break;
		}
		if (getWindowProperty("Output", WND_PROP_VISIBLE) < 1) {
			break;
		}
	}
	imwrite("OSNormal_backup.png", outMap);
	OSNormalMap = outMap;
}