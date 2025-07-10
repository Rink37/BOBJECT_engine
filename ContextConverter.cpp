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
	x = fabs(x) / sqrt(2.0f);

	double t = 1.0 / (1.0 + p * x);
	double y = 1.0 - (((((a5 * t + a4) * t) + a3) * t + a2) * t + a1) * t * exp(-x * x);

	return static_cast<float>(0.5 * (1.0 + sign * y));
}

Mat maskImage(Mat input, Mat mask) {
	Mat output = input.clone();
	for (int y = 0; y != input.rows; y++) {
		for (int x = 0; x != input.cols; x++) {
			if (mask.at<uchar>(y, x) != 0) {
				output.at<Vec3b>(y, x) = input.at<Vec3b>(y,x);
			}
			else {
				output.at<Vec3b>(y, x) = Vec3b(0, 0, 0);
			}
		}
	}
	return output;
}

Vec3b avgColourFromMask(Mat srcImg, Mat mask, Rect ROI) {
	
	Vec3f sumColour;
	int sum = 0;
	int darksum = 0;
	for (int y = ROI.y; y != ROI.y + ROI.height; y++) {
		for (int x = ROI.x; x != ROI.x+ROI.width; x++){
			if (mask.at<uchar>(y, x) != 0){//&& srcImg.at<Vec3b>(y, x) != Vec3b(0, 0, 0)) {
				if (srcImg.at<Vec3b>(y, x) == Vec3b(0, 0, 0)) {
					darksum++;
					sum++;
				}
				else {
					sumColour += static_cast<Vec3f>(srcImg.at<Vec3b>(y, x)) / 255.0f;
					sum++;
				}
			}
		}
	}
	if (sum != 0 && (static_cast<float>(darksum)/static_cast<float>(sum) < 0.5f)) {
		return static_cast<Vec3b>(255 * sumColour / (sum-darksum));
	}
	else {
		return Vec3b(0, 0, 0);
	}
}

