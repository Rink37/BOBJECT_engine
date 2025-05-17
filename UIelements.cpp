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

	if (vBuffer == nullptr) {
		createVertexBuffer();
		createIndexBuffer();
	}
	else {
		updateVertexBuffer();
	}
}

void UIImage::createVertexBuffer() {

	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	Engine::get()->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(Engine::get()->device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(Engine::get()->device, stagingBufferMemory);

	Engine::get()->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, vertexBuffer, vertexBufferMemory);

	Engine::get()->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	vkMapMemory(Engine::get()->device, vertexBufferMemory, 0, bufferSize, 0, &vBuffer);

	vkDestroyBuffer(Engine::get()->device, stagingBuffer, nullptr);
	vkFreeMemory(Engine::get()->device, stagingBufferMemory, nullptr);
}

void UIImage::updateVertexBuffer() {
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	memcpy(vBuffer, vertices.data(), (size_t)bufferSize);
}

void UIImage::createIndexBuffer() {

	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	Engine::get()->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(Engine::get()->device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(Engine::get()->device, stagingBufferMemory);

	Engine::get()->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

	Engine::get()->copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(Engine::get()->device, stagingBuffer, nullptr);
	vkFreeMemory(Engine::get()->device, stagingBufferMemory, nullptr);
}

void UIItem::calculateScreenPosition(int winWidth, int winHeight) {
	float W = static_cast<float>(winWidth);
	float H = static_cast<float>(winHeight);

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

void hArrangement::arrangeItems(int wWidth, int wHeight) {
	this->calculateScreenPosition(wWidth, wHeight);
	this->winWidth = static_cast<float>(wWidth);
	this->winHeight = static_cast<float>(wHeight);

	float totalWidth = 0;

	// We scale all items to have a height of 1, then calculate the width of all items summed up
	
	for (size_t i = 0; i != Items.size(); i++) {
		Items[i]->update(0.0f, 0.0f, this->extenty/Items[i]->sqAxisRatio, this->extenty, wWidth, wHeight);
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
		Items[i]->update(xp, yp, xsc, ysc, wWidth, wHeight);
		Items[i]->updateDisplay(wWidth, wHeight);
		Items[i]->arrangeItems(wWidth, wHeight);
	}
}

void vArrangement::arrangeItems(int wWidth, int wHeight) {
	this->calculateScreenPosition(wWidth, wHeight);
	this->winWidth = static_cast<float>(wWidth);
	this->winHeight = static_cast<float>(wHeight);

	float totalHeight = 0;

	// We scale all items to have a width of 1, then calculate the width of all items summed up

	for (size_t i = 0; i != Items.size(); i++) {
		Items[i]->update(0.0f, 0.0f, this->extentx, this->extentx*Items[i]->sqAxisRatio, wWidth, wHeight);
		totalHeight += Items[i]->extenty * 2 * winWidth/winHeight;
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
		yp = this->posy - this->extenty + occupiedFraction + (Items[j]->extenty * winWidth/winHeight * scaleFactor);
		ysc = scaleFactor * Items[j]->extenty;
		xsc = ysc / Items[j]->sqAxisRatio;
		occupiedFraction += ysc * winWidth/winHeight * 2 + spacing * this->extenty * 2;
		Items[j]->update(xp, yp, xsc, ysc, wWidth, wHeight);
		Items[j]->updateDisplay(wWidth, wHeight);
		Items[j]->arrangeItems(wWidth, wHeight);
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