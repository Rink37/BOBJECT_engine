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