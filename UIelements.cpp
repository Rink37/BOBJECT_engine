#include "UIelements.h"

using namespace std;

void UIImage::UpdateVertices(float xp, float yp, float xsc, float ysc) {
	mesh.UpdateVertices(xp, yp, xsc, ysc);
}

void bufferPosition(float &extentx, float &extenty, float &posx, float &posy, float brx, float bry, float ax, float ay) {
	
	float minx, maxx;
	minx = ax - extentx;
	maxx = ax + extentx;
	// x coordinates are described by -1 -> 1 = left to right

	if (minx < -1 + brx) {
		posx = -1 + brx + extentx;
	}
	else if (maxx > 1 - brx) {
		posx = 1 - brx - extentx;
	}
	else {
		posx = ax;
	}

	float miny, maxy;
	miny = ay - extenty;
	maxy = ay + extenty;
	// y coordinates are described by -1 -> 1 = top to bottom

	if (miny < -1 + bry) {
		posy = -1 + bry + extenty;
	}
	else if (maxy > 1 - bry) {
		posy = 1 - bry - extenty;
	}
	else {
		posy = ay;
	}
}

void UIItem::calculateScreenPosition() {
	float W = static_cast<float>(Engine::get()->windowWidth);
	float H = static_cast<float>(Engine::get()->windowHeight);

	extenty = extentx * sqAxisRatio * W / H;

	float bufferRatioX, bufferRatioY;

	bufferRatioX = static_cast<float>(buffer) / (2 * W);
	bufferRatioY = static_cast<float>(buffer) / (2 * H);

	bufferPosition(extentx, extenty, posx, posy, bufferRatioX, bufferRatioY, anchorx, anchory);

	this->windowPositions[0] = (((posx - extentx) / 2.0f) + 0.5f) * W; // left position
	this->windowPositions[1] = (((posx + extentx) / 2.0f) + 0.5f) * W; // right position
	this->windowPositions[2] = (((posy - extenty) / 2.0f) + 0.5f) * H; // top position
	this->windowPositions[3] = (((posy + extenty) / 2.0f) + 0.5f) * H; // bottom position

}

void UIItem::addItem(UIItem *item) {
	Items.push_back(item);
}

