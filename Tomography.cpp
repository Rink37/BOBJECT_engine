#include"Tomography.h"
#include<cassert>

using namespace cv;
using namespace std;

float eucDist(Point a, Point b) {
	return sqrtf(powf(a.x - b.x, 2) + powf(a.y - b.y, 2));
}

const int MAX_FEATURES = 5000;
const float GOOD_MATCH_PERCENT = 0.5f;

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

vector<float> calculateCDF(Mat hist) {
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

vector<uint8_t> calculateLUT(vector<float> refCDF, vector<float>srcCDF) {
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

Mat getDiffuseGray(Mat img) {

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

	cout << "Image matched" << endl;

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

void match_partial(Mat src, Mat* target, Size outdims) {

	int defaultHeight = src.rows;
	int defaultWidth = src.cols;
	int stepsPerIter = 8;

	bool vertical = (target->rows >= target->cols);

	int index = 0;
	int secondIndex = 0;
	float correlation = 0.0f;
	float secondCorrelation = 0.0f;

	Mat shrunkTarget;
	if (!vertical) {
		resize(*target, shrunkTarget, Size(defaultWidth, defaultWidth * static_cast<float>(target->rows) / static_cast<float>(target->cols)));
	}
	else {
		resize(*target, shrunkTarget, Size(defaultHeight * static_cast<float>(target->cols) / static_cast<float>(target->rows), defaultHeight));
	}

	Point matchedLoc;

	for (int i = 0; i != stepsPerIter; i++) {
		

		Mat downscaled;
		if (!vertical) {
			int iterWidth = defaultWidth * static_cast<float>(i + 1) / static_cast<float>(stepsPerIter);
			resize(shrunkTarget, downscaled, Size(iterWidth, iterWidth * static_cast<float>(shrunkTarget.rows) / static_cast<float>(shrunkTarget.cols)));
		}
		else {
			int iterHeight = defaultHeight * static_cast<float>(i + 1) / static_cast<float>(stepsPerIter);
			resize(shrunkTarget, downscaled, Size(iterHeight * static_cast<float>(shrunkTarget.cols) / static_cast<float>(shrunkTarget.rows), iterHeight));
		}
		
		Mat res;
		int result_cols = src.cols - downscaled.cols + 1;
		int result_rows = src.rows - downscaled.rows + 1;

		res.create(result_rows, result_cols, CV_32FC1);

		matchTemplate(src, downscaled, res, TM_CCORR_NORMED);

		double min, max;
		Point minLoc, maxLoc;
		minMaxLoc(res, &min, &max, &minLoc, &maxLoc);

		float maxCorr = max;
		
		if (maxCorr > correlation) {
			matchedLoc = maxLoc;
			index = i;
			correlation = maxCorr;
		}
	}

	float scaleFactor;
	if (vertical) {
		scaleFactor = static_cast<float>(stepsPerIter) / static_cast<float>(index + 1) * target->rows / src.rows;
	}
	else {
		scaleFactor = static_cast<float>(stepsPerIter) / static_cast<float>(index + 1) * target->cols / src.cols;
	}

	Mat matched = src.clone();

	resize(matched, matched, Size(matched.cols * scaleFactor, matched.rows * scaleFactor));

	matched = Scalar(0, 0, 0, 0);

	target->copyTo(matched.colRange(matchedLoc.x * scaleFactor, matchedLoc.x * scaleFactor + target->cols).rowRange(matchedLoc.y * scaleFactor, matchedLoc.y * scaleFactor + target->rows));

	*target = matched.clone();
}

void match_template(Mat src, Mat* target, Size outdims) {
	Mat srcGray, targetGray;
	Mat srcChannels[3], targetChannels[3];

	split(src, srcChannels);
	split(*target, targetChannels);

	vector<Point2f> srcPoints, matchPoints;
	
	cvtColor(src, srcGray, COLOR_BGR2GRAY);
	cvtColor(*target, targetGray, COLOR_BGR2GRAY);

	resize(targetGray, targetGray, Size(src.rows * target->cols / target->rows, src.rows));

	normalize(srcGray, srcGray, 0, 255, NORM_MINMAX);
	normalize(targetGray, targetGray, 0, 255, NORM_MINMAX);

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

	Mat imMatches;
	drawMatches(srcGray, keypoints1, targetGray, keypoints2, matches, imMatches);


	for (size_t i = 0; i < matches.size(); i++) {
		if (norm(keypoints1[matches[i].queryIdx].pt - keypoints2[matches[i].trainIdx].pt) < 100) {
			srcPoints.push_back(keypoints1[matches[i].queryIdx].pt);
			matchPoints.push_back(keypoints2[matches[i].trainIdx].pt);
		}
	}

	for (size_t i = 0; i < srcPoints.size(); i++) {
		srcPoints[i] = Point2f(srcPoints[i].x * outdims.width / src.cols, srcPoints[i].y * outdims.height / src.rows);
		matchPoints[i] = Point2f(matchPoints[i].x * target->cols / targetGray.cols, matchPoints[i].y * target->rows / targetGray.rows);
	}

	Mat h = estimateAffine2D(matchPoints, srcPoints);

	warpAffine(*target, *target, h, outdims);
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

vector<vector<float>> constructTomogMatrix(vector<int> indexes, vector<vector<float>> D) {
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

	//printMatrix(Ddot);

	vector<vector<float>> DdotInverse;
	matrixInverse(Ddot, DdotInverse);
	// DdotInverse = 3x3 matrix

	//printMatrix(DdotInverse);

	vector<vector<float>> transformationD;
	matrixDot(DT, DdotInverse, transformationD);
	// transformationD = reducedD.size() x 3 matrix

	vector<vector<float>> tomogMatrix;
	matrixTranspose(transformationD, tomogMatrix);

	return tomogMatrix;
}

bool checkForEmptyInArea(Mat img, int x, int y, int range) {
	for (int dx = x - range; dx != x + range + 1; dx++) {
		for (int dy = y - range; dy != y + range + 1; dy++) {
			if (dy < 0 || dy >= img.rows) {
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

Mat calculateNormal(vector<Texture*> images, vector<vector<float>> D) { // Calculates the normal texture which describes the surface of the canvas from a set of differently lit images
	// This could be made into a GPU compute operation since it's highly parallel, but I'm not sure if this would actually be faster considering the time cost of copying a vector of (presumably high resolution) images
	// Seems like CPU compute takes a few minutes so worth investigating GPU
	// D represents the list of light vectors for each image
	// Assumes that the painting is a lambertian surface

	assert(images.size() == D.size(), "Input vectors must be the same size");

	vector<vector<float>> tomogMatrix;

	Mat normal = images[0]->texMat.clone();
	normal = Scalar(0, 0, 0);

	map <string, vector<vector<float>>> tomogMatrices;

	vector<Mat> grayImages;

	for (int i = 0; i != images.size(); i++) {
		Mat gray;// = getDiffuseGray(images[i]);
		cvtColor(images[i]->texMat, gray, COLOR_RGB2GRAY); // These are fine
		grayImages.push_back(gray);
	}

	//for (int i = 0; i != images.size(); i++) {
	//	std::cout << D[i][0] << " " << D[i][1] << " " << D[i][2] << std::endl;
	//}

	for (int y = 0; y != normal.cols; y++) {
		cout << y << endl;
		for (int x = 0; x != normal.rows; x++) {
			vector<vector<float>> L;
			vector<float> Lcol;
			vector<int> validIndexes;
			string keyName = "";
			for (int i = 0; i != images.size(); i++) {
				if (!checkForEmptyInArea(grayImages[i], x, y, 2)) {
					validIndexes.push_back(i);
					Lcol.push_back(grayImages[i].at<uint8_t>(x, y));
					keyName += to_string(i);
				}
			}
			if (validIndexes.size() == 0) {
				normal.at<Vec3b>(x, y) = Vec3b(127, 127, 255);
				//cout << "No valid indexes" << endl;
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
					normalPixel[i] = static_cast<uint8_t>(((normalVector[i] / normalLength) - 1.0) /-2.0 * 255.0);
				}
			}
			
			normal.at<Vec3b>(x, y) = Vec3b(normalPixel[0], normalPixel[1], normalPixel[2]);

			//std::cout << normalPixel[0] << " " << normalPixel[1] << " " << normalPixel[2] << std::endl;

		}
	}

	cvtColor(normal, normal, COLOR_RGB2BGR);

	return normal;
}

Mat calculateDiffuse(vector<Texture*> images, vector<vector<float>> D, Mat normal) {

	Mat diffuse = images[0]->texMat.clone();
	diffuse = Scalar(0, 0, 0);

	vector<Mat> grayImages;
	for (int k = 0; k != images.size(); k++) {
		//Mat grayImg;
		Mat grayImg;// = getDiffuseGray(images[k]);
		cvtColor(images[k]->texMat, grayImg, COLOR_RGB2GRAY);
		grayImages.push_back(grayImg);
	}

	for (int y = 0; y != diffuse.cols; y++) {
		cout << y << endl;
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
				if (checkForEmptyInArea(grayImages[k], x, y, 2)) {
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

void Tomographer::add_image(string filename) {
	Mat image = imread(filename);
	Texture* texture = new imageTexture(image);
	texture->getCVMat();

	originalImages.push_back(texture);

	Mat scaledAlign = alignTemplate->clone();
	Size dims(scaledAlign.cols, scaledAlign.rows);
	int height = 1024;
	resize(scaledAlign, scaledAlign, Size(height * static_cast<float>(scaledAlign.cols) / static_cast<float>(scaledAlign.rows), height));

	match_partial(scaledAlign, &image, dims);
	match_template(scaledAlign, &image, dims);

	Texture* matchedTex = new imageTexture(image);
	matchedTex->getCVMat();

	images.push_back(matchedTex);
}

void Tomographer::add_lightVector(float phi, float theta) {
	vector<float> lightVec;
	calculateVector(lightVec, phi, theta);
	vectors.push_back(lightVec);
}

void Tomographer::calculate_normal() {
	//if (alignRequired && (alignTemplate != nullptr)) {
		// perform template matching for each image in the set
	//	Mat scaledAlign = alignTemplate->clone();
	//	Size dims(scaledAlign.cols, scaledAlign.rows);
	//	int height = 1024;
	//	resize(scaledAlign, scaledAlign, Size(height * static_cast<float>(scaledAlign.cols) / static_cast<float>(scaledAlign.rows), height));

	//	for (int i = 0; i != images.size(); i++) {
	//		match_partial(scaledAlign, &images[i], dims);
	//		match_template(scaledAlign, &images[i], dims);
	//	}
		//waitKey(0);
	//}
	computedNormal = calculateNormal(images, vectors);
	normalExists = true;

	imshow("Calculated normal", computedNormal);
	waitKey(0);
}

void Tomographer::calculate_diffuse() {
	if (!normalExists) {
		calculate_normal(); // This will also match the image layouts
	}
	computedDiffuse = calculateDiffuse(images, vectors, computedNormal);

	imshow("Calculated diffuse", computedDiffuse);
	waitKey(0);
}