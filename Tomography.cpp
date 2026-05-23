#include"Tomography.h"
#include<cassert>

using namespace cv;
using namespace std;

static float eucDist(Point a, Point b) {
	return sqrtf(powf(a.x - b.x, 2) + powf(a.y - b.y, 2));
}

const int MAX_FEATURES = 5000;
const float GOOD_MATCH_PERCENT = 0.05f;

static void change_contrast(Mat* img, float alpha, int beta) {
	if (img->channels() == 3) {
		for (int y = 0; y < img->rows; y++) {
			for (int x = 0; x < img->cols; x++) {
				for (int c = 0; c < img->channels(); c++) {
					img->at<Vec3b>(y, x)[c] = saturate_cast<uchar>(alpha * img->at<Vec3b>(y, x)[c] + beta);
				}
			}
		}
	}
	else {
		for (int y = 0; y < img->rows; y++) {
			for (int x = 0; x < img->cols; x++) {
				img->at<uchar>(y, x) = saturate_cast<uchar>(alpha * img->at<uchar>(y, x) + beta);
			}
		}
	}
}

static vector<float> calculateCDF(Mat hist) {
	vector<float> cdf;
	uint32_t currentSum = 0;
	for (int i = 0; i != 256; i++) {
		currentSum += static_cast<uint32_t>(hist.at<float>(i));
		cdf.push_back(currentSum);
	}
	for (int i = 0; i != 256; i++) {
		cdf[i] /= currentSum;
	}
	return cdf;
}

static vector<uint8_t> calculateLUT(vector<float> refCDF, vector<float>srcCDF) {
	vector<uint8_t> lookup;
	uint8_t lookup_val = 0;
	for (int i = 0; i != 256; i++) {
		for (int j = 0; j != 256; j++) {
			if (refCDF[j] >= srcCDF[i]) {
				lookup_val = j;
				break;
			}
		}
		lookup.push_back(lookup_val);
	}
	return lookup;
}

static Mat getDiffuseGray(Mat img) {

	// This is a functional implementation of the algorithm but success will depend on the image being adequately illuminantly normalized
	// This also does not appear to play very nicely with naturally light brushstrokes, so I'm not sure what to do about that

	Mat PSF_img = img.clone();

	uint32_t IminTotal = 0;

	Mat Imins;
	cvtColor(img, Imins, COLOR_BGR2GRAY);

	for (int x = 0; x != img.cols; x++) {
		for (int y = 0; y != img.rows; y++) {
			Vec3b I;
			uint8_t I_min = 255;
			I = img.at<Vec3b>(x, y);
			for (int k = 0; k != 3; k++) {
				if (I[k] < I_min) {
					I_min = I[k];
				}
			}
			IminTotal += I_min;
			PSF_img.at<Vec3b>(x, y) = Vec3b(I[0] - I_min, I[1] - I_min, I[2] - I_min);
		}
	}

	float beta = static_cast<float>(IminTotal) / static_cast<float>(img.rows * img.cols);

	cvtColor(img, img, COLOR_BGR2YCrCb);
	cvtColor(PSF_img, PSF_img, COLOR_BGR2YCrCb);

	vector<Mat> img_channels(3);
	split(img, img_channels);

	vector<Mat> PSF_img_channels(3);
	split(PSF_img, PSF_img_channels);

	Mat U = img_channels[0].clone();

	for (int x = 0; x != img.cols; x++) {
		for (int y = 0; y != img.rows; y++) {
			U.at<uint8_t>(x, y) = static_cast<uint8_t>((Imins.at<uint8_t>(x, y) < 4 * beta));
		}
	}

	threshold(U, U, 0.5, 255, THRESH_BINARY);

	Mat imgHist;
	Mat PSFimgHist;

	float range[] = { 0, 256 };
	const float* histRange[] = { range };
	int histSize = 256;

	bool uniform = true, accumulate = false;

	calcHist(&img_channels[0], 1, 0, U, imgHist, 1, &histSize, histRange, uniform, accumulate);
	calcHist(&PSF_img_channels[0], 1, 0, U, PSFimgHist, 1, &histSize, histRange, uniform, accumulate);

	vector<float> imgHistCDF = calculateCDF(imgHist);
	vector<float> PSFimgHistCDF = calculateCDF(PSFimgHist);

	vector<uint8_t> lookup = calculateLUT(imgHistCDF, PSFimgHistCDF);

	Mat imgMatched = PSF_img_channels[0].clone();
	LUT(imgMatched, lookup, imgMatched);

	imshow("Matched", imgMatched);
	waitKey(0);

	Mat diffuse = imgMatched.clone();

	for (int x = 0; x != img.cols; x++) {
		for (int y = 0; y != img.rows; y++) {
			if (imgMatched.at<uint8_t>(x, y) < img_channels[0].at<uint8_t>(x, y)) {
				diffuse.at<uint8_t>(x, y) = imgMatched.at<uint8_t>(x, y);
			}
			else {
				diffuse.at<uint8_t>(x, y) = img_channels[0].at<uint8_t>(x, y);
			}
		}
	}

	imshow("Original", img_channels[0]);
	waitKey(0);

	imshow("Diffuse", diffuse);
	waitKey(0);

	return diffuse;
}