Vec3f stdFromMask(Mat srcImg, Mat mask, Vec3b avgColour, Rect ROI) {
	Vec3f currentColour;
	float c0 = 0;
	float c1 = 0;
	float c2 = 0;
	int sum = 0;
	for (int y = ROI.y; y != ROI.y + ROI.height; y++) {
		for (int x = ROI.x; x != ROI.x + ROI.width; x++) {
			if (mask.at<uchar>(y, x) != 0) {
				currentColour = static_cast<Vec3f>(srcImg.at<Vec3b>(y, x));
				if (currentColour == Vec3f(0.0f, 0.0f, 0.0f)) {
					continue;
				}
				c0 += (currentColour[0] * currentColour[0] - 2 * currentColour[0] * static_cast<float>(avgColour[0]) + static_cast<float>(avgColour[0] * avgColour[0]));
				c1 += (currentColour[1] * currentColour[1] - 2 * currentColour[1] * static_cast<float>(avgColour[1]) + static_cast<float>(avgColour[1] * avgColour[1]));
				c2 += (currentColour[2] * currentColour[2] - 2 * currentColour[2] * static_cast<float>(avgColour[2]) + static_cast<float>(avgColour[2] * avgColour[2]));
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
	//colourP /= 0.5f;
	return colourP;
}

float pointDist(Point2f pointA, Point2f pointB){
	return static_cast<float>(norm( pointA - pointB ));
}

float colourDist(Vec3b colourA, Vec3b colourB) {
	return static_cast<float>(norm( colourA - colourB ));
}
void NormalGen::contextualConvertMap(Mat srcImg) {

	// Function attempts to map the object-space normal map so that each different colour in the input image appears as a flat plane on the result

	// I'm working on optimization currently: For simplicity it is assumed that any opencv function is the fastest way to perform that operation

	Mat segmentMask;
	Mat srcMap;
	Mat outMap;
	Mat islands;
	Mat islandMarkers;
	int islandMarker = 0;
	vector<int> islandUniqueMarkers;

	resize(OSNormalMap, srcMap, Size(srcImg.cols, srcImg.rows), INTER_LINEAR);
	outMap = srcMap.clone();
	Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));

	int val;

	// First we want to separate unique UV islands so that we can process islands separately

	Mat trackingMap;
	resize(srcMap, trackingMap, Size(512 * srcImg.cols / srcImg.rows, 512), INTER_LINEAR);
	Mat trackingImg;
	resize(srcImg, trackingImg, Size(512 * srcImg.cols / srcImg.rows, 512), INTER_LINEAR);

	Mat srcImgSegment = trackingImg.clone();

	float scalefac = static_cast<float>(srcImg.rows) / 512.0f;

	cvtColor(trackingMap, islands, COLOR_BGR2GRAY);
	threshold(islands, islands, 1, 255, THRESH_BINARY);

	connectedComponents(islands, islandMarkers);
	
	for (int x = 0; x != islandMarkers.rows; x++) {
		for (int y = 0; y != islandMarkers.cols; y++) {
			val = islandMarkers.at<int>(x, y)+1;
			islandMarkers.at<int>(x, y) = val;
			if (val >= 0 && (find(islandUniqueMarkers.begin(), islandUniqueMarkers.end(), val) == islandUniqueMarkers.end())) {
				islandUniqueMarkers.push_back(val);
			}
		}
	}
	
	islandMarkers.convertTo(islandMarkers, CV_32F);

	Mat mainOpening;
	Mat twoDimage;

	uint32_t imageSize = trackingImg.rows * trackingImg.cols;
	uint32_t segmentSize = imageSize;

	int K = 25;
	int tempK = K;
	Rect segmentRect;
	int attempts = 10;

	Mat label;
	Mat center;
	TermCriteria criteria(3, 10, 1.0);

	Mat opening = trackingImg.clone();
	cvtColor(opening, opening, COLOR_BGR2GRAY);
	Mat sure_bg;
	Mat sure_fg;
	Mat dist_transform;
	Mat unknown;
	Mat markers;
	Mat target;
	vector<vector<Point> > contours;

	int um;
	int tot;

	vector<int> tindexes;

	Rect bBox;
	Point2f centroid;

	Vec3f calculatedColour;

	float minDist;
	float currentDist;
	Point2f coord;

	float d;

	int closeIndex = 0;

	//double matMax = 0;
	//double matMin = 0;
	//Point maxloc;
	//Point minloc;
	float minProb;

	Vec3b SourceCol;

	//morphologyEx(trackingImg, mainOpening, MORPH_OPEN, kernel, Point(-1, -1), 2);
	mainOpening = trackingImg.clone();

	for (int m = 1; m != islandUniqueMarkers.size()-1; m++) {
		islandMarker = islandUniqueMarkers[m];
		inRange(islandMarkers, islandMarker, islandMarker, segmentMask);
		segmentMask.convertTo(segmentMask, CV_8U);

		segmentRect = boundingRect(segmentMask);
		segmentSize = segmentRect.area();

		tempK = 5 + static_cast<uint32_t>(sqrt(static_cast<float>(segmentSize) / static_cast<float>(imageSize)) * static_cast<float>(K));
		
		cout << "Segment " << m << "/" << islandUniqueMarkers.size() << " has " << tempK << " layers:" << endl;

		vector<Mat> totMarkers; 
		vector<Point2f> centroids;
		vector<Vec3b> mapColours;
		vector<Vec3b> avgColours;
		vector<Vec3f> colourStdev;
		vector<vector<int>> totUniqueMarkers;
		vector<int> unique_markers;
		vector<vector<int>> indexes;
		vector<Rect> totRects;
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

		//cv::namedWindow("Output");
		//while (true) {
		//	imshow("Output", srcImgSegment);
		//	char c = (char)waitKey(25); //Waits for us to press 'Esc', then exits
		//	if (c == 27) {
		//		cv::destroyWindow("Output");
		//		break;
		//	}
		//	if (getWindowProperty("Output", WND_PROP_VISIBLE) < 1) {
		//		break;
		//	}
		//}

		twoDimage = srcImgSegment.reshape(1, srcImgSegment.rows * srcImgSegment.cols);
		twoDimage.convertTo(twoDimage, CV_32FC3);

		kmeans(twoDimage, tempK, label, criteria, attempts, NULL, center);
		center.convertTo(center, CV_8U);
		label = label.reshape(1, 1);
		label.convertTo(label, CV_8UC1);

		// Preparation loop

		for (int cluster = 0; cluster < center.rows; cluster++) {

			cout << cluster << endl;

			// First we find threshed, which is an image representing the pixels of the image which are in the given cluster
			for (int x = 0; x != opening.rows; x++) {
				for (int y = 0; y != opening.cols; y++) {
					if (static_cast<int>(label.at<uchar>(0, x * opening.cols + y)) == cluster) {
						opening.at<uchar>(x, y) = 255;
					}
					else {
						opening.at<uchar>(x, y) = 0;
					}
				}
			}

			dilate(opening, sure_bg, kernel, Point(-1, 1), 3);
			distanceTransform(opening, dist_transform, DIST_L2, 5);

			threshold(dist_transform, sure_fg, 3.0, 255, 0);

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

			watershed(trackingImg, markers);
			//resize(markers, markers, Size(srcImg.cols, srcImg.rows), INTER_LINEAR);

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
				inRange(markers, um, um, target);
				target.convertTo(target, CV_8U);

				bBox = boundingRect(target);

				if (bBox.width == srcImgSegment.cols && bBox.height == srcImgSegment.rows) {
					continue;
				}

				SourceCol = avgColourFromMask(srcImgSegment, target, bBox);
				
				if (SourceCol != Vec3b(0, 0, 0) && bBox.area() > 20) {

					centroid.x = (static_cast<float>(bBox.x) + (static_cast<float>(bBox.width) / 2.0f)) * scalefac;
					centroid.y = (static_cast<float>(bBox.y) + (static_cast<float>(bBox.height) / 2.0f)) * scalefac;
					
					centroids.push_back(centroid);

					mapColours.push_back(avgColourFromMask(trackingMap, target, bBox));
					avgColours.push_back(SourceCol);
					colourStdev.push_back(stdFromMask(trackingImg, target, SourceCol, bBox));
					totRects.push_back(bBox);

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

		Vec3b thisMapColour;
		Vec3b thisAvgColour;
		Vec3f thisColourStd;

		Vec3b closestMapColour;
		Vec3b closestAvgColour;

		resize(srcImgSegment, srcImgSegment, Size(srcImg.cols, srcImg.rows), INTER_LINEAR);

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

				inRange(markers, um, um, target);
				target.convertTo(target, CV_32F);
				GaussianBlur(target, target, Size(11, 11), 5.0, 5.0);
				resize(target, target, Size(srcImg.cols, srcImg.rows), INTER_NEAREST);

				//cv::namedWindow("Output");
				//while (true) {
				//	imshow("Output", target);
				//	char c = (char)waitKey(25); //Waits for us to press 'Esc', then exits
				//	if (c == 27) {
				//		cv::destroyWindow("Output");
				//		break;
				//	}
				//	if (getWindowProperty("Output", WND_PROP_VISIBLE) < 1) {
				//		break;
				//	}
				//}
				
				bBox = totRects[tindexes[i]];

				vector<Point2f> allCentroids;
				std::copy(centroids.begin(), centroids.end(), back_inserter(allCentroids));
				allCentroids.erase(allCentroids.begin() + tindexes[i]);

				vector<Vec3b> tempMapColours;
				std::copy(mapColours.begin(), mapColours.end(), back_inserter(tempMapColours));
				tempMapColours.erase(tempMapColours.begin() + tindexes[i]);

				vector<Vec3b> tempAvgColours;
				std::copy(avgColours.begin(), avgColours.end(), back_inserter(tempAvgColours));
				tempAvgColours.erase(tempAvgColours.begin() + tindexes[i]);

				vector<int> idxsToKeep;

				for (int j = 0; j != allCentroids.size(); j++) {
					currentDist = pointDist(centroids[tindexes[i]], allCentroids[j]);
					if (currentDist < 50 * scalefac * scalefac) {
						idxsToKeep.push_back(j);
					}
				}

				vector<Point2f> tempCentroids;

				for (int j = 0; j != idxsToKeep.size(); j++) {
					tempCentroids.push_back(allCentroids[idxsToKeep[j]]);
				}

				for (int x = static_cast<int>((bBox.y - bBox.height) * scalefac); x != static_cast<int>((bBox.y + 2*bBox.height) * scalefac); x++) {
					for (int y = static_cast<int>((bBox.x - bBox.width) * scalefac); y != static_cast<int>((bBox.x + 2*bBox.width) * scalefac); y++) {
						if (x > srcImg.rows - 1 || x < 0) {
							continue;
						}
						if (y > srcImg.cols - 1 || y < 0) {
							continue;
						}
						//if (srcImgSegment.at<Vec3b>(x, y) == Vec3b(0, 0, 0)) {
						//	continue;
						//}
						closeIndex = -1;
						if (target.at<float>(x, y) > 0.1) {
						//if (true){

							coord.x = static_cast<float>(y);
							coord.y = static_cast<float>(x);
							
							minDist = 10000.0f;
							array<int, 2> closestPoints = { 0,0 };
							int numOfMinDists = 0;
							for (int j = 0; j != tempCentroids.size(); j++) {
								currentDist = pointDist(coord, tempCentroids[j]);
								if (currentDist < minDist) {
									minDist = currentDist;
									closestPoints[1] = closestPoints[0];
									closestPoints[0] = idxsToKeep[j];
									closeIndex = idxsToKeep[j];
									numOfMinDists++;
								}
							}
							numOfMinDists = 0;
							if (numOfMinDists > 1) {
								float totDist = pointDist(allCentroids[closestPoints[0]], allCentroids[closestPoints[1]]);
								float gradient = static_cast<float>((allCentroids[closestPoints[0]].y - allCentroids[closestPoints[1]].y)) / static_cast<float>((allCentroids[closestPoints[0]].x - allCentroids[closestPoints[1]].x));
								float intersect = allCentroids[closestPoints[0]].y - allCentroids[closestPoints[0]].x * gradient;
								float invGradient = -1.0f / gradient;
								float invIntersect = coord.y - coord.x * invGradient;
								float intersectX = (invIntersect - intersect) / (gradient - invGradient);
								float intersectY = gradient * intersectX + intersect;

								Point2f intersectCoord;
								intersectCoord.x = intersectX;
								intersectCoord.y = intersectY;

								float curDist = (pointDist(allCentroids[closestPoints[0]], intersectCoord)) / totDist;
								closestAvgColour = tempAvgColours[closestPoints[0]] * (1-curDist) + tempAvgColours[closestPoints[1]] * curDist;
							}
							else {
								if (closeIndex == -1) {
									continue;
								}
								closestAvgColour = tempAvgColours[closeIndex];
							}
							minProb = colourPval(closestAvgColour, thisAvgColour, thisColourStd);
							d = (colourPval(srcImg.at<Vec3b>(x, y), thisAvgColour, thisColourStd) - minProb) / (1 - minProb); 
							if (d > 1.0f) {
								d = 1.0f;
							}
							if (d < 0.0f) {
								d = 0.0f;
							}
							//d = 2.0f * d - d*d;
							//d *= d;
							calculatedColour = (static_cast<Vec3f>(thisMapColour) * d + static_cast<Vec3f>(outMap.at<Vec3b>(x, y)) * (1 - d));
							outMap.at<Vec3b>(x, y) = static_cast<Vec3b>(calculatedColour);
						}
					}
				}
			}
		}
		resize(srcImgSegment, srcImgSegment, Size(trackingImg.cols, trackingImg.rows), INTER_LINEAR);
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
	imwrite("OSNormal_sqbackup.png", outMap);
	OSNormalMap = outMap;
}