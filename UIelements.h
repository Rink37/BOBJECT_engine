#include"Bobject_Engine.h"

#ifndef UI_ELEMENTS
#define UI_ELEMENTS

#include"Textures.h"
#include"Materials.h"
#include<iostream>
#include<vector>
#include<array>
#include<chrono>

#include"include/ImageDataType.h"

struct UIImage {
	//std::string texPath;
	imageData* imgData;

	int texHeight = 1;
	int texWidth = 1;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices = { 0, 3, 2, 2, 1, 0 };

	//imageTexture* texture = nullptr;
	//webcamTexture* wtexture = nullptr;

	Material* mat = nullptr;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	void* vBuffer = nullptr;

	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	//VkDescriptorPool descriptorPool;
	//std::vector<VkDescriptorSet> descriptorSets;

	uint32_t mipLevels;

	void UpdateVertices(int, int, float, float, float, float);
	void createVertexBuffer();
	void createIndexBuffer();
	void updateVertexBuffer();
};

struct UIItem {
	float posx, posy;
	float extentx, extenty;

	float winWidth, winHeight;

	float sqAxisRatio; // The ratio between axes if the window was perfectly square

	std::string Name = "Unlabelled";

	UIImage* image = new UIImage;

	virtual void update(float x, float y, float xsize, float ysize, int wWidth, int wHeight) {
		this->posx = x;
		this->posy = y;

		this->extentx = xsize;
		this->extenty = ysize;

		this->winWidth = static_cast<float>(wWidth);
		this->winHeight = static_cast<float>(wHeight);

		arrangeItems(wWidth, wHeight);

		if (image->texWidth > 1) {
			this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);
		}
	};

	virtual void updateDisplay(int winWidth, int winHeight) {
		this->winWidth = static_cast<float>(winWidth);
		this->winHeight = static_cast<float>(winHeight);
		image->UpdateVertices(winWidth, winHeight, posx, posy, extentx, extenty);
	}

	virtual void getSubclasses(std::vector<UIItem*> &scs) {
		scs.push_back(this);
	};

	virtual void getImages(std::vector<UIImage*>& images) {
		images.push_back(image);
	};

	virtual void arrangeItems(int, int) {

	};

	virtual void checkForEvent(double, double, int) {

	};
};

class WebcamPanel : public UIItem {
// Represents only a webcam view
public:
	WebcamPanel(float x, float y, float xsize, float ysize, int wWidth, int wHeight, Material* webcamMat) {
		update(x, y, xsize, ysize, wWidth, wHeight);
		image->mat = webcamMat;

		image->texWidth = image->mat->textures[0]->texWidth;
		image->texHeight = image->mat->textures[0]->texHeight;

		this->sqAxisRatio = ysize / xsize;
	}
};

class Button : public UIItem // Here a button is just a rectangle area in screen space which can be queried with coordinates to check if it has been pressed
{
private:
	float windowPositions[4] = { 0.0f };
public:
	std::function<void(UIItem*)> clickFunction = nullptr;
	std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

	Button(float x, float y, float xsize, float ysize, imageData* iDpointer, int wWidth, int wHeight) {
		this->sqAxisRatio = ysize / xsize;
		update(x, y, xsize, ysize, wWidth, wHeight);

		Texture* tex = new imageTexture(iDpointer);
		image->mat = new Material(tex);
		
		image->texWidth = image->mat->textures[0]->texWidth;
		image->texHeight = image->mat->textures[0]->texHeight;
	};

	void update(float x, float y, float xsize, float ysize, int wWidth, int wHeight) {
		this->posx = x;
		this->posy = y;

		this->extentx = xsize;
		this->extenty = ysize;

		this->winWidth = static_cast<float>(wWidth);
		this->winHeight = static_cast<float>(wHeight);

		this->windowPositions[0] = (((posx - extentx) / 2.0f) + 0.5f) * this->winWidth;
		this->windowPositions[1] = (((posx + extentx) / 2.0f) + 0.5f) * this->winWidth;
		this->windowPositions[2] = (((-posy + extenty) / 2.0f) + 0.5f) * this->winHeight;
		this->windowPositions[3] = (((-posy - extenty) / 2.0f) + 0.5f) * this->winHeight;

		arrangeItems(wWidth, wHeight);

		if (image->texWidth > 1) {
			this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);
		}
	};

	bool isInArea(double x, double y) {
		bool result = false;
		if (x >= windowPositions[0] && x <= windowPositions[1] && y <= windowPositions[2] && y >= windowPositions[3]) {
			result = true;
		}
		return result;
	};

	void setClickFunction(std::function<void(UIItem*)> func) {
		clickFunction = func;
	}

	void checkForEvent(double mousex, double mousey, int state) {
		if (clickFunction != nullptr) {
			bool check = isInArea(mousex, mousey);
			if (check && state == 1) {
				auto currentTime = std::chrono::steady_clock::now();
				auto duration = std::chrono::duration<double>(currentTime - startTime);
				if (duration.count() >= 0.25f) {
					clickFunction(this);
					startTime = std::chrono::steady_clock::now();
				}
			};
		}
		else {
			std::cout << "No valid click function found" << std::endl;
		}
	};
};

class Checkbox : public UIItem
{
private:
	float windowPositions[4] = { 0.0f };
public:
	bool state;

	Checkbox(float x, float y, float xsize, float ysize, std::string texpath, int wWidth, int wHeight, bool defaultState) {
		this->sqAxisRatio = ysize / xsize;
		update(x, y, xsize, ysize, wWidth, wHeight);
		//image->texPath = texpath;
		state = defaultState;
	};

	void update(float x, float y, float xsize, float ysize, int wWidth, int wHeight) {
		this->posx = x;
		this->posy = y;

		this->extentx = xsize;
		this->extenty = ysize;

		this->winWidth = static_cast<float>(wWidth);
		this->winHeight = static_cast<float>(wHeight);

		this->windowPositions[0] = (((posx - extentx) / 2.0f) + 0.5f) * this->winWidth;
		this->windowPositions[1] = (((posx + extentx) / 2.0f) + 0.5f) * this->winWidth;
		this->windowPositions[2] = (((-posy + extenty) / 2.0f) + 0.5f) * this->winHeight;
		this->windowPositions[3] = (((-posy - extenty) / 2.0f) + 0.5f) * this->winHeight;

		arrangeItems(wWidth, wHeight);

		if (image->texWidth > 1) {
			this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);
		}
	};

	bool isInArea(double x, double y) {
		bool result = false;
		if (x >= windowPositions[0] && x <= windowPositions[1] && y <= windowPositions[2] && y >= windowPositions[3]) {
			result = true;
		}
		return result;
	};

	void checkForEvent(float mousex, float mousey, int clickstate) {
		bool check = isInArea(mousex, mousey);
		if (check && clickstate == 1) {
			state = !state;
		};
	};
};

class hArrangement : public UIItem {
public:
	float spacing;
	
	hArrangement(float px, float py, float ex, float ey, float spc) {
		this->posx = px;
		this->posy = py;
		this->extentx = ex;
		this->extenty = ey;
		this->spacing = spc;

		this->sqAxisRatio = ey / ex;
	}

	std::vector<UIItem *> Items;

	void addItem(UIItem *item);

	void updateDisplay(int, int);

	void arrangeItems(int, int); // Should be called any time there is a layout change

	void getSubclasses(std::vector<UIItem*> &scs) {
		scs.push_back(this);
		for (size_t i = 0; i != Items.size(); i++) {
			//scs.push_back(Items[i]);
			std::vector<UIItem*> sscs;
			Items[i]->getSubclasses(sscs);
			for (size_t j = 0; j != sscs.size(); j++) {
				scs.push_back(sscs[j]);
			}
		}
	};

	void getImages(std::vector<UIImage*>& images) {
		for (size_t i = 0; i != Items.size(); i++) {
			std::vector<UIImage*> subimages;
			Items[i]->getImages(subimages);
			for (size_t j = 0; j != subimages.size(); j++) {
				images.push_back(subimages[j]);
			}
		}
	};

private:

	int method = 0;
};

class vArrangement : public UIItem{
public:
	float spacing;

	vArrangement(float px, float py, float ex, float ey, float spc) {
		posx = px;
		posy = py;
		extentx = ex;
		extenty = ey;
		spacing = spc;

		this->sqAxisRatio = ey / ex;
	}

	std::vector<UIItem*> Items;

	void addItem(UIItem* item);

	void updateDisplay(int, int);

	void arrangeItems(int, int); // Should be called any time there is a layout change

	void getSubclasses(std::vector<UIItem*>& scs) {
		scs.push_back(this);
		for (size_t i = 0; i != Items.size(); i++) {
			//scs.push_back(Items[i]);
			std::vector<UIItem*> sscs;
			Items[i]->getSubclasses(sscs);
			for (size_t j = 0; j != sscs.size(); j++) {
				scs.push_back(sscs[j]);
			}
		}
	};

	void getImages(std::vector<UIImage*>& images) {
		for (size_t i = 0; i != Items.size(); i++) {
			std::vector<UIImage*> subimages;
			Items[i]->getImages(subimages);
			for (size_t j = 0; j != subimages.size(); j++) {
				images.push_back(subimages[j]);
			}
		}
	};

private:

	int method = 0;
};

#endif
