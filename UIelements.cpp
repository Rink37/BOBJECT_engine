#include "UIelements.h"

using namespace std;

void UIImage::UpdateVertices(float xp, float yp, float xsc, float ysc) {
	mesh.UpdateVertices(xp, yp, xsc, ysc);
}

void bufferPosition(float &extentx, float &extenty, float &posx, float &posy, float brx, float bry) {
	
	float minx, maxx;
	minx = posx - extentx;
	maxx = posx + extentx;
	// x coordinates are described by -1 -> 1 = left to right

	if (minx < -1 + brx) {
		posx = -1 + brx + extentx;
	}
	else if (maxx > 1 - brx) {
		posx = 1 - brx - extentx;
	}

	float miny, maxy;
	miny = posy - extenty;
	maxy = posy + extenty;
	// y coordinates are described by -1 -> 1 = top to bottom

	if (miny < -1 + bry) {
		posy = -1 + bry + extenty;
	}
	else if (maxy > 1 - bry) {
		posy = 1 - bry - extenty;
	}
}

void UIItem::calculateScreenPosition() {
	float W = static_cast<float>(Engine::get()->windowWidth);
	float H = static_cast<float>(Engine::get()->windowHeight);

	extenty = extentx * sqAxisRatio * W / H;

	float bufferRatioX, bufferRatioY;

	bufferRatioX = static_cast<float>(buffer) / (2 * W);
	bufferRatioY = static_cast<float>(buffer) / (2 * H);

	bufferPosition(extentx, extenty, posx, posy, bufferRatioX, bufferRatioY);

	this->windowPositions[0] = (((posx - extentx) / 2.0f) + 0.5f) * W; // left position
	this->windowPositions[1] = (((posx + extentx) / 2.0f) + 0.5f) * W; // right position
	this->windowPositions[2] = (((posy - extenty) / 2.0f) + 0.5f) * H; // top position
	this->windowPositions[3] = (((posy + extenty) / 2.0f) + 0.5f) * H; // bottom position

}

void UIItem::addItem(UIItem *item) {
	Items.push_back(item);
}

void Arrangement::calculateScreenPosition() {
	float W = static_cast<float>(Engine::get()->windowWidth);
	float H = static_cast<float>(Engine::get()->windowHeight);

	float bufferRatioX, bufferRatioY;

	bufferRatioX = static_cast<float>(buffer) / (2 * W);
	bufferRatioY = static_cast<float>(buffer) / (2 * H);

	bufferPosition(extentx, extenty, posx, posy, bufferRatioX, bufferRatioY);

	this->windowPositions[0] = (((posx - extentx) / 2.0f) + 0.5f) * W;
	this->windowPositions[1] = (((posx + extentx) / 2.0f) + 0.5f) * W;
	this->windowPositions[2] = (((posy - extenty) / 2.0f) + 0.5f) * H;
	this->windowPositions[3] = (((posy + extenty) / 2.0f) + 0.5f) * H;
}

void Arrangement::updateDisplay() {
	arrangeItems();
	for (size_t i = 0; i != Items.size(); i++) {
		Items[i]->updateDisplay();
	}
}

void Arrangement::getItemProperties(float& totalArea, int& numSpacers, float& buffer, std::vector<float>& extents) {
	float W = static_cast<float>(Engine::get()->windowWidth);
	float H = static_cast<float>(Engine::get()->windowHeight);

	for (size_t i = 0; i != Items.size(); i++) {
		if (Items[i]->isSpacer()) {
			numSpacers++;
		}
	}

	if (orientation == ORIENT_HORIZONTAL) {
		buffer = this->extentx * 2 * this->spacing; // Each item will have half of this value of empty space on each side
	}
	else if (orientation == ORIENT_VERTICAL) {
		buffer = this->extenty * 2 * this->spacing;
	}

	// We scale all items to have a height equal to the height of this arrangement, then calculate the width of all items summed up

	float rescale = 0;
	float maxSize = 0;

	switch (sizing) {
	case SCALE_BY_DIMENSIONS:
		if (orientation == ORIENT_HORIZONTAL) {
			for (size_t i = 0; i != Items.size(); i++) {
				if (!Items[i]->isSpacer()) {
					if (Items[i]->extenty * 2 > maxSize) {
						maxSize = Items[i]->extenty * 2;
					}
				}
			}
			rescale = (this->extenty * 2 - this->spacing) / maxSize;
			for (size_t i = 0; i != Items.size(); i++) {
				if (!Items[i]->isSpacer()) {
					if (!Items[i]->isArrangement() || numSpacers != 0) {
						extents.push_back((Items[i]->extenty * rescale / (Items[i]->sqAxisRatio * W / H)));
					}
					else {
						extents.push_back(Items[i]->extentx * rescale);
					}

					totalArea += extents[i] * 2;
				}
				else {
					extents.push_back(0.0f);
				}
			}
		}
		else if (orientation == ORIENT_VERTICAL) {
			for (size_t i = 0; i != Items.size(); i++) {
				if (!Items[i]->isSpacer()) {
					if (Items[i]->extentx * 2 > maxSize) {
						maxSize = Items[i]->extentx * 2;
					}
				}
			}
			rescale = (this->extentx * 2 - this->spacing) / maxSize;
			for (size_t i = 0; i != Items.size(); i++) {
				if (!Items[i]->isSpacer()) {
					if (!Items[i]->isArrangement() || numSpacers != 0) {
						extents.push_back(Items[i]->extentx * rescale * Items[i]->sqAxisRatio);
						totalArea += extents[i] * 2 * W / H;
					}
					else {
						extents.push_back(Items[i]->extenty * rescale);
						totalArea += extents[i] * 2;
					}
				}
				else {
					extents.push_back(0.0f);
				}
			}
		}
		break;
	default:
		if (orientation == ORIENT_HORIZONTAL) {
			for (size_t i = 0; i != Items.size(); i++) {
				if (!Items[i]->isSpacer()) {
					if (!Items[i]->isArrangement() || numSpacers != 0) {
						extents.push_back((this->extenty * (1.0f - this->spacing) / (Items[i]->sqAxisRatio * W / H)));
					}
					else {
						extents.push_back(Items[i]->extentx * (1.0f - this->spacing));
					}
					totalArea += extents[i] * 2;
				}
				else {
					extents.push_back(0.0f);
				}
			}
		}
		else if (orientation == ORIENT_VERTICAL) {
			for (size_t i = 0; i != Items.size(); i++) {
				if (!Items[i]->isSpacer()) {
					if (!Items[i]->isArrangement() || numSpacers != 0) {
						extents.push_back(this->extentx * (1.0f - this->spacing) * Items[i]->sqAxisRatio);
						totalArea += extents[i] * 2 * W / H;
					}
					else {
						extents.push_back(Items[i]->extenty * (1.0f - this->spacing));
						totalArea += extents[i] * 2;
					}

				}
				else {
					extents.push_back(0.0f);
				}
			}
		}
		break;
	}
}

void Arrangement::arrangeItems() {
	if (orientation != ORIENT_VERTICAL && orientation != ORIENT_HORIZONTAL) {
		cout << "Invalid orientation" << endl;
		return;
	}

	this->calculateScreenPosition();
	
	float totalArea = 0;
	int numSpacers = 0;
	float buffer = 0.0f;
	vector<float> extents = {};

	getItemProperties(totalArea, numSpacers, buffer, extents);

	float bufferSpace = buffer * Items.size(); // Total space occupied by x buffers
	float spacerSize = 0.0f;

	// Now we find the constant scale factor required to fit all items into the same arrangement horizontally 

	float scaleFactor;

	if (orientation == ORIENT_HORIZONTAL) {
		scaleFactor = (this->extentx * 2 - bufferSpace) / (totalArea);
		calculateHSpacing(scaleFactor, numSpacers, spacerSize, totalArea, buffer);

		float remainingWidth = this->extentx * 2 - totalArea * scaleFactor - bufferSpace;

		// Finally for all items we calculate their positions on the screen and their sizes 

		calculateHPositions(buffer, spacerSize, scaleFactor, extents, remainingWidth);

	}
	else if (orientation == ORIENT_VERTICAL) {
		scaleFactor = (this->extenty * 2 - bufferSpace) / (totalArea);
		calculateVSpacing(scaleFactor, numSpacers, spacerSize, totalArea, buffer);

		float remainingHeight = this->extenty * 2 - totalArea * scaleFactor - bufferSpace;

		// Finally for all items we calculate their positions on the screen and their sizes 

		float W = static_cast<float>(Engine::get()->windowWidth);
		float H = static_cast<float>(Engine::get()->windowHeight);

		calculateVPositions(buffer, spacerSize, scaleFactor, extents, remainingHeight, W / H);
	}

}

void Arrangement::calculateVSpacing(float& scaleFactor, int numSpacers, float& spacerSize, float& totalHeight, float& ybuffer) {
	if (scaleFactor > 1.0f) {
		// This means that the total width of the items is smaller than the free space
		scaleFactor = 1.0f;
		if (numSpacers > 0) {
			spacerSize = (this->extenty * 2 - (totalHeight + ybuffer * Items.size())) / numSpacers;
		}
		else if (this->method == ARRANGE_FILL) {
			ybuffer = (this->extenty * 2 - totalHeight) / Items.size();
		}
	}
}

void Arrangement::calculateVPositions(float ybuffer, float spacerSize, float scaleFactor, vector<float> extents, float remainingHeight, float vScale) {
	float xp, yp, xsc, ysc;

	float currentPosition = 0;

	switch (this->method) {
	case (ARRANGE_CENTER):
		currentPosition += remainingHeight / 2;
		break;
	case (ARRANGE_END):
		currentPosition += remainingHeight;
		break;
	default:
		break;
	}

	for (size_t i = 0; i != Items.size(); i++) {
		currentPosition += ybuffer / 2;
		if (Items[i]->isSpacer()) {
			currentPosition += spacerSize;
		}
		else {
			ysc = extents[i] * scaleFactor;
			xsc = ysc / Items[i]->sqAxisRatio;
			xp = this->posx;
			if (!Items[i]->isArrangement()) {
				yp = this->posy - this->extenty + currentPosition + ysc * vScale; // vScale = W/H
			}
			else {
				yp = this->posy - this->extenty + currentPosition + ysc;
				xsc = this->extentx * (1.0f - this->spacing);
			}
			

			Items[i]->update(xp, yp, xsc, ysc);
			Items[i]->updateDisplay();
			Items[i]->arrangeItems();

			if (!Items[i]->isArrangement()) {
				currentPosition += ysc * 2 * vScale;
			}
			else {
				currentPosition += ysc * 2;
			}
			
		}
		currentPosition += ybuffer / 2;
	}
}

void Arrangement::calculateHSpacing(float& scaleFactor, int numSpacers, float& spacerSize, float& totalWidth, float& xbuffer) {
	if (scaleFactor > 1.0f) {
		// This means that the total width of the items is smaller than the free space
		scaleFactor = 1.0f;
		if (numSpacers > 0) {
			spacerSize = (this->extentx * 2 - (totalWidth + xbuffer * Items.size())) / numSpacers;
		}
		else if (this->method == ARRANGE_FILL) {
			xbuffer = (this->extentx * 2 - totalWidth) / Items.size();
		}
	}
}

void Arrangement::calculateHPositions(float xbuffer, float spacerSize, float scaleFactor, vector<float> extents, float remainingWidth) {
	float xp, yp, xsc, ysc;

	float currentPosition = 0;

	switch (this->method) {
	case (ARRANGE_CENTER):
		currentPosition += remainingWidth / 2;
		break;
	case (ARRANGE_END):
		currentPosition += remainingWidth;
		break;
	default:
		currentPosition = 0;
		break;
	}

	for (size_t i = 0; i != Items.size(); i++) {
		currentPosition += xbuffer / 2;
		if (Items[i]->isSpacer()) {
			currentPosition += spacerSize;
		}
		else {
			xsc = extents[i] * scaleFactor;
			ysc = xsc * Items[i]->sqAxisRatio;
			yp = this->posy;
			xp = this->posx - this->extentx + currentPosition + xsc;
			if (Items[i]->isArrangement()) {
				ysc = this->extenty * (1.0f - this->spacing);
			}

			Items[i]->update(xp, yp, xsc, ysc);
			Items[i]->updateDisplay();
			Items[i]->arrangeItems();

			currentPosition += xsc * 2;
		}
		currentPosition += xbuffer / 2;
	}
}

bool Arrangement::checkForSpace(UIItem* checkItem) {
	if (orientation != ORIENT_VERTICAL && orientation != ORIENT_HORIZONTAL) {
		cout << "Invalid orientation" << endl;
		return false;
	}
	if (checkItem->isSpacer()) {
		return true;
	}

	this->calculateScreenPosition();
	float totalArea = 0;
	int numSpacers = 0;
	float buffer = 0.0f;
	vector<float> extents = {};

	Items.push_back(checkItem);

	getItemProperties(totalArea, numSpacers, buffer, extents);

	Items.erase(Items.end() - 1);

	float bufferSpace = buffer * Items.size(); // Total space occupied by x buffers
	float spacerSize = 0.0f;

	// Now we find the constant scale factor required to fit all items into the same arrangement horizontally 

	float scaleFactor;

	if (orientation == ORIENT_HORIZONTAL) {
		scaleFactor = (this->extentx * 2 - bufferSpace) / (totalArea);
	}
	else if (orientation == ORIENT_VERTICAL) {
		scaleFactor = (this->extenty * 2 - bufferSpace) / (totalArea);
	}

	if (scaleFactor > 1.0f) {
		return true;
	}
	else {
		return false;
	}
}

void Grid::calculateScreenPosition() {
	float W = static_cast<float>(Engine::get()->windowWidth);
	float H = static_cast<float>(Engine::get()->windowHeight);

	float bufferRatioX, bufferRatioY;

	bufferRatioX = static_cast<float>(buffer) / (2 * W);
	bufferRatioY = static_cast<float>(buffer) / (2 * H);

	bufferPosition(extentx, extenty, posx, posy, bufferRatioX, bufferRatioY);

	this->windowPositions[0] = (((posx - extentx) / 2.0f) + 0.5f) * W;
	this->windowPositions[1] = (((posx + extentx) / 2.0f) + 0.5f) * W;
	this->windowPositions[2] = (((posy - extenty) / 2.0f) + 0.5f) * H;
	this->windowPositions[3] = (((posy + extenty) / 2.0f) + 0.5f) * H;

	arrangeItems();
	mainArrangement->calculateScreenPosition();
}

void Grid::updateDisplay() {
	arrangeItems();
	mainArrangement->updateDisplay();
}

void Grid::arrangeItems() {
	delete mainArrangement;

	float subExtentx = 0.0f;
	float subExtenty = 0.0f;

	float W = static_cast<float>(Engine::get()->windowWidth);
	float H = static_cast<float>(Engine::get()->windowHeight);

	if (orientation == ORIENT_HORIZONTAL) {
		mainArrangement = new Arrangement(ORIENT_VERTICAL, this->posx, -1*this->posy, this->extentx, this->extenty, this->spacing, ARRANGE_START);
		subExtentx = this->extentx;
		subExtenty = this->extentx / (float(numArrangements) * 1.1f) * this->extenty / this->extentx;
	}
	else if (orientation == ORIENT_VERTICAL) {
		mainArrangement = new Arrangement(ORIENT_HORIZONTAL, this->posx, -1*this->posy, this->extentx, this->extenty, this->spacing, ARRANGE_FILL);
		subExtenty = this->extenty;
		subExtentx = this->extenty / (float(numArrangements) * 1.1f) * this->extentx/this->extenty;
	}

	Arrangement* subArrangement = new Arrangement(orientation, 0.0f, 0.0f, subExtentx, subExtenty, this->spacing, ARRANGE_START);

	int index = 1;

	for (size_t i = 0; i != Items.size(); i++) {
		Items[i]->update(0.0f, 0.0f, 1.0f, 1.0f);
		if (subArrangement->checkForSpace(Items[i])) {
			subArrangement->addItem(Items[i]);
		}
		else {
			subArrangement->setArrangeMethod(ARRANGE_FILL);
			mainArrangement->addItem(subArrangement);
			subArrangement = new Arrangement(orientation, 0.0f, 0.0f, subExtentx, subExtenty, this->spacing, ARRANGE_START);
			index++;
			subArrangement->addItem(Items[i]);
		}
	}

	mainArrangement->addItem(subArrangement);
	mainArrangement->arrangeItems();

	if (index > numArrangements) {
		numArrangements = index;
		arrangeItems();
	}
}

void Slider::calculateScreenPosition() {
	float W = static_cast<float>(Engine::get()->windowWidth);
	float H = static_cast<float>(Engine::get()->windowHeight);

	float bufferRatioX, bufferRatioY;

	bufferRatioX = static_cast<float>(buffer) / (2 * W);
	bufferRatioY = static_cast<float>(buffer) / (2 * H);

	bufferPosition(extentx, extenty, posx, posy, bufferRatioX, bufferRatioY);

	switch (orientation) {
	case (ORIENT_HORIZONTAL):
		this->windowPositions[0] = ((((posx - extentx) + (2 * extentx * slideValue) - sliderWidth) / 2.0f) + 0.5f) * W; // left position
		this->windowPositions[1] = ((((posx - extentx) + (2 * extentx * slideValue) + sliderWidth) / 2.0f) + 0.5f) * W; // right position
		this->windowPositions[2] = (((posy - extenty) / 2.0f) + 0.5f) * H; // top position
		this->windowPositions[3] = (((posy + extenty) / 2.0f) + 0.5f) * H; // bottom position

		this->valuePositions[0] = (((posx - extentx) / 2.0f) + 0.5f) * W; // pixel x value at min position
		this->valuePositions[1] = (((posx + extentx) / 2.0f) + 0.5f) * W; // pixel x value at max position

		break;
	case (ORIENT_VERTICAL):
		this->windowPositions[0] = (((posx - extentx) / 2.0f) + 0.5f) * W; // left position
		this->windowPositions[1] = (((posx + extentx) / 2.0f) + 0.5f) * W; // right position
		this->windowPositions[2] = ((((posy + extenty) - (2 * extenty * slideValue) - sliderWidth) / 2.0f) + 0.5f) * H; // top position
		this->windowPositions[3] = ((((posy + extenty) - (2 * extenty * slideValue) + sliderWidth) / 2.0f) + 0.5f) * H; // bottom position

		this->valuePositions[0] = (((posy + extenty) / 2.0f) + 0.5f) * H; // pixel y value at min position
		this->valuePositions[1] = (((posy - extenty) / 2.0f) + 0.5f) * H; // pixel y value at max position

		break;

	default:
		this->windowPositions[0] = ((((posx - extentx) + (2 * extentx * slideValue) - sliderWidth) / 2.0f) + 0.5f) * W; // left position
		this->windowPositions[1] = ((((posx - extentx) + (2 * extentx * slideValue) + sliderWidth) / 2.0f) + 0.5f) * W; // right position
		this->windowPositions[2] = (((posy - extenty) / 2.0f) + 0.5f) * H; // top position
		this->windowPositions[3] = (((posy + extenty) / 2.0f) + 0.5f) * H; // bottom position

		this->valuePositions[0] = (((posx - extentx) / 2.0f) + 0.5f) * W; // pixel x value at min position
		this->valuePositions[1] = (((posx + extentx) / 2.0f) + 0.5f) * W; // pixel x value at max position

		break;
	}
}

void Slider::calculateSlideValue(double mouseX, double mouseY) {
	switch (orientation) {
	case (ORIENT_HORIZONTAL):
		slideValue = clamp(float((mouseX - valuePositions[0]) / (valuePositions[1] - valuePositions[0])), 0.0f, 1.0f);
		break;
	case (ORIENT_VERTICAL):
		slideValue = clamp(float((mouseY - valuePositions[0]) / (valuePositions[1] - valuePositions[0])), 0.0f, 1.0f);
		break;
	default:
		slideValue = clamp(float((mouseX - valuePositions[0]) / (valuePositions[1] - valuePositions[0])), 0.0f, 1.0f);
		break;
	}
	if (valueType == SLIDER_DISCRETE) {
		slideValue *= (maxValue - minValue);
		slideValue += minValue;

		slideValue = (static_cast<float>(round(slideValue)) - minValue)/(maxValue-minValue);
	}
}

void Rotator::calculateScreenPosition() {
	W = static_cast<float>(Engine::get()->windowWidth);
	H = static_cast<float>(Engine::get()->windowHeight);

	float bufferRatioX, bufferRatioY;

	bufferRatioX = static_cast<float>(buffer) / (2 * W);
	bufferRatioY = static_cast<float>(buffer) / (2 * H);

	bufferPosition(extentx, extenty, posx, posy, bufferRatioX, bufferRatioY);

	float theta = OPF_PI - (2 * PI * slideValue);

	float x = radius * cos(theta) + this->posx;
	float y = radius * sin(theta) + this->posy;

	this->windowPositions[0] = (((x - sliderWidth) / 2.0f) + 0.5f) * W; // left position
	this->windowPositions[1] = (((x + sliderWidth) / 2.0f) + 0.5f) * W; // right position
	this->windowPositions[2] = (((y - sliderWidth) / 2.0f) + 0.5f) * H; // top position
	this->windowPositions[3] = (((y + sliderWidth) / 2.0f) + 0.5f) * H; // bottom position

	this->centroid[0] = (((this->posx) / 2.0f) + 0.5f) * W;
	this->centroid[1] = (((this->posy) / 2.0f) + 0.5f) * H;
}

void Rotator::calculateSlideValue(double mouseX, double mouseY) {
	float x = mouseX - centroid[0];
	float y = mouseY - centroid[1];

	float theta = atan2(y, x);
	float angleFromVert = OPF_PI - theta; // We want to increase clockwise from vertical

	slideValue = angleFromVert / (2 * PI);

	slideValue = (slideValue > 1) ? slideValue - 1 : slideValue;
	slideValue = (slideValue < 0) ? slideValue + 1 : slideValue;

	if (valueType == SLIDER_DISCRETE) {
		slideValue *= (maxValue - minValue);
		slideValue += minValue;

		slideValue = (static_cast<float>(round(slideValue)) - minValue) / (maxValue - minValue);
	}
}