void TextBox::updateDisplay() {
	this->calculateScreenPosition();
	
	float W = static_cast<float>(Engine::get()->windowWidth);
	float H = static_cast<float>(Engine::get()->windowHeight);

	float scaledCharacterSize = static_cast<float>(characterSize) / W;

	float characterHeight = scaledCharacterSize * W / H;

	float pos_x = posx - extentx;
	float maxPos_x = posx + extentx;
	float pos_y = posy - extenty + characterHeight;
	float maxPos_y = posy + extenty - characterHeight;
	float vSpacing = characterHeight * 2.0f;
	uint32_t maxLines = static_cast<uint32_t>(extenty / characterHeight);

	float lineWidth = 0.0f;
	float maxLineWidth = 2.0f * extentx;

	//float lastSpacePosition = pos_x;
	bool hideCharacters = false;
	std::vector<uint32_t> wordIndices{};
	std::vector<uint32_t> spaceIndices{};
	std::vector<uint32_t> lineIndices{};
	std::vector<float> positions{};

	std::vector<float> xps{};
	std::vector<int> yis{};
	std::vector<float> lineps = {pos_y};
	uint32_t index = 0;
	uint32_t lineIndex = 0;
	for (fontMesh* mesh : characters) {
		if (hideCharacters) {
			mesh->setVisibility(false);
			continue;
		}

		lineWidth += scaledCharacterSize * mesh->advanceWidth * 2.4f;

		bool isBlank = (mesh->unicodeCharacter == SPACE_CHAR || mesh->unicodeCharacter == TAB_CHAR); // We will want to ignore cases where a spacebar causes an overlap at the edge

		if (lineWidth >= maxLineWidth || mesh->unicodeCharacter == NEWLINE_CHAR) {
			// New line handling
			float leftSpacing = 0.0f;

			if (isBlank || mesh->unicodeCharacter == NEWLINE_CHAR) {
				for (uint32_t wIndex : wordIndices) {
					lineIndices.push_back(wIndex);
				}
				wordIndices.clear();
			}

			if (lineIndices.size() > 0) {
				uint32_t lastCharacterIndex = lineIndices[lineIndices.size() - 1];
				lineWidth = xps[lastCharacterIndex] + scaledCharacterSize * characters[lastCharacterIndex]->advanceWidth * 1.2f;

				switch (horizontalArrange) {
				case(ARRANGE_CENTER):
					leftSpacing = (maxPos_x - lineWidth) / 2.0f;
					for (uint32_t index : lineIndices) {
						xps[index] += leftSpacing;
					}
					break;
				case (ARRANGE_END):
					leftSpacing = (maxPos_x - lineWidth);
					for (uint32_t index : lineIndices) {
						xps[index] += leftSpacing;
					}
					break;
				case (ARRANGE_FILL):
					if (spaceIndices.size() > 0) {
						leftSpacing = (maxPos_x - lineWidth) / spaceIndices.size();
						for (uint32_t spaceIndex : spaceIndices) {
							for (uint32_t i = spaceIndex; i != lineIndices[lineIndices.size() - 1] + 1; i++) {
								xps[i] += leftSpacing;
							}
						}
					}
					else {
						leftSpacing = (maxPos_x - lineWidth) / (lineIndices.size() - 1);
						float totalSpacing = 0.0f;
						for (uint32_t index : lineIndices) {
							xps[index] += totalSpacing;
							totalSpacing += leftSpacing;
						}
					}
					break;
				default:
					break;
				}
			}
			
			pos_x = posx - extentx;
			pos_y += vSpacing;
			lineIndex++;
			lineps.push_back(pos_y);
			lineIndices.clear();
			spaceIndices.clear();
			lineWidth = 0.0f;
			if (pos_y > maxPos_y) {
				hideCharacters = true;
			}

			//std::cout << "Updating last word" << std::endl;

			if (mesh->unicodeCharacter == NEWLINE_CHAR || isBlank) {
				xps.push_back(pos_x);
				yis.push_back(lineIndex);
				mesh->setVisibility(false);
			}
			else {
				for (uint32_t meshRef : wordIndices) {
					characters[meshRef]->setVisibility(!hideCharacters);
					pos_x += scaledCharacterSize * characters[meshRef]->advanceWidth * 1.2f;
					xps[meshRef] = pos_x;
					yis[meshRef] = lineIndex;
					pos_x += scaledCharacterSize * characters[meshRef]->advanceWidth * 1.2f;
				}
				wordIndices.push_back(index);
				mesh->setVisibility(!hideCharacters);
				pos_x += scaledCharacterSize * mesh->advanceWidth * 1.2f;
				xps.push_back(pos_x);
				yis.push_back(lineIndex);
				pos_x += scaledCharacterSize * mesh->advanceWidth * 1.2f;
				lineWidth = pos_x - (posx - extentx);
			}
			//std::cout << "Successfully updated word" << std::endl;
		}
		else {
			if (isBlank) {
				for (uint32_t wIndex : wordIndices) {
					lineIndices.push_back(wIndex);
				}
				wordIndices.clear();
				spaceIndices.push_back(index);
				lineIndices.push_back(index);
				mesh->setVisibility(false);
				pos_x += scaledCharacterSize * mesh->advanceWidth * 1.2f;
				xps.push_back(pos_x);
				yis.push_back(lineIndex);
				pos_x += scaledCharacterSize * mesh->advanceWidth * 1.2f;
			}
			else {
				wordIndices.push_back(index);
				mesh->setVisibility(!hideCharacters);
				pos_x += scaledCharacterSize * mesh->advanceWidth * 1.2f;
				xps.push_back(pos_x);
				yis.push_back(lineIndex);
				pos_x += scaledCharacterSize * mesh->advanceWidth * 1.2f;
			}
		}
		index++;
	}

	lineIndex++;
	lineIndex = (lineIndex > maxLines) ? maxLines : lineIndex;
		
	float remainingVSpace = 2.0f * extenty - lineIndex * characterHeight * 2.0f;
	remainingVSpace = (remainingVSpace < 0.0f) ? 0.0f : remainingVSpace;

	switch (verticalArrange) {
	case (ARRANGE_CENTER):
		remainingVSpace /= 2.0f;
		for (int i = 0; i != lineps.size(); i++) {
			lineps[i] += remainingVSpace;
		}
		break;
	case (ARRANGE_END):
		for (int i = 0; i != lineps.size(); i++) {
			lineps[i] += remainingVSpace;
		}
		break;
	case (ARRANGE_FILL):
		vSpacing = remainingVSpace / (lineIndex - 1);
		for (int i = 0; i != lineps.size(); i++) {
			lineps[i] += vSpacing * i;
		}
		break;
	default:
		break;
	}
	float factor = W / H;
	for (uint32_t i = 0; i != xps.size(); i++) {
		characters[i]->UpdateVertices(xps[i], lineps[yis[i]], scaledCharacterSize, factor);
	}
}

