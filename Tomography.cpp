#include"Tomography.h"
#include<cassert>

using namespace cv;
using namespace std;

float eucDist(Point a, Point b) {
	return sqrtf(powf(a.x - b.x, 2) + powf(a.y - b.y, 2));
}

void match_template(Mat src, Mat &target) {

	// Untested 

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

	warpPerspective(target, target, M, Size(src.rows, src.cols));
}

void calculateVector(vector<float>& lightVec, float phi, float theta) {
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

void matrixTranspose(vector<vector<float>> src, vector<vector<float>>& out) {
	// switches the rows and columns of the src matrix
	
	int width = src.size();
	int height = src[0].size();

	out.clear();

	for (int y = 0; y != height; y++) {
		vector<float> col;
		for (int x = 0; x != width; x++) {
			col.push_back(src[x][y]);
		}
		out.push_back(col);
	}
}

void matrixDot(vector<vector<float>> a, vector<vector<float>> b, vector<vector<float>>& out) {
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

float matrixDeterminant(vector<vector<float>> matrix) {
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
}

void calculateCofactor(vector<vector<float>> matrix, vector<vector<float>>& out) {
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
	}
}

void matrixInverse(vector<vector<float>> matrix, vector<vector<float>>& inverse) { // compute the inverse of a square matrix
	vector<vector<float>> cofactorMatrix;
	vector<vector<float>> cofactorTranspose;
	calculateCofactor(matrix, cofactorMatrix);
	matrixTranspose(cofactorMatrix, cofactorTranspose);

	float determinant = matrixDeterminant(matrix);

	for (int x = 0; x != matrix.size(); x++) {
		vector<float> col;
		for (int y = 0; y != matrix[0].size(); y++) {
			col.push_back(cofactorTranspose[x][y] * 1.0 / determinant);
		}
		inverse.push_back(col);
	}
}

void calculateNormal(vector<Mat> images, vector<vector<float>> D, Mat *normal) { // Calculates the normal texture which describes the surface of the canvas from a set of differently lit images
	// This could be made into a GPU compute operation since it's highly parallel, but I'm not sure if this would actually be faster considering the time cost of copying a vector of (presumably high resolution) images
	// D represents the list of light vectors for each image
	// Assumes that the painting is a lambertian surface

	assert(images.size() == D.size(), "Input vectors must be the same size");

	// D = images.size() x 3 matrix
	
	vector<vector<float>> DT;
	matrixTranspose(D, DT);
	// DT = 3 x images.size() matrix

	vector<vector<float>> Ddot;
	matrixDot(DT, D, Ddot);
	// DT = 3x3 matrix

	vector<vector<float>> DdotInverse;
	matrixInverse(Ddot, DdotInverse);
	// DdotInverse = 3x3 matrix

	vector<vector<float>> transformationD;
	matrixDot(DdotInverse, D, transformationD);
	// transformationD = images.size() x 3 matrix

	*normal = images[0].clone();
	*normal = Scalar(128, 128, 128);

	vector<Mat> grayImages;
	for (int i = 0; i != images.size(); i++) {
		Mat gray;
		cvtColor(images[i], gray, COLOR_RGB2GRAY);
		grayImages.push_back(gray);
	}

	for (int y = 0; y != images[0].rows; y++) {
		for (int x = 0; x != images[0].cols; x++) {
			vector<vector<float>> L;
			vector<float> Lcol;
			for (int i = 0; i != images.size(); i++) {
				Lcol.push_back(grayImages[i].at<int>(x, y));
			}
			L.push_back(Lcol);
			// L = 1 x images.size() matrix
			// transformationD = images.size() x 3 matrix
			vector<vector<float>> normalMatrix;
			matrixDot(transformationD, L, normalMatrix);

			assert(normalMatrix.size() == 1 && normalMatrix[0].size() == 3);

			vector<float> normalVector = normalMatrix[0];
			float normalLength = sqrt(normalVector[0] * normalVector[0] + normalVector[1] * normalVector[1] + normalVector[2] * normalVector[2]);

			vector<int> normalPixel;
			for (int i = 0; i != 3; i++) {
				normalPixel.push_back(static_cast<int>(-((normalVector[i] / normalLength) - 1.0) / 2.0 * 255.0));
			}
			normal->at<Vec3b>(x, y) = Vec3b(normalPixel[0], normalPixel[1], normalPixel[2]);
		}
	}
}

void Tomographer::add_image(string filename, float phi, float theta) {
	Mat image = imread(filename);
	cvtColor(image, image, COLOR_BGR2RGB);

	vector<float> lightVec;
	calculateVector(lightVec, phi, theta);

	images.push_back(image);
	vectors.push_back(lightVec);
}

void Tomographer::calculate_normal() {
	if (alignRequired && (alignTemplate != nullptr)) {
		// perform template matching for each image in the set
		for (int i = 0; i != images.size(); i++) {
			match_template(*alignTemplate, images[i]);
		}
	}
	calculateNormal(images, vectors, &surfaceNormal->texMat);

	imshow("Calculated normal", surfaceNormal->texMat);
	waitKey(0);
	return;
}