#include"Tomography.h"
#include<cassert>

using namespace cv;
using namespace std;

float eucDist(Point a, Point b) {
	return sqrtf(powf(a.x - b.x, 2) + powf(a.y - b.y, 2));
}

const int MAX_FEATURES = 5000;
const float GOOD_MATCH_PERCENT = 0.1f;

void change_contrast(Mat* img, float alpha, int beta) {
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

void match_template(Mat src, Mat* target, Size outdims) {
	Mat srcGray, targetGray;
	cvtColor(src, srcGray, COLOR_BGR2GRAY);
	cvtColor(*target, targetGray, COLOR_BGR2GRAY);

	resize(targetGray, targetGray, Size(src.rows * target->cols / target->rows, src.rows));
	
	change_contrast(&srcGray, 1.4f, -20);
	change_contrast(&targetGray, 1.4f, -20);

	vector<KeyPoint> keypoints1, keypoints2;
	Mat descriptors1, descriptors2;

	Ptr<Feature2D> orb = ORB::create(MAX_FEATURES);
	orb->detectAndCompute(srcGray, Mat(), keypoints1, descriptors1);
	orb->detectAndCompute(targetGray, Mat(), keypoints2, descriptors2);

	vector<DMatch> matches;
	Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("BruteForce-Hamming");
	matcher->match(descriptors1, descriptors2, matches, Mat());

	sort(matches.begin(), matches.end());

	const int numGoodMatches = matches.size() * GOOD_MATCH_PERCENT;
	matches.erase(matches.begin() + numGoodMatches, matches.end());

	//Mat imMatches;
	//drawMatches(srcGray, keypoints1, targetGray, keypoints2, matches, imMatches);

	//imshow("Matches", imMatches);
	//waitKey(0);

	vector<Point2f> srcPoints, matchPoints;
	for (size_t i = 0; i < matches.size(); i++) {
		srcPoints.push_back(keypoints1[matches[i].queryIdx].pt);
		matchPoints.push_back(keypoints2[matches[i].trainIdx].pt);
	}

	for (size_t i = 0; i < srcPoints.size(); i++) {
		srcPoints[i] = Point2f(srcPoints[i].x * outdims.width / src.cols, srcPoints[i].y * outdims.height / src.rows);
		matchPoints[i] = Point2f(matchPoints[i].x * target->cols / targetGray.cols, matchPoints[i].y * target->rows / targetGray.rows);
	}

	Mat h = findHomography(matchPoints, srcPoints, RANSAC);

	warpPerspective(*target, *target, h, outdims);
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
	// Does not fail
	
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

void printMatrix(vector<vector<float>> matrix) {
	for (int y = 0; y != matrix[0].size(); y++) {
		for (int x = 0; x != matrix.size(); x++) {
			cout << matrix[x][y] << " ";
		}
		cout << endl;
	}
	cout << endl;
}


float matrixDeterminant(vector<vector<float>> matrix) {
	// Does not fail
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
		out.push_back(cofactorCol);
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

Mat calculateNormal(vector<Mat> images, vector<vector<float>> D) { // Calculates the normal texture which describes the surface of the canvas from a set of differently lit images
	// This could be made into a GPU compute operation since it's highly parallel, but I'm not sure if this would actually be faster considering the time cost of copying a vector of (presumably high resolution) images
	// Seems like CPU compute takes a few minutes so worth investigating GPU
	// D represents the list of light vectors for each image
	// Assumes that the painting is a lambertian surface

	assert(images.size() == D.size(), "Input vectors must be the same size");

	// D = images.size() x 3 matrix
	
	vector<vector<float>> DT;
	matrixTranspose(D, DT);
	// DT = 3 x images.size() matrix

	vector<vector<float>> Ddot;
	matrixDot(D, DT, Ddot);
	// Ddot = 3x3 matrix

	printMatrix(Ddot);

	vector<vector<float>> DdotInverse;
	matrixInverse(Ddot, DdotInverse);
	// DdotInverse = 3x3 matrix

	printMatrix(DdotInverse);

	vector<vector<float>> transformationD;
	matrixDot(DT, DdotInverse, transformationD);
	// transformationD = images.size() x 3 matrix

	printMatrix(transformationD);

	matrixTranspose(transformationD, D);

	printMatrix(D);

	Mat normal = images[0].clone();
	normal = Scalar(0, 0, 0);

	vector<Mat> grayImages;
	for (int i = 0; i != images.size(); i++) {
		Mat gray;
		cvtColor(images[i], gray, COLOR_RGB2GRAY);
		grayImages.push_back(gray);
		//imshow("Gray", grayImages[i]);
		//waitKey(0);
	}

	for (int y = 0; y != normal.cols; y++) {
		cout << y << endl;
		for (int x = 0; x != normal.rows; x++) {
			vector<vector<float>> L;
			vector<float> Lcol;
			for (int i = 0; i != images.size(); i++) {
				Lcol.push_back(grayImages[i].at<uint8_t>(x, y));
			}
			L.push_back(Lcol);

			vector<vector<float>> normalMatrix;

			matrixDot(D, L, normalMatrix);

			assert(normalMatrix.size() == 1 && normalMatrix[0].size() == 3);

			vector<float> normalVector = normalMatrix[0];

			float normalLength = sqrt(normalVector[0] * normalVector[0] + normalVector[1] * normalVector[1] + normalVector[2] * normalVector[2]);

			vector<int> normalPixel = { 128, 128, 128 };

			if (normalLength != 0.0f) {
				//cout << normalVector[0] / normalLength << " " << normalVector[1] / normalLength << " " << normalVector[2] / normalLength << endl;
				for (int i = 0; i != 3; i++) {
					normalPixel[i] = static_cast<uint8_t>(((normalVector[i] / normalLength) - 1.0) /-2.0 * 255.0);
				}
			}
			
			//cout << normalPixel[0] << " " << normalPixel[1] << " " << normalPixel[2] << endl;
			normal.at<Vec3b>(x, y) = Vec3b(normalPixel[0], normalPixel[1], normalPixel[2]);
		}
	}

	cvtColor(normal, normal, COLOR_RGB2BGR);

	return normal;
}

void Tomographer::add_image(string filename, float phi, float theta) {
	Mat image = imread(filename);

	vector<float> lightVec;
	calculateVector(lightVec, phi, theta);
	cout << lightVec[0] << " " << lightVec[1] << " " << lightVec[2] << endl;

	images.push_back(image);
	vectors.push_back(lightVec);
}

void Tomographer::calculate_normal() {
	if (alignRequired && (alignTemplate != nullptr)) {
		// perform template matching for each image in the set
		Mat scaledAlign = alignTemplate->clone();
		Size dims(scaledAlign.cols, scaledAlign.rows);
		resize(scaledAlign, scaledAlign, Size(512 * static_cast<float>(scaledAlign.cols) / static_cast<float>(scaledAlign.rows), 512));

		for (int i = 0; i != images.size(); i++) {
			match_template(scaledAlign, &images[i], dims);
		}

	}
	computedNormal = calculateNormal(images, vectors);

	imshow("Calculated normal", computedNormal);
	waitKey(0);
}