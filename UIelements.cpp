#include "UIelements.h"

using namespace std;

void UIImage::UpdateVertices(float xp, float yp, float xsc, float ysc) {
	mesh.UpdateVertices(xp, yp, xsc, ysc);
}

void UIItem::calculateScreenPosition() {
	float W = static_cast<float>(Engine::get()->windowWidth);
	float H = static_cast<float>(Engine::get()->windowHeight);

	extenty = extentx * sqAxisRatio * W / H;

	float bufferRatioX, bufferRatioY;

	bufferRatioX = static_cast<float>(buffer) / (2 * W);
	bufferRatioY = static_cast<float>(buffer) / (2 * H);

	float minx, maxx;
	minx = -extentx + posx;
	maxx = posx + extentx;

	if (minx < -1+bufferRatioX) {
		posx = -1+bufferRatioX + extentx;
	}
	else if (maxx > 1-bufferRatioX) {
		posx = 1-bufferRatioX - extentx;
	}

	float miny, maxy;
	miny = -extenty - posy;
	maxy = extenty - posy;

	if (miny < -1+bufferRatioY) {
		posy = -(- 1 + bufferRatioY + extenty);
	}
	else if (maxy > 1-bufferRatioY) {
		posy = -(1-bufferRatioY - extenty);
	}

	this->windowPositions[0] = (((posx - extentx) / 2.0f) + 0.5f) * W;
	this->windowPositions[1] = (((posx + extentx) / 2.0f) + 0.5f) * W;
	this->windowPositions[2] = (((-posy + extenty) / 2.0f) + 0.5f) * H;
	this->windowPositions[3] = (((-posy - extenty) / 2.0f) + 0.5f) * H;
}

void hArrangement::addItem(UIItem *item) {
	Items.push_back(item);
}

void vArrangement::addItem(UIItem* item) {
	Items.push_back(item);
}

void hArrangement::arrangeItems() {
	this->calculateScreenPosition();

	float totalWidth = 0;

	// We scale all items to have a height of 1, then calculate the width of all items summed up
	
	for (size_t i = 0; i != Items.size(); i++) {
		Items[i]->update(0.0f, 0.0f, this->extenty/Items[i]->sqAxisRatio, this->extenty);
		totalWidth += Items[i]->extentx * 2;
	}

	// Now we calculate the fraction of the width which is taken up by spaces

	float spaceFraction = spacing * (Items.size());

	// Now we find the constant scale factor required to fit all items into the same arrangement horizontally 

	float scaleFactor = ((1 - spaceFraction) * this->extentx * 2) / (totalWidth);
	if (scaleFactor > 1.0f) {
		scaleFactor = 1.0f;
	}
	
	// Finally for all items we calculate their positions on the screen and their sizes 

	float xp, yp, xsc, ysc;

	float occupiedFraction = spacing/2 * this->extentx * 2;

	for (size_t i = 0; i != Items.size(); i++) {
		yp = this->posy;
		xp = this->posx - this->extentx + occupiedFraction + (Items[i]->extentx * scaleFactor);
		xsc = scaleFactor*Items[i]->extentx;
		ysc = xsc * Items[i]->sqAxisRatio;
		occupiedFraction +=  xsc*2 + spacing*this->extentx*2;
		Items[i]->update(xp, yp, xsc, ysc);
		Items[i]->updateDisplay();
		Items[i]->arrangeItems();
	}
}

void vArrangement::arrangeItems() {
	this->calculateScreenPosition();
	float W = static_cast<float>(Engine::get()->windowWidth);
	float H = static_cast<float>(Engine::get()->windowHeight);

	float totalHeight = 0;

	// We scale all items to have a width of 1, then calculate the width of all items summed up

	for (size_t i = 0; i != Items.size(); i++) {
		Items[i]->update(0.0f, 0.0f, this->extentx, this->extentx*Items[i]->sqAxisRatio);
		totalHeight += Items[i]->extenty * 2 * W/H;
	}

	// Now we calculate the fraction of the height which is taken up by spaces

	float spaceFraction = spacing * (Items.size());

	// Now we find the constant scale factor required to fit all items into the same arrangement vertically

	float scaleFactor = ((1 - spaceFraction) * this->extenty * 2) / (totalHeight);
	if (scaleFactor > 1.0f) {
		scaleFactor = 1.0f;
	}

	// Finally for all items we calculate their positions on the screen and their sizes 

	float xp, yp, xsc, ysc;

	float occupiedFraction = spacing / 2 * this->extenty * 2;

	size_t j = 0;

	for (size_t i = 0; i != Items.size(); i++) {
		j = Items.size() - 1 - i;
		xp = this->posx;
		yp = this->posy - this->extenty + occupiedFraction + (Items[j]->extenty * W/H * scaleFactor);
		ysc = scaleFactor * Items[j]->extenty;
		xsc = ysc / Items[j]->sqAxisRatio;
		occupiedFraction += ysc * W/H * 2 + spacing * this->extenty * 2;
		Items[j]->update(xp, yp, xsc, ysc);
		Items[j]->updateDisplay();
		Items[j]->arrangeItems();
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
}