void TextBox::calculateScreenPosition() {
	float W = static_cast<float>(Engine::get()->windowWidth);
	float H = static_cast<float>(Engine::get()->windowHeight);

	float bufferRatioX, bufferRatioY;

	bufferRatioX = static_cast<float>(buffer) / (2 * W);
	bufferRatioY = static_cast<float>(buffer) / (2 * H);

	bufferPosition(extentx, extenty, posx, posy, bufferRatioX, bufferRatioY, anchorx, anchory);

	this->windowPositions[0] = (((posx - extentx) / 2.0f) + 0.5f) * W;
	this->windowPositions[1] = (((posx + extentx) / 2.0f) + 0.5f) * W;
	this->windowPositions[2] = (((posy - extenty) / 2.0f) + 0.5f) * H;
	this->windowPositions[3] = (((posy + extenty) / 2.0f) + 0.5f) * H;
}

void Arrangement::calculateScreenPosition() {
	float W = static_cast<float>(Engine::get()->windowWidth);
	float H = static_cast<float>(Engine::get()->windowHeight);

	float bufferRatioX, bufferRatioY;

	bufferRatioX = static_cast<float>(buffer) / (2 * W);
	bufferRatioY = static_cast<float>(buffer) / (2 * H);

	bufferPosition(extentx, extenty, posx, posy, bufferRatioX, bufferRatioY, anchorx, anchory);

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
					if (Items[i]->baseExtenty * 2 > maxSize) {
						maxSize = Items[i]->baseExtenty * 2;
					}
				}
			}
			rescale = (this->extenty * 2 - this->spacing) / maxSize;
			for (size_t i = 0; i != Items.size(); i++) {
				if (!Items[i]->isSpacer()) {
					if (!Items[i]->isArrangement() || numSpacers != 0) {
						extents.push_back((Items[i]->baseExtenty * rescale / (Items[i]->baseSqAxisRatio * W / H)));
					}
					else {
						extents.push_back(Items[i]->baseExtentx * rescale);
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
					if (Items[i]->baseExtentx * 2 > maxSize) {
						maxSize = Items[i]->baseExtentx * 2;
					}
				}
			}
			rescale = (this->extentx * 2 - this->spacing) / maxSize;
			for (size_t i = 0; i != Items.size(); i++) {
				if (!Items[i]->isSpacer()) {
					if (!Items[i]->isArrangement() || numSpacers != 0) {
						extents.push_back(Items[i]->baseExtentx * rescale * Items[i]->baseSqAxisRatio);
						totalArea += extents[i] * 2 * W / H;
					}
					else {
						extents.push_back(Items[i]->baseExtenty * rescale);
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
						extents.push_back((this->extenty * (1.0f - this->spacing) / (Items[i]->baseSqAxisRatio * W / H)));
					}
					else {
						extents.push_back(Items[i]->baseExtentx * (1.0f - this->spacing));
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
						extents.push_back(this->extentx * (1.0f - this->spacing) * Items[i]->baseSqAxisRatio);
						totalArea += extents[i] * 2 * W / H;
					}
					else {
						extents.push_back(Items[i]->baseExtenty * (1.0f - this->spacing));
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
			xsc = ysc / Items[i]->baseSqAxisRatio;
			xp = this->posx;
			if (!Items[i]->isArrangement()) {
				yp = this->posy - this->extenty + currentPosition + ysc * vScale; // vScale = W/H
			}
			else {
				yp = this->posy - this->extenty + currentPosition + ysc;
				xsc = this->extentx * (1.0f - this->spacing);
			}
			

			Items[i]->updateArrangedPosition(xp, yp, xsc, ysc);
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
			ysc = xsc * Items[i]->baseSqAxisRatio;
			yp = this->posy;
			xp = this->posx - this->extentx + currentPosition + xsc;
			if (Items[i]->isArrangement()) {
				ysc = this->extenty * (1.0f - this->spacing);
			}

			Items[i]->updateArrangedPosition(xp, yp, xsc, ysc);
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

	bufferPosition(extentx, extenty, posx, posy, bufferRatioX, bufferRatioY, anchorx, anchory);

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

	bufferPosition(extentx, extenty, posx, posy, bufferRatioX, bufferRatioY, anchorx, anchory);

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

	bufferPosition(extentx, extenty, posx, posy, bufferRatioX, bufferRatioY, anchorx, anchory);

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