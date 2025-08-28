#include"ImageProcessor.h"

using namespace cv;
using namespace std;

float eucDist(Point a, Point b) {
	return sqrtf(powf(a.x - b.x, 2) + powf(a.y - b.y, 2));
}

void match_templates(Mat src, Mat target, Mat& out) {
	int division = 64;
	uint8_t offset = 2;
	
	int xdivs = src.rows / division;
	int ydivs = src.cols / division;

	vector<Point> initialPositions = {};
	vector<Point> matchedPositions = {};

	for (uint16_t y = 0; y != ydivs; y++) {
		for (uint16_t x = 0; x != xdivs; x++) {
			for (uint8_t dy = 0; dy != offset; dy++) {
				for (uint8_t dx = 0; dx != offset; dx++) {
					if (((y + 1) * division + dy * division / offset) < src.cols && ((x + 1) * division + dx * division / offset) < src.rows) {
						Mat temp = src(Range(x * division + dx * division / offset, (x + 1) * division + dx * division / offset), Range(y * division + dy * division / offset, (y + 1) * division + dy * division / offset));
						Mat res;
						matchTemplate(target, temp, res, TM_CCORR_NORMED);

						double min_val;
						double max_val;
						Point min_loc;
						Point matchedPosition;

						minMaxLoc(res, &min_val, &max_val, &min_loc, &matchedPosition);

						Point initialPosition(Vec2i(x * division + dx * division / offset, y * division + dy * division / offset));

						if (eucDist(initialPosition, matchedPosition) < division * 2) {
							initialPositions.push_back(initialPosition);
							matchedPositions.push_back(matchedPosition);
						}

					}
				}
			}
		}
	}
	Mat M;
	findHomography(initialPositions, matchedPositions, M, RANSAC, 5.0);

	warpPerspective(target, out, M, Size(src.rows, src.cols));
}