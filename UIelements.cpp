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

void Arrangement::arrangeItems() {
	if (orientation != ORIENT_VERTICAL && orientation != ORIENT_HORIZONTAL) {
		cout << "Invalid orientation" << endl;
		return;
	}

	this->calculateScreenPosition();
	float W = static_cast<float>(Engine::get()->windowWidth);
	float H = static_cast<float>(Engine::get()->windowHeight);

	float totalArea = 0;
	int numSpacers = 0;

	float buffer;
	if (orientation == ORIENT_HORIZONTAL) {
		buffer = this->extentx * 2 * this->spacing; // Each item will have half of this value of empty space on each side
	}
	else if (orientation == ORIENT_VERTICAL) {
		buffer = this->extenty * 2 * this->spacing; 
	}

	// We scale all items to have a height equal to the height of this arrangement, then calculate the width of all items summed up

	vector<float> extents;

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
					extents.push_back((Items[i]->extenty * rescale / (Items[i]->sqAxisRatio * W / H)));
					totalArea += extents[i] * 2;
				}
				else {
					extents.push_back(0.0f);
					numSpacers++;
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
					extents.push_back(Items[i]->extentx * rescale * Items[i]->sqAxisRatio);
					totalArea += extents[i] * 2 * W / H;
				}
				else {
					extents.push_back(0.0f);
					numSpacers++;
				}
			}
		}
		break;
	default:
		if (orientation == ORIENT_HORIZONTAL) {
			for (size_t i = 0; i != Items.size(); i++) {
				if (!Items[i]->isSpacer()) {
					extents.push_back((this->extenty * (1 - this->spacing) / (Items[i]->sqAxisRatio * W / H)));
					totalArea += extents[i] * 2;
				}
				else {
					extents.push_back(0.0f);
					numSpacers++;
				}
			}
		}
		else if (orientation == ORIENT_VERTICAL) {
			for (size_t i = 0; i != Items.size(); i++) {
				if (!Items[i]->isSpacer()) {
					extents.push_back(this->extentx * (1 - this->spacing) * Items[i]->sqAxisRatio);
					totalArea += extents[i] * 2 * W / H;
				}
				else {
					extents.push_back(0.0f);
					numSpacers++;
				}
			}
		}
		break;
	}

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
				yp = this->posy - this->extenty + currentPosition + ysc; // vScale = W/H
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
	float W = static_cast<float>(Engine::get()->windowWidth);
	float H = static_cast<float>(Engine::get()->windowHeight);

	float totalArea = 0;
	int numSpacers = 0;

	float buffer;
	if (orientation == ORIENT_HORIZONTAL) {
		buffer = this->extentx * 2 * this->spacing; // Each item will have half of this value of empty space on each side
	}
	else if (orientation == ORIENT_VERTICAL) {
		buffer = this->extenty * 2 * this->spacing;
	}

	// We scale all items to have a height equal to the height of this arrangement, then calculate the width of all items summed up

	vector<float> extents;

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
			if (checkItem->extenty * 2 > maxSize) {
				maxSize = checkItem->extenty * 2;
			}
			rescale = (this->extenty * 2 - this->spacing) / maxSize;
			for (size_t i = 0; i != Items.size(); i++) {
				if (!Items[i]->isSpacer()) {
					extents.push_back((Items[i]->extenty * rescale / (Items[i]->sqAxisRatio * W / H)));
					totalArea += extents[i] * 2;
				}
				else {
					extents.push_back(0.0f);
					numSpacers++;
				}
			}
			extents.push_back((checkItem->extenty * rescale / (checkItem->sqAxisRatio * W / H)));
			totalArea += extents[Items.size()] * 2;
		}
		else if (orientation == ORIENT_VERTICAL) {
			for (size_t i = 0; i != Items.size(); i++) {
				if (!Items[i]->isSpacer()) {
					if (Items[i]->extentx * 2 > maxSize) {
						maxSize = Items[i]->extentx * 2;
					}
				}
			}
			if (checkItem->extentx * 2 > maxSize) {
				maxSize = checkItem->extentx * 2;
			}
			rescale = (this->extentx * 2 - this->spacing) / maxSize;
			for (size_t i = 0; i != Items.size(); i++) {
				if (!Items[i]->isSpacer()) {
					extents.push_back(Items[i]->extentx * rescale * Items[i]->sqAxisRatio);
					totalArea += extents[i] * 2 * W / H;
				}
				else {
					extents.push_back(0.0f);
					numSpacers++;
				}
			}
			extents.push_back(checkItem->extentx * rescale * checkItem->sqAxisRatio);
			totalArea += extents[Items.size()] * 2 * W / H;
		}
		break;
	default:
		if (orientation == ORIENT_HORIZONTAL) {
			for (size_t i = 0; i != Items.size(); i++) {
				if (!Items[i]->isSpacer()) {
					extents.push_back((this->extenty * (1 - this->spacing) / (Items[i]->sqAxisRatio * W / H)));
					totalArea += extents[i] * 2;
				}
				else {
					extents.push_back(0.0f);
					numSpacers++;
				}
			}
			extents.push_back((this->extenty * (1 - this->spacing) / (checkItem->sqAxisRatio * W / H)));
			totalArea += extents[Items.size()] * 2;
		}
		else if (orientation == ORIENT_VERTICAL) {
			for (size_t i = 0; i != Items.size(); i++) {
				if (!Items[i]->isSpacer()) {
					extents.push_back(this->extentx * (1 - this->spacing) * Items[i]->sqAxisRatio);
					totalArea += extents[i] * 2 * W / H;
				}
				else {
					extents.push_back(0.0f);
					numSpacers++;
				}
			}
			extents.push_back(this->extentx * (1 - this->spacing) * checkItem->sqAxisRatio);
			totalArea += extents[Items.size()] * 2 * W / H;
		}
		break;
	}

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
		mainArrangement = new Arrangement(ORIENT_VERTICAL, this->posx, this->posy, this->extentx, this->extenty, this->spacing, ARRANGE_START, SCALE_BY_DIMENSIONS);
		subExtentx = 1.0f;
		subExtenty = 1.0f / (float(numArrangements) * 1.1f);
	}
	else if (orientation == ORIENT_VERTICAL) {
		mainArrangement = new Arrangement(ORIENT_HORIZONTAL, this->posx, this->posy, this->extentx, this->extenty, this->spacing, ARRANGE_START, SCALE_BY_DIMENSIONS);
		subExtenty = 1.0f;
		subExtentx = 1.0f / (float(numArrangements) * 1.1f);
	}

	Arrangement* subArrangement = new Arrangement(orientation, 0.0f, 0.0f, subExtentx, subExtenty, this->spacing, ARRANGE_START);

	int index = 0;

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

	index -= (numArrangements-3);
	if (index < 0) {
		index = 0;
	}

	mainArrangement->addItem(subArrangement);
	mainArrangement->arrangeItems();

	if (index > 0) {
		numArrangements = numArrangements + index;
		arrangeItems();
	}
}