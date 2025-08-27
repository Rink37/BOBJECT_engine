<<<<<<< HEAD
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

	this->windowPositions[0] = (((posx - extentx) / 2.0f) + 0.5f) * W;
	this->windowPositions[1] = (((posx + extentx) / 2.0f) + 0.5f) * W;
	this->windowPositions[2] = (((posy - extenty) / 2.0f) + 0.5f) * H;
	this->windowPositions[3] = (((posy + extenty) / 2.0f) + 0.5f) * H;
}

void hArrangement::calculateScreenPosition() {
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

void vArrangement::calculateScreenPosition() {
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

void UIItem::addItem(UIItem *item) {
	Items.push_back(item);
}

void hArrangement::arrangeItems() {
	this->calculateScreenPosition();
	float W = static_cast<float>(Engine::get()->windowWidth);
	float H = static_cast<float>(Engine::get()->windowHeight);

	float totalWidth = 0;
	int numSpacers = 0;

	float xbuffer = this->extentx * 2 * this->spacing; // Each item will have half of this value of empty space on each side

	// We scale all items to have a height equal to the height of this arrangement, then calculate the width of all items summed up

	vector<float> extents;
	
	for (size_t i = 0; i != Items.size(); i++) {
		if (!Items[i]->isSpacer()) {
			extents.push_back((this->extenty * (1 - this->spacing) / (Items[i]->sqAxisRatio * W / H)));
			totalWidth += extents[i] * 2;
		}
		else {
			extents.push_back(0.0f);
			numSpacers++;
		}
	}

	float bufferSpace = xbuffer * Items.size(); // Total space occupied by x buffers
	float spacerSize = 0.0f;

	// Now we find the constant scale factor required to fit all items into the same arrangement horizontally 

	float scaleFactor = (this->extentx * 2 - bufferSpace) / (totalWidth);
	
	if (scaleFactor > 1.0f) {
		// This means that the total width of the items is smaller than the free space, so we need to add more buffer space
		scaleFactor = 1.0f;
		if (numSpacers > 0) {
			spacerSize = (this->extentx * 2 - (totalWidth + xbuffer * Items.size())) / numSpacers;
		}
		else {
			xbuffer = (this->extentx * 2 - totalWidth) / Items.size();
		}
	}
	
	// Finally for all items we calculate their positions on the screen and their sizes 

	float xp, yp, xsc, ysc;

	float currentPosition = 0;

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

void vArrangement::arrangeItems() {
	this->calculateScreenPosition();
	float W = static_cast<float>(Engine::get()->windowWidth);
	float H = static_cast<float>(Engine::get()->windowHeight);

	float totalHeight = 0;
	int numSpacers = 0;

	float ybuffer = this->extentx * 2 * this->spacing;

	vector<float> extents;

	for (size_t i = 0; i != Items.size(); i++) {
		if (!Items[i]->isSpacer()) {
			// sf = static_cast<float>(texHeights[i]) / static_cast<float>(totalTexHeight);
			extents.push_back(this->extentx * (1 - this->spacing) * Items[i]->sqAxisRatio);
			totalHeight += extents[i] * 2 * W / H;
		}
		else {
			extents.push_back(0.0f);
			numSpacers++;
		}
	}

	// Now we calculate the fraction of the height which is taken up by spaces

	float bufferSpace = ybuffer * Items.size(); // Total space occupied by x buffers
	float spacerSize = 0.0f;

	// Now we find the constant scale factor required to fit all items into the same arrangement vertically

	float scaleFactor = (this->extenty * 2 - bufferSpace) / (totalHeight);

	if (scaleFactor > 1.0f) {
		// This means that the total width of the items is smaller than the free space, so we need to add more buffer space
		scaleFactor = 1.0f;
		if (numSpacers > 0) {
			spacerSize = (this->extenty * 2 - (totalHeight + ybuffer * Items.size())) / numSpacers;
		}
		else {
			ybuffer = (this->extenty * 2 - totalHeight) / Items.size();
		}
	}

	// Finally for all items we calculate their positions on the screen and their sizes 

	float xp, yp, xsc, ysc;

	float currentPosition = 0;

	for (size_t i = 0; i != Items.size(); i++) {
		currentPosition += ybuffer / 2;
		if (Items[i]->isSpacer()) {
			currentPosition += spacerSize;
		}
		else {
			ysc = extents[i] * scaleFactor;
			xsc = ysc / Items[i]->sqAxisRatio;
			xp = this->posx;
			yp = this->posy - this->extenty + currentPosition + ysc * W / H;

			Items[i]->update(xp, yp, xsc, ysc);
			Items[i]->updateDisplay();
			Items[i]->arrangeItems();

			currentPosition += ysc * 2 * W / H;
		}
		currentPosition += ybuffer / 2;
	}
}

void hArrangement::updateDisplay() {
	arrangeItems();
	for (size_t i = 0; i != Items.size(); i++) {
		Items[i]->updateDisplay();
	}
}

void vArrangement::updateDisplay() {
	arrangeItems();
	for (size_t i = 0; i != Items.size(); i++) {
		Items[i]->updateDisplay();
	}
=======
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

	this->windowPositions[0] = (((posx - extentx) / 2.0f) + 0.5f) * W;
	this->windowPositions[1] = (((posx + extentx) / 2.0f) + 0.5f) * W;
	this->windowPositions[2] = (((posy - extenty) / 2.0f) + 0.5f) * H;
	this->windowPositions[3] = (((posy + extenty) / 2.0f) + 0.5f) * H;
}

void hArrangement::calculateScreenPosition() {
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

void vArrangement::calculateScreenPosition() {
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

void UIItem::addItem(UIItem *item) {
	Items.push_back(item);
}

void hArrangement::arrangeItems() {
	this->calculateScreenPosition();
	float W = static_cast<float>(Engine::get()->windowWidth);
	float H = static_cast<float>(Engine::get()->windowHeight);

	float totalWidth = 0;
	int numSpacers = 0;

	float xbuffer = this->extentx * 2 * this->spacing; // Each item will have half of this value of empty space on each side

	// We scale all items to have a height equal to the height of this arrangement, then calculate the width of all items summed up

	vector<float> extents;
	
	for (size_t i = 0; i != Items.size(); i++) {
		if (!Items[i]->isSpacer()) {
			extents.push_back((this->extenty * (1 - this->spacing) / (Items[i]->sqAxisRatio * W / H)));
			totalWidth += extents[i] * 2;
		}
		else {
			extents.push_back(0.0f);
			numSpacers++;
		}
	}

	float bufferSpace = xbuffer * Items.size(); // Total space occupied by x buffers
	float spacerSize = 0.0f;

	// Now we find the constant scale factor required to fit all items into the same arrangement horizontally 

	float scaleFactor = (this->extentx * 2 - bufferSpace) / (totalWidth);
	
	if (scaleFactor > 1.0f) {
		// This means that the total width of the items is smaller than the free space, so we need to add more buffer space
		scaleFactor = 1.0f;
		if (numSpacers > 0) {
			spacerSize = (this->extentx * 2 - (totalWidth + xbuffer * Items.size())) / numSpacers;
		}
		else {
			xbuffer = (this->extentx * 2 - totalWidth) / Items.size();
		}
	}
	
	// Finally for all items we calculate their positions on the screen and their sizes 

	float xp, yp, xsc, ysc;

	float currentPosition = 0;

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

void vArrangement::arrangeItems() {
	this->calculateScreenPosition();
	float W = static_cast<float>(Engine::get()->windowWidth);
	float H = static_cast<float>(Engine::get()->windowHeight);

	float totalHeight = 0;
	int numSpacers = 0;

	float ybuffer = this->extentx * 2 * this->spacing;

	vector<float> extents;

	for (size_t i = 0; i != Items.size(); i++) {
		if (!Items[i]->isSpacer()) {
			// sf = static_cast<float>(texHeights[i]) / static_cast<float>(totalTexHeight);
			extents.push_back(this->extentx * (1 - this->spacing) * Items[i]->sqAxisRatio);
			totalHeight += extents[i] * 2 * W / H;
		}
		else {
			extents.push_back(0.0f);
			numSpacers++;
		}
	}

	// Now we calculate the fraction of the height which is taken up by spaces

	float bufferSpace = ybuffer * Items.size(); // Total space occupied by x buffers
	float spacerSize = 0.0f;

	// Now we find the constant scale factor required to fit all items into the same arrangement vertically

	float scaleFactor = (this->extenty * 2 - bufferSpace) / (totalHeight);

	if (scaleFactor > 1.0f) {
		// This means that the total width of the items is smaller than the free space, so we need to add more buffer space
		scaleFactor = 1.0f;
		if (numSpacers > 0) {
			spacerSize = (this->extenty * 2 - (totalHeight + ybuffer * Items.size())) / numSpacers;
		}
		else {
			ybuffer = (this->extenty * 2 - totalHeight) / Items.size();
		}
	}

	// Finally for all items we calculate their positions on the screen and their sizes 

	float xp, yp, xsc, ysc;

	float currentPosition = 0;

	for (size_t i = 0; i != Items.size(); i++) {
		currentPosition += ybuffer / 2;
		if (Items[i]->isSpacer()) {
			currentPosition += spacerSize;
		}
		else {
			ysc = extents[i] * scaleFactor;
			xsc = ysc / Items[i]->sqAxisRatio;
			xp = this->posx;
			yp = this->posy - this->extenty + currentPosition + ysc * W / H;

			Items[i]->update(xp, yp, xsc, ysc);
			Items[i]->updateDisplay();
			Items[i]->arrangeItems();

			currentPosition += ysc * 2 * W / H;
		}
		currentPosition += ybuffer / 2;
	}
}

void hArrangement::updateDisplay() {
	arrangeItems();
	for (size_t i = 0; i != Items.size(); i++) {
		Items[i]->updateDisplay();
	}
}

void vArrangement::updateDisplay() {
	arrangeItems();
	for (size_t i = 0; i != Items.size(); i++) {
		Items[i]->updateDisplay();
	}
>>>>>>> 65e49fd884fc33b59605b3036ff7b8ff8393947b
}