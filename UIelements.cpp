#include "UIelements.h"

using namespace std;

void UIImage::UpdateVertices(int windowWidth, int windowHeight, float xp, float yp, float xsc, float ysc) {
	vertices.clear();

	Vertex vertex;

	vertex.pos = { -xsc + xp, -ysc - yp, 0.0f };
	vertex.normal = { 0.0f, 0.0f, 0.0f };
	vertex.texCoord = { 0.0f, 0.0f };
	vertices.push_back(vertex);
	vertex.pos = { xsc + xp, -ysc - yp, 0.0f };
	vertex.texCoord = { 1.0f, 0.0f };
	vertices.push_back(vertex);
	vertex.pos = { xsc + xp, ysc - yp, 0.0f };
	vertex.texCoord = { 1.0f, 1.0f };
	vertices.push_back(vertex);
	vertex.pos = { -xsc + xp, ysc - yp, 0.0f };
	vertex.texCoord = { 0.0f, 1.0f };
	vertices.push_back(vertex);
}

void hArrangement::addItem(UIItem *item) {
	Items.push_back(item);
}

void vArrangement::addItem(UIItem* item) {
	Items.push_back(item);
}

void hArrangement::arrangeItems(int wWidth, int wHeight) {
	float axisRatio;

	float itemHeight, itemWidth;

	float totalWidth, scalefactor;

	float xp, yp, xsc, ysc;

	this->winWidth = static_cast<float>(wWidth);
	this->winHeight = static_cast<float>(wHeight);

	if (method == 0) {
		// For the moment method = 0 means that we scale items by constant height, with a width of the size ratio
		itemHeight = 2 * extenty - (4 * extenty * spacing);
		totalWidth = 2 * extentx * spacing;

		UIImage image;

		for (size_t i = 0; i != Items.size(); i++) {
			image = *Items[i]->image;
			axisRatio = Items[i]->sqAxisRatio * winHeight / winWidth;
			ysc = itemHeight/2;
			itemWidth = itemHeight * axisRatio;
			xsc = itemWidth/2;
			yp = 0.5f * extenty;
			xp = 0.5f * itemWidth + totalWidth;
			totalWidth += itemWidth + 2 * extenty * spacing;

			Items[i]->update(xp, yp, xsc, ysc, wWidth, wHeight);
		}

		if (totalWidth > 2*extentx) {
			scalefactor = 2*extentx / totalWidth;

			for (size_t i = 0; i != Items.size(); i++) {
				ysc = Items[i]->extenty * scalefactor;
				xsc = Items[i]->extentx * scalefactor;
				xp = Items[i]->posx * scalefactor + posx - extentx;
				yp = Items[i]->posy + posy - extenty;
				Items[i]->update(xp, yp, xsc, ysc, wWidth, wHeight);
			}
		}
		else {
			float distance = (2*extentx - totalWidth) / (Items.size() - 1);

			totalWidth = extentx * spacing;
			for (size_t i = 0; i != Items.size(); i++) {
				if (i > 1) {
					xp = Items[i - 1]->posx + Items[i - 1]->extentx + 2*extenty * spacing + distance + posx - extentx;
				}else{
					xp = Items[i]->posx + this->posx - extentx;
				}
				yp = Items[i]->posy + posy - extenty;
				Items[i]->update(xp, yp, Items[i]->extentx, Items[i]->extenty, wWidth, wHeight);
			}
		}
	}
}

void vArrangement::arrangeItems(int wWidth, int wHeight) {
	float axisRatio;

	float itemHeight, itemWidth;

	float totalHeight, scalefactor;

	float xp, yp, xsc, ysc;

	this->winWidth = static_cast<float>(wWidth);
	this->winHeight = static_cast<float>(wHeight);

	if (method == 0) {
		// For the moment method = 0 means that we scale items by constant height, with a width of the size ratio
		itemWidth = 2*extentx - (4 * extentx * spacing);
		totalHeight = 2 * extenty * spacing;

		UIImage image;

		for (int i = Items.size()-1; i >= 0; i--) {
			image = *Items[i]->image;
			axisRatio = Items[i]->sqAxisRatio * winWidth / winHeight;
			xsc = itemWidth / 2;
			itemHeight = itemWidth * axisRatio;
			ysc = itemHeight / 2;
			xp = 0.5f * extentx;
			yp = 0.5f * itemHeight + totalHeight;
			totalHeight += itemHeight + 2 * extenty * spacing;

			Items[i]->update(xp, yp, xsc, ysc, wWidth, wHeight);
		}

		if (totalHeight > 2*extenty) {
			scalefactor = 2*extenty / totalHeight;

			for (size_t i = 0; i != Items.size(); i++) {
				ysc = Items[i]->extenty * scalefactor;
				xsc = Items[i]->extentx * scalefactor;
				yp = Items[i]->posy * scalefactor + posy - extenty;
				xp = Items[i]->posx + posx - extentx;
				Items[i]->update(xp, yp, xsc, ysc, wWidth, wHeight);
			}
		}
		else {
			float distance = (2*extenty - totalHeight) / (Items.size() - 1);

			totalHeight = extenty * spacing;
			for (size_t i = 0; i != Items.size(); i++) {
				if (i > 1) {
					yp = Items[i - 1]->posy + Items[i - 1]->extenty + 2*extentx * spacing + distance + posy - extenty;
				}
				else {
					yp = Items[i]->posy + posy - extenty;
				}
				xp = Items[i]->posx + posx - extentx;
				Items[i]->update(xp, yp, Items[i]->extentx, Items[i]->extenty, wWidth, wHeight);
			}
		}
	}
}

void hArrangement::updateDisplay(int winWidth, int winHeight) {
	arrangeItems(winWidth, winHeight);
	for (size_t i = 0; i != Items.size(); i++) {
		Items[i]->updateDisplay(winWidth, winHeight);
	}
}

void vArrangement::updateDisplay(int winWidth, int winHeight) {
	arrangeItems(winWidth, winHeight);
	for (size_t i = 0; i != Items.size(); i++) {
		Items[i]->updateDisplay(winWidth, winHeight);
	}
}