static float lineLength(Vec4i l) {
	return sqrt(static_cast<float>((l[0] - l[2]) * (l[0] - l[2]) + (l[1] - l[3]) * (l[1] - l[3])));
}

static float angleBetweenLines(Vec4i l1, Vec4i l2) {
	Vec2f l1_dir = Vec2f(static_cast<float>(l1[2] - l1[0]) / lineLength(l1), static_cast<float>(l1[3] - l1[1]) / lineLength(l1));
	Vec2f l2_dir = Vec2f(static_cast<float>(l2[2] - l2[0]) / lineLength(l2), static_cast<float>(l2[3] - l2[1]) / lineLength(l2));
	float angle = acos(l1_dir[0] * l2_dir[0] + l1_dir[1] * l2_dir[1]);
	return angle * 180.0f / 3.14159265f;
}

static Point rotate(Point a, float angle) {
	float ang = - angle * 3.14159265f / 180.0f;
	return Point(a.x * cos(ang) - a.y * sin(ang), a.x * sin(ang) + a.y * cos(ang));
}

static void rotateLines(Vec4i& l1, Vec4i& l2, float angle, Point rotationCenter) {
	Point l1_1 = Point(l1[0], l1[1]);
	Point l1_2 = Point(l1[2], l1[3]);
	Point l2_1 = Point(l2[0], l2[1]);
	Point l2_2 = Point(l2[2], l2[3]);
	l1_1 -= rotationCenter;
	l1_2 -= rotationCenter;
	l2_1 -= rotationCenter;
	l2_2 -= rotationCenter;
	l1_1 = rotate(l1_1, angle);
	l1_2 = rotate(l1_2, angle);
	l2_1 = rotate(l2_1, angle);
	l2_2 = rotate(l2_2, angle);
	l1_1 += rotationCenter;
	l1_2 += rotationCenter;
	l2_2 += rotationCenter;
	l2_1 += rotationCenter;
	l1 = Vec4i(l1_1.x, l1_1.y, l1_2.x, l1_2.y);
	l2 = Vec4i(l2_1.x, l2_1.y, l2_2.x, l2_2.y);
}

static Point intersectionOfLines(Vec4i l1, Vec4i l2) {
	float x_1 = l1[0];
	float x_2 = l1[2];
	float x_3 = l2[0];
	float x_4 = l2[2];

	float y_1 = l1[1];
	float y_2 = l1[3];
	float y_3 = l2[1];
	float y_4 = l2[3];

	float determinant = (x_1 - x_2) * (y_3 - y_4) - (y_1 - y_2) * (x_3 - x_4);
	float x = (x_1 * y_2 - y_1 * x_2) * (x_3 - x_4) - (x_1 - x_2) * (x_3 * y_4 - y_3 * x_4);
	float y = (x_1 * y_2 - y_1 * x_2) * (y_3 - y_4) - (y_1 - y_2) * (x_3 * y_4 - y_3 * x_4);

	if (determinant == 0.0f) {
		// We assume that this function will never be called on parallel lines, so this case is only found if lines are perfectly vertical/horizontal
		if (l1[0] - l1[2] == 0.0f && l2[0] - l2[2] != 0.0f) {
			x = l1[0];
		}
		else if (l1[0] - l1[2] != 0.0f && l2[0] - l2[2] == 0.0f) {
			x = l2[0];
		}
		else {
			x = -1.0f;
		}
		if (l1[1] - l1[3] == 0.0f && l2[1] - l2[3] != 0.0f) {
			y = l1[1];
		}
		else if (l1[1] - l1[3] != 0.0f && l2[1] - l2[3] == 0.0f) {
			y = l2[1];
		}
		else {
			y = -1.0f;
		}
		y = (x == -1.0f) ? -1.0f : y;
		x = (y == -1.0f) ? -1.0f : x;
		return Point(x, y);
	}
	Point intersect = Point(abs(x / determinant), abs(y / determinant));
	return intersect;
}

static float pointDist(Point a, Point b) {
	return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

static int findCornerType(Vec4i l1, Vec4i l2, Point cornerPos) {
	// Assume that lines have been rotated so that they match the newly rotated image
	Point l1_1 = Point(l1[0], l1[1]);
	Point l1_2 = Point(l1[2], l1[3]);
	Point l2_1 = Point(l2[0], l2[1]);
	Point l2_2 = Point(l2[2], l2[3]);
	Point a;
	Point b;
	if (pointDist(l1_1, cornerPos) > pointDist(l1_2, cornerPos)) {
		a = l1_1;
	}
	else {
		a = l1_2;
	}
	if (pointDist(l2_1, cornerPos) > pointDist(l2_2, cornerPos)) {
		b = l2_1;
	}
	else {
		b = l2_2;
	}
	float max_x_dist = (abs(a.x - cornerPos.x) > abs(b.x - cornerPos.x))? a.x - cornerPos.x: b.x - cornerPos.x;
	float max_y_dist = (abs(a.y - cornerPos.y) > abs(b.y - cornerPos.y)) ? a.y - cornerPos.y : b.y - cornerPos.y;
	std::cout << max_x_dist << " " << max_y_dist << std::endl;
	if (max_x_dist > 0.0f && max_y_dist > 0.0f) {
		return 0; // Top left
	}
	else if (max_x_dist < 0.0f && max_y_dist > 0.0f) {
		return 1; // Top right
	}
	else if (max_x_dist < 0.0f && max_y_dist < 0.0f) {
		return 2; // Bottom right
	}
	return 3; // Bottom left
}

static float estMaxDist(Point corner, Size imageSize, float rotation) {
	// Need a function which can determine the maximum size we should check for so we can reduce the number of scale iterations
	Point a = Point(0.0f, 0.0f);
	Point b = Point(0.0f, imageSize.height);
	Point c = Point(imageSize.width, 0.0f);
	Point d = Point(imageSize.width, imageSize.height);
	Point furthestCorner;
	float furthestDist = 0.0f;
	vector<Point> imageCorners = { a, b, c, d };
	for (Point imgCorner : imageCorners) {
		float dist = pointDist(imgCorner, corner);
		if (dist > furthestDist) {
			furthestDist = dist;
			furthestCorner = imgCorner;
		}
	}
	Vec4i line = Vec4i(furthestCorner.x, furthestCorner.y, corner.x, corner.y);
	Vec4i line2 = Vec4i(furthestCorner.x, furthestCorner.y, corner.x, corner.y);
	rotateLines(line, line2, rotation, corner);
	Point furthestCornerRotated = Point(line[0], line[1]) - corner;
	float maxDist = max(abs(furthestCornerRotated.x), abs(furthestCornerRotated.y));
	std::cout << "Maximum distance = " << maxDist << std::endl;
	return maxDist;
}

static Point getCurrentCorner(int cornerType, Point center, int width, int height) {
	Point currentCorner;
	int x = 0;
	int y = 0;
	switch (cornerType) {
	case (0):
		// Top left
		x = -width / 2;
		y = -height / 2;
		break;
	case (1):
		// Top right
		x = width / 2;
		y = -height / 2;
		break;
	case (2):
		// Bottom right
		x = width / 2;
		y = height / 2;
		break;
	case(3):
		// Bottom left
		x = -width / 2;
		y = height / 2;
		break;
	default:
		break;
	}
	currentCorner = Point(x, y) + center;
	return currentCorner;
}

static bool match_template(Mat src, Mat* target, Size outdims, float& rotation) {
	Mat srcGray, targetGray;

	vector<Point2f> srcPoints, matchPoints;

	cvtColor(src, srcGray, COLOR_BGR2GRAY);
	cvtColor(*target, targetGray, COLOR_BGR2GRAY);

	normalize(srcGray, srcGray, 0, 255, NORM_MINMAX);
	normalize(targetGray, targetGray, 0, 255, NORM_MINMAX);

	vector<KeyPoint> keypoints1, keypoints2;
	Mat descriptors1, descriptors2;

	Ptr<Feature2D> orb = ORB::create(MAX_FEATURES);
	orb->detectAndCompute(srcGray, Mat(), keypoints1, descriptors1);
	orb->detectAndCompute(targetGray, Mat(), keypoints2, descriptors2);

	if (descriptors1.empty() || descriptors2.empty()) {
		return false;
	}

	vector<DMatch> matches;
	Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("BruteForce-Hamming");
	matcher->match(descriptors1, descriptors2, matches, Mat());

	sort(matches.begin(), matches.end());

	const int numGoodMatches = matches.size() * GOOD_MATCH_PERCENT;
	matches.erase(matches.begin() + numGoodMatches, matches.end());

	//Mat imMatches;
	//drawMatches(srcGray, keypoints1, targetGray, keypoints2, matches, imMatches);
	//cv::imshow("Matches", imMatches);
	//cv::waitKey(0);

	int minDistance = 50000; //min(srcGray.size().width, srcGray.size().height) / 10;

	for (size_t i = 0; i < matches.size(); i++) {
		if (norm(keypoints1[matches[i].queryIdx].pt - keypoints2[matches[i].trainIdx].pt) < minDistance) {
			srcPoints.push_back(keypoints1[matches[i].queryIdx].pt);
			matchPoints.push_back(keypoints2[matches[i].trainIdx].pt);
		}
	}

	for (size_t i = 0; i < srcPoints.size(); i++) {
		srcPoints[i] = Point2f(srcPoints[i].x * outdims.width / src.cols, srcPoints[i].y * outdims.height / src.rows);
		matchPoints[i] = Point2f(matchPoints[i].x * target->cols / targetGray.cols, matchPoints[i].y * target->rows / targetGray.rows);
	}
	if (srcPoints.size() > 150) {
		Point2f rotatePoint(0.0f, -1.0f);
		Mat_<float> pm(2, 1);
		pm << rotatePoint.x, rotatePoint.y;
		
		cv::Mat h = cv::estimateAffine2D(matchPoints, srcPoints);

		cv::Mat_<float> hRot(2, 2);
		hRot = h(Rect(0.0f, 0.0f, 2, 2));

		pm = hRot * pm;

		Point2f rotatedPoint(pm(0), pm(1));
		std::cout << rotatedPoint << std::endl;

		rotation -= (std::atan2f(rotatedPoint.y, rotatedPoint.x) - std::atan2f(-1.0f, 0.0f)) * 180.0f / 3.1415926f;
		//std::cout << rotation << std::endl;

		cv::Mat matched;

		warpAffine(*target, matched, h, outdims);

		*target = matched.clone();

		return true;
	}
	return false;
}

static void calculateVector(vector<float>& lightVec, float phi, float theta) {
	// phi is the rotation around z, theta is the angle above the surface
	// We assume theta is given from the plane not from the plane normal
	// We assume that phi is given from the positive horizontal

	double pi = 3.1415926;

	theta = 90.0 - theta;
	phi = phi * pi / 180;
	theta = theta * pi / 180;

	float x = sin(theta) * cos(phi);
	float y = sin(theta) * sin(phi);
	float z = cos(theta);

	lightVec.push_back(x * -1);
	lightVec.push_back(y * -1);
	lightVec.push_back(z * -1);
}

static void matrixTranspose(vector<vector<float>> src, vector<vector<float>>& out) {
	// switches the rows and columns of the src matrix
	// Does not fail
	
	int width = static_cast<int>(src.size());
	int height = static_cast<int>(src[0].size());

	out.clear();

	for (int y = 0; y != height; y++) {
		vector<float> col;
		for (int x = 0; x != width; x++) {
			col.push_back(src[x][y]);
		}
		out.push_back(col);
	}
}

static void matrixDot(vector<vector<float>> a, vector<vector<float>> b, vector<vector<float>>& out) {
	// Consider index 0 as horizontal and index 1 as vertical i.e. value = mat[x][y]

	assert(a.size() == b[0].size()); // function only works when the row length of a matches the column length of b

	for (int x = 0; x != b.size(); x++) {
		vector<float> col;
		for (int y = 0; y != a[0].size(); y++) {
			float value = 0;
			for (int i = 0; i != a.size(); i++) {
				value += a[i][y] * b[x][i];
			}
			col.push_back(value);
		}
		out.push_back(col);
	}

}

static void printMatrix(vector<vector<float>> matrix) {
	for (int y = 0; y != matrix[0].size(); y++) {
		for (int x = 0; x != matrix.size(); x++) {
			cout << matrix[x][y] << " ";
		}
		cout << endl;
	}
	cout << endl;
}


static float matrixDeterminant(vector<vector<float>> matrix) {
	if (matrix.size() == 1 && matrix[0].size() == 1) {
		return matrix[0][0];
	}
	float determinant = 0;
	for (int x = 0; x != matrix.size(); x++) {
		vector<vector<float>> subMatrix;
		for (int i = 0; i != matrix.size(); i++) {
			if (i == x) {
				continue;
			}
			else {
				vector<float> col;
				for (int j = 1; j != matrix[0].size(); j++) {
					col.push_back(matrix[i][j]);
				}
				subMatrix.push_back(col);
			}
		}
		if (x % 2 == 0) {
			determinant += matrix[x][0] * matrixDeterminant(subMatrix);
		}
		else {
			determinant -= matrix[x][0] * matrixDeterminant(subMatrix);
		}
	}
	return determinant;
}

static void calculateCofactor(vector<vector<float>> matrix, vector<vector<float>>& out) {
	out.clear();
	for (int x = 0; x != matrix.size(); x++) {
		vector<float> cofactorCol;
		for (int y = 0; y != matrix[0].size(); y++) {
			vector<vector<float>> tempMatrix;
			for (int i = 0; i != matrix.size(); i++) {
				vector<float> col;
				if (i == x) {
					continue;
				}
				else {
					for (int j = 0; j != matrix[0].size(); j++) {
						if (j == y) {
							continue;
						}
						else {
							col.push_back(matrix[i][j]);
						}
					}
				}
				tempMatrix.push_back(col);
			}
			if ((x + y) % 2 == 0) {
				cofactorCol.push_back(matrixDeterminant(tempMatrix));
			}
			else {
				cofactorCol.push_back(-1*matrixDeterminant(tempMatrix));
			}
		}
		out.push_back(cofactorCol);
	}
}

static void matrixInverse(vector<vector<float>> matrix, vector<vector<float>>& inverse) { // compute the inverse of a square matrix
	vector<vector<float>> cofactorMatrix;
	vector<vector<float>> cofactorTranspose;
	calculateCofactor(matrix, cofactorMatrix);
	matrixTranspose(cofactorMatrix, cofactorTranspose);

	float determinant = matrixDeterminant(matrix);

	for (int x = 0; x != matrix.size(); x++) {
		vector<float> col;
		for (int y = 0; y != matrix[0].size(); y++) {
			col.push_back(cofactorTranspose[x][y] * 1.0f / determinant);
		}
		inverse.push_back(col);
	}
}

static vector<vector<float>> constructTomogMatrix(vector<int> indexes, vector<vector<float>> D) {
	vector<vector<float>> reducedD;

	for (int i = 0; i != D.size(); i++) {
		if (find(indexes.begin(), indexes.end(), i) != indexes.end()) {
			reducedD.push_back(D[i]);
		}
	}

	vector<vector<float>> DT;
	matrixTranspose(reducedD, DT);
	// DT = 3 x reducedD.size() matrix

	vector<vector<float>> Ddot;
	matrixDot(reducedD, DT, Ddot);
	// Ddot = 3x3 matrix

	vector<vector<float>> DdotInverse;
	matrixInverse(Ddot, DdotInverse);
	// DdotInverse = 3x3 matrix

	vector<vector<float>> transformationD;
	matrixDot(DT, DdotInverse, transformationD);
	// transformationD = reducedD.size() x 3 matrix

	vector<vector<float>> tomogMatrix;
	matrixTranspose(transformationD, tomogMatrix);
	// tomogMatrix = 3 x reducedD.size() matrix

	return tomogMatrix;
}

static bool checkForEmptyInArea(Mat img, int x, int y, int range) {
	for (int dx = x - range; dx != x + range + 1; dx++) {
		for (int dy = y - range; dy != y + range + 1; dy++) {
			if (dy < 0 || dy >= img.cols) {
				continue;
			}
			if (dx < 0 || dx >= img.rows) {
				continue;
			}
			if (img.at<uint8_t>(dx, dy) == 0) {
				return true;
			}
		}
	}
	return false;
}

static Mat calculateNormal(std::vector<TomogItem*> items) { // Calculates the normal texture which describes the surface of the canvas from a set of differently lit images
	// This could be made into a GPU compute operation since it's highly parallel, but I'm not sure if this would actually be faster considering the time cost of copying a vector of (presumably high resolution) images
	// Seems like CPU compute takes a few minutes so worth investigating GPU
	// D represents the list of light vectors for each image
	// Assumes that the painting is a lambertian surface

	std::vector<Texture*> images{};
	std::vector<std::vector<float>> D{};

	for (size_t i = 0; i != items.size(); i++) {
		if (items[i]->correctedImage != nullptr && items[i]->lightDirection != std::vector<float>{0.0f, 0.0f, 0.0f}) {
			images.push_back(items[i]->correctedImage);
			D.push_back(items[i]->lightDirection);
		}
	}

	assert(images.size() == D.size(), "Input vectors must be the same size");

	vector<vector<float>> tomogMatrix;

	Mat normal = images[0]->texMat.clone();
	normal = Scalar(0, 0, 0);

	map <string, vector<vector<float>>> tomogMatrices;

	vector<Mat> grayImages;

	for (int i = 0; i != images.size(); i++) {
		Mat gray;
		cvtColor(images[i]->texMat, gray, COLOR_RGB2GRAY);
		grayImages.push_back(gray);
	}

	for (int y = 0; y != normal.cols; y++) {
		if (y % 25 == 0) {
			cout << static_cast<float>(y)/static_cast<float>(normal.cols) << endl;
		}
		for (int x = 0; x != normal.rows; x++) {
			vector<vector<float>> L;
			vector<float> Lcol;
			vector<int> validIndexes;
			string keyName = "";
			for (int i = 0; i != images.size(); i++) {
				if (!checkForEmptyInArea(grayImages[i], x, y, 3)) {
					validIndexes.push_back(i);
					Lcol.push_back(grayImages[i].at<uint8_t>(x, y));
					keyName += to_string(i);
				}
			}
			if (validIndexes.size() == 0) {
				normal.at<Vec3b>(x, y) = Vec3b(127, 127, 255);
				continue;
			}

			if (tomogMatrices.find(keyName) == tomogMatrices.end()) {
				tomogMatrices.insert({ keyName, constructTomogMatrix(validIndexes, D) });
			}
			
			tomogMatrix = tomogMatrices.at(keyName);
			
			L.push_back(Lcol);

			vector<vector<float>> normalMatrix;

			matrixDot(tomogMatrix, L, normalMatrix);

			assert(normalMatrix.size() == 1 && normalMatrix[0].size() == 3);

			vector<float> normalVector = normalMatrix[0];

			float normalLength = sqrt(normalVector[0] * normalVector[0] + normalVector[1] * normalVector[1] + normalVector[2] * normalVector[2]);

			vector<int> normalPixel = { 128, 128, 128 };

			if (normalLength != 0.0f) {
				for (int i = 0; i != 3; i++) {
					normalPixel[i] = static_cast<uint8_t>(((normalVector[i] / normalLength) - 1.0f) / -2.0f * 255.0f);
				}
			}
			
			normal.at<Vec3b>(x, y) = Vec3b(normalPixel[0], normalPixel[1], normalPixel[2]);

		}
	}

	cvtColor(normal, normal, COLOR_RGB2BGR);

	return normal;
}

static Mat calculateDiffuse(std::vector<TomogItem*> items, Mat normal) {

	std::vector<Texture*> images = {};
	std::vector<std::vector<float>> D = {};

	for (size_t i = 0; i != items.size(); i++) {
		if (items[i]->correctedImage != nullptr && items[i]->lightDirection != std::vector<float>{0.0f, 0.0f, 0.0f}) {
			images.push_back(items[i]->correctedImage);
			D.push_back(items[i]->lightDirection);
		}
	}

	Mat diffuse = images[0]->texMat.clone();
	diffuse = Scalar(0, 0, 0);

	vector<Mat> grayImages;
	for (int k = 0; k != images.size(); k++) {
		Mat grayImg;
		cvtColor(images[k]->texMat, grayImg, COLOR_RGB2GRAY);
		grayImages.push_back(grayImg);
	}

	for (int y = 0; y != diffuse.cols; y++) {
		if (y % 25 == 0) {
			cout << static_cast<float>(y) / static_cast<float>(normal.cols) << endl;
		}
		for (int x = 0; x != diffuse.rows; x++) {
			Vec3f normalVector = static_cast<Vec3f>(normal.at<Vec3b>(x, y));
			for (int k = 0; k != 3; k++) {
				normalVector[k] /= 128.0f;
				normalVector[k]--;
			}
			Vec2f xyNormal = Vec2f(normalVector[0], normalVector[1]);
			float normalLength = sqrt(normalVector[0] * normalVector[0] + normalVector[1] * normalVector[1]);
			if (normalLength == 0) {
				diffuse.at<Vec3b>(x, y) = images[0]->texMat.at<Vec3b>(x, y);
				continue;
			}
			xyNormal[0] /= normalLength;
			xyNormal[1] /= normalLength;

			vector<float> weights;
			float total = 0;
			
			for (int k = 0; k != images.size(); k++) {
				if (checkForEmptyInArea(grayImages[k], x, y, 3)) {
					weights.push_back(0.0f);
					continue;
				}
				Vec2f imageVector = Vec2f(D[k][0], D[k][1]);
				float imageVectorLength = sqrt(imageVector[0] * imageVector[0] + imageVector[1] * imageVector[1]);
				imageVector[0] /= imageVectorLength;
				imageVector[1] /= imageVectorLength;

				float comp = abs(xyNormal[0] * imageVector[0] + xyNormal[1] * imageVector[1]);

				weights.push_back(1.0f - comp);
				total += weights[weights.size() - 1];
			}

			Vec3f floatPixel = Vec3f(0.0f, 0.0f, 0.0f);

			for (int k = 0; k != images.size(); k++) {
				Vec3f pixel = static_cast<Vec3f>(images[k]->texMat.at<Vec3b>(x, y));
				floatPixel[0] += pixel[0] * weights[k] / total;
				floatPixel[1] += pixel[1] * weights[k] / total;
				floatPixel[2] += pixel[2] * weights[k] / total;
			}

			diffuse.at<Vec3b>(x, y) = static_cast<Vec3b>(floatPixel);
		}
	}
	return diffuse;
}

static std::vector<Mat> calculate_norm_diff(std::vector<TomogItem*> items) {
	// This could be made into a GPU compute operation since it's highly parallel, but I'm not sure if this would actually be faster considering the time cost of copying a vector of (presumably high resolution) images
	// Seems like CPU compute takes a few minutes so worth investigating GPU
	// D represents the list of light vectors for each image
	// Assumes that the painting is a lambertian surface

	std::vector<Texture*> images = {};
	std::vector<std::vector<float>> D = {};

	for (size_t i = 0; i != items.size(); i++) {
		if (items[i]->correctedImage != nullptr && items[i]->lightDirection != std::vector<float>{0.0f, 0.0f, 0.0f}) {
			images.push_back(items[i]->correctedImage);
			//std::cout << items[i]->lightDirection[0] << " " << items[i]->lightDirection[1] << " " << items[i]->lightDirection[0] << std::endl;
			D.push_back(items[i]->lightDirection);
		}
	}

	assert(images.size() == D.size(), "Input vectors must be the same size");

	vector<vector<float>> tomogMatrix;

	Mat normal = images[0]->texMat.clone();
	normal = Scalar(0, 0, 0);

	Mat diffuse = images[0]->texMat.clone();
	diffuse = Scalar(0, 0, 0);

	map <string, vector<vector<float>>> tomogMatrices;

	vector<Mat> grayImages;

	for (int i = 0; i != images.size(); i++) {
		Mat gray;
		cvtColor(images[i]->texMat, gray, COLOR_RGB2GRAY);
		grayImages.push_back(gray);
	}

	for (int y = 0; y != normal.cols; y++) {
		for (int x = 0; x != normal.rows; x++) {
			vector<vector<float>> L;
			vector<float> Lcol;
			vector<int> validIndexes;
			string keyName = "";
			for (int i = 0; i != images.size(); i++) {
				if (!checkForEmptyInArea(grayImages[i], x, y, 3)) {
					validIndexes.push_back(i);
					Lcol.push_back(grayImages[i].at<uint8_t>(x, y));
					keyName += to_string(i);
				}
			}
			if (validIndexes.size() == 0) {
				//std::cout << "No valid indexes" << std::endl;
				normal.at<Vec3b>(x, y) = Vec3b(127, 127, 255);
				diffuse.at<Vec3b>(x, y) = images[0]->texMat.at<Vec3b>(x, y);
				continue;
			}

			if (tomogMatrices.find(keyName) == tomogMatrices.end()) {
				tomogMatrices.insert({ keyName, constructTomogMatrix(validIndexes, D) });
			}

			tomogMatrix = tomogMatrices.at(keyName);

			L.push_back(Lcol);

			vector<vector<float>> normalMatrix;

			matrixDot(tomogMatrix, L, normalMatrix);

			assert(normalMatrix.size() == 1 && normalMatrix[0].size() == 3);

			vector<float> normalVector = normalMatrix[0];

			float normalLength = sqrt(normalVector[0] * normalVector[0] + normalVector[1] * normalVector[1] + normalVector[2] * normalVector[2]);

			vector<int> normalPixel = { 128, 128, 128 };

			if (normalLength != 0.0f) {
				for (int i = 0; i != 3; i++) {
					normalPixel[i] = static_cast<uint8_t>(((normalVector[i] / normalLength) - 1.0) / -2.0 * 255.0);
				}
			}

			normal.at<Vec3b>(x, y) = Vec3b(normalPixel[0], normalPixel[1], normalPixel[2]);

			Vec2f xyNormal;

			if (normalLength == 0.0f) {
				normal.at<Vec3b>(x, y) = Vec3b(127, 127, 255);
				xyNormal = Vec2f(0.0f, 0.0f);
			}
			else {
				xyNormal = Vec2f(normalVector[0] / normalLength, normalVector[1] / normalLength);
			}

			vector<float> weights;
			float total = 0;

			for (int k = 0; k != images.size(); k++) {
				if (checkForEmptyInArea(grayImages[k], x, y, 3)) {
					weights.push_back(0.0f);
					continue;
				}
				Vec2f imageVector = Vec2f(D[k][0], D[k][1]);
				float imageVectorLength = sqrt(imageVector[0] * imageVector[0] + imageVector[1] * imageVector[1]);
				imageVector[0] /= imageVectorLength;
				imageVector[1] /= imageVectorLength;

				float comp = abs(xyNormal[0] * imageVector[0] + xyNormal[1] * imageVector[1]);

				weights.push_back(1.0f - comp);
				total += weights[weights.size() - 1];
			}

			Vec3f floatPixel = Vec3f(0.0f, 0.0f, 0.0f);

			for (int k = 0; k != images.size(); k++) {
				Vec3f pixel = static_cast<Vec3f>(images[k]->texMat.at<Vec3b>(x, y));
				floatPixel[0] += pixel[0] * weights[k] / total;
				floatPixel[1] += pixel[1] * weights[k] / total;
				floatPixel[2] += pixel[2] * weights[k] / total;
			}

			diffuse.at<Vec3b>(x, y) = static_cast<Vec3b>(floatPixel);
		}
	}

	cvtColor(normal, normal, COLOR_RGB2BGR);

	return std::vector<Mat>{diffuse, normal};
}

void Tomographer::add_image(string filename, string name) {
	TomogItem* newItem = new TomogItem;
	newItem->name = name;

	Mat image = imread(filename);
	newItem->baseImage = loadList->replacePtr(new imageTexture(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, 1), name);
	newItem->baseImage->getCVMat();

	items.push_back(newItem);
}

void Tomographer::align(int index) {
	TomogItem* item = items[index];
	cv::Mat image = item->baseImage->texMat.clone();

	Mat scaledAlign = alignTemplate.clone();
	Size dims = scaledAlign.size();
	//std::cout << dims << std::endl;
	if (false) {
		int height = 2048;
		dims = Size(height * static_cast<float>(scaledAlign.cols) / static_cast<float>(scaledAlign.rows), height);
		resize(scaledAlign, scaledAlign, Size(height * static_cast<float>(scaledAlign.cols) / static_cast<float>(scaledAlign.rows), height));
	}
	
	match_template(scaledAlign, &image, dims, item->rotation);

	item->correctedImage = loadList->replacePtr(new imageTexture(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, 1), item->name + "Matched");
	item->correctedImage->getCVMat();
}

void Tomographer::remove_element(int index) {
	items.erase(items.begin() + index);
}

void Tomographer::add_lightVector(float phi, float theta, int index) {
	vector<float> lightVec;
	calculateVector(lightVec, phi, theta);
	items[index]->lightDirection = lightVec;
}

void Tomographer::calculate_normal() {
	computedNormal = calculateNormal(items);
	normalExists = true;
}

void Tomographer::calculate_diffuse() {
	if (!normalExists) {
		calculate_normal(); // This will also match the image layouts
	}
	computedDiffuse = calculateDiffuse(items, computedNormal);
}

void Tomographer::calculate_NormAndDiff() {
	std::vector<Mat> results = calculate_norm_diff(items);
	computedDiffuse = results[0];
	computedNormal = results[1];
	normalExists = true;
}