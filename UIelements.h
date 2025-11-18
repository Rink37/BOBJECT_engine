#ifndef UI_ELEMENTS
#define UI_ELEMENTS

#include"Bobject_Engine.h"
#include"Textures.h"
#include"Materials.h"
#include"Meshes.h"
#include<iostream>
#include<vector>
#include<array>
#include<chrono>
#include<cmath>

#include"LoadLists.h"
#include"include/ImageDataType.h"
#include"InputManager.h"

#define ARRANGE_FILL 0
#define ARRANGE_START 1
#define ARRANGE_CENTER 2
#define ARRANGE_END 3

#define SCALE_BY_CONTAINER 0
#define SCALE_BY_DIMENSIONS 1
#define SCALE_BY_IMAGES 2

#define ORIENT_VERTICAL 0
#define ORIENT_HORIZONTAL 1

#define SLIDER_CONTINUOUS 0
#define SLIDER_DISCRETE 1

#define PI 3.14159265f
#define OPF_PI 4.71238898f // Equivalent to 3*PI / 2 - basic optimization
#define HALF_PI 1.57079632f // Equivalent to PI / 2

struct UIImage {
	bool isVisible = true;

	int texHeight = 0;
	int texWidth = 0;

	std::vector<Material*> mat;
	uint32_t matidx = 0;
	
	UIMesh mesh;

	uint32_t mipLevels = 0;

	bool isGray = true;

	void UpdateVertices(float, float, float, float);

	void cleanup() {
		mesh.cleanup();
	}

	VkCommandBuffer draw(VkCommandBuffer commandBuffer, uint32_t currentFrame) {
		if (!isVisible) {
			return commandBuffer;
		}
		VkBuffer vertexBuffers[] = { mesh.vertexBuffer };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Engine::get()->diffusePipelineLayout, 0, 1, &mat[matidx]->descriptorSets[currentFrame], 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);

		return commandBuffer;
	}

	~UIImage() {
		cleanup();
	}
};

struct UIItem {
	float buffer = 50.0;
	
	float posx, posy = 0.0f;
	float extentx, extenty = 1.0f;
	float anchorx, anchory = 0.0f;

	float windowPositions[4] = { 0.0f };

	float sqAxisRatio = 0.0f; // The ratio between axes if the window was perfectly square

	std::string Name = "Unlabelled";

	std::shared_ptr<UIImage> image = nullptr; // new UIImage;

	std::vector<UIItem*> Items; // These items are managed by owning widgets, pointers reference a vector of objects
	std::vector<Texture*> textures; // These items are stored in load lists, so this is just a vector of pointers

	virtual void addItem(UIItem*);

	bool isEnabled = true;
	bool activestate = false;

	int clickType = LMB_PRESS;
	int posType = MOUSE_HOVER;

	virtual void update(float x, float y, float xsize, float ysize) {
		this->posx = x;
		this->posy = y;
		this->anchorx = x;
		this->anchory = y;

		this->extentx = xsize;
		this->extenty = ysize;

		arrangeItems();

		if (image != nullptr && image->texHeight > 1) {
			this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);
		}
		else {
			this->sqAxisRatio = ysize / xsize;
		}
	};

	virtual void updateDisplay() {
		this->calculateScreenPosition();
		if (image != nullptr) {
			image->UpdateVertices(posx, posy, extentx, extenty);
		}
	}

	virtual void getSubclasses(std::vector<UIItem*> &scs) {
		scs.push_back(this);
	};

	virtual void getImages(std::vector<UIImage*>& images, bool isUI) {
		if (image != nullptr && image->texHeight > 1 && image->isGray == isUI) {
			images.push_back(image.get());
		}
	};

	virtual bool isInArea(double x, double y) {
		bool result = false;
		if (x >= windowPositions[0] && x <= windowPositions[1] && y >= windowPositions[2] && y <= windowPositions[3]) {
			result = true;
		}
		return result;
	};

	virtual void arrangeItems() {

	};

	virtual bool checkForClickEvent(double, double, int) {
		return false;
	};

	virtual bool checkForPosEvent(double, double, int) {
		return false;
	};

	virtual void calculateScreenPosition();

	virtual bool isSpacer() {
		return false;
	}

	virtual bool isArrangement() {
		return false;
	}

	virtual void setVisibility(bool vis) {
		image->isVisible = vis;
	}

	virtual void setIsEnabled(bool enabled) {
		isEnabled = enabled;
	}

	virtual void drawUI(VkCommandBuffer commandBuffer, uint32_t currentFrame) {
		std::vector<UIImage*> images;
		getImages(images, true);

		for (UIImage* image : images) {
			image->draw(commandBuffer, currentFrame);
		}
	}

	virtual void drawImages(VkCommandBuffer commandBuffer, uint32_t currentFrame) {
		std::vector<UIImage*> images;
		getImages(images, false);

		for (UIImage* image : images) {
			image->draw(commandBuffer, currentFrame);
		}
	}

	virtual void cleanup() {
		std::vector<UIImage*> images;
		getImages(images, true);
		getImages(images, false);

		for (UIImage* image : images) {
			image->cleanup();
			image = nullptr;
		}
	}
};

class ImagePanel : public UIItem {
// Represents only a webcam view
public:
	bool isWebcam;

	ImagePanel(Material* surf, bool iW) {
		update(0.0f, 0.0f, 1.0f, 1.0f);

		image = std::make_shared<UIImage>(new UIImage);
		image->mat.emplace_back(surf);

		image->isGray = surf->isUIMat;

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

		this->extenty = this->extentx * this->sqAxisRatio;

		this->isWebcam = iW;
	}
	
	ImagePanel(float x, float y, float xsize, float ysize, Material* surf, bool iW) {
		update(x, -1.0f * y, xsize, ysize);

		image = std::make_shared<UIImage>(new UIImage);
		image->mat.emplace_back(surf);

		image->isGray = surf->isUIMat;

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = ysize / xsize;

		this->isWebcam = iW;
	}

	ImagePanel() = default;
};

class Button : public UIItem // Here a button is just a rectangle area in screen space which can be queried with coordinates to check if it has been pressed
{
public:
	std::function<void(UIItem*)> clickFunction = nullptr;

	Button() = default;

	Button(Material* mat, std::function<void(UIItem*)> func) {

		image = std::make_shared<UIImage>(new UIImage);
		image->mat.push_back(mat);

		image->isGray = mat->isUIMat;

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

		clickFunction = func;

		update(0.0f, 0.0f, 1.0f, 1.0f * this->sqAxisRatio);
	}

	Button(Material* mat, std::function<void(UIItem*)> func, int code) {

		image = std::make_shared<UIImage>(new UIImage);
		image->mat.push_back(mat);

		image->isGray = mat->isUIMat;

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

		clickFunction = func;
		clickType = code;

		update(0.0f, 0.0f, 1.0f, 1.0f * this->sqAxisRatio);
	}

	Button(Material* mat) {

		image = std::make_shared<UIImage>(new UIImage);
		image->mat.push_back(mat);

		image->isGray = mat->isUIMat;

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

		update(0.0f, 0.0f, 1.0f, 1.0f * this->sqAxisRatio);
	}

	void setClickFunction(std::function<void(UIItem*)> func) {
		clickFunction = func;
	}

	void setClickFunction(std::function<void(UIItem*)> func, int code) {
		clickFunction = func;
		clickType = code;
	}

	bool checkForClickEvent(double mousex, double mousey, int eventType) {
		bool found = false;
		if (clickFunction != nullptr && isEnabled && eventType == clickType) {
			if (isInArea(mousex, mousey)) {
				found = true;
				clickFunction(this);
			}
		}
		return found;
	};
};

class Checkbox : public UIItem
{
public:
	std::function<void(UIItem*)> clickFunction = nullptr;

	Checkbox() = default;

	Checkbox(Material* onMat, Material* offMat, std::function<void(UIItem*)> func) {
		
		image = std::unique_ptr<UIImage>(new UIImage);
		image->mat.push_back(onMat);
		image->mat.push_back(offMat);
		image->matidx = 0;
		this->activestate = true;

		image->isGray = onMat->isUIMat;

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

		update(0.0f, 0.0f, 1.0f, 1.0f * sqAxisRatio);

		clickFunction = func;
	}

	Checkbox(Material* onMat, Material* offMat, std::function<void(UIItem*)> func, int code) {

		image = std::unique_ptr<UIImage>(new UIImage);
		image->mat.push_back(onMat);
		image->mat.push_back(offMat);
		image->matidx = 0;
		this->activestate = true;

		image->isGray = onMat->isUIMat;

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

		update(0.0f, 0.0f, 1.0f, 1.0f * sqAxisRatio);

		clickFunction = func;
		clickType = code;
	}

	void setClickFunction(std::function<void(UIItem*)> func) {
		clickFunction = func;
	}

	void setClickFunction(std::function<void(UIItem*)> func, int code) {
		clickFunction = func;
		clickType = code;
	}

	bool checkForClickEvent(double mousex, double mousey, int eventType) {
		bool found = false;
		bool check = isInArea(mousex, mousey);
		if (check && eventType == clickType && isEnabled) {
			found = true;
			activestate = !activestate;
			if (activestate) {
				image->matidx = 0;
			}
			else {
				image->matidx = 1;
			}
			if (clickFunction != nullptr) {
				clickFunction(this);
			}
		};
		return found;
	};
};

class spacer : public UIItem {
public:
	bool isSpacer() {
		return true;
	}

	void update(float x, float y, float xsize, float ysize) {};

	void updateDisplay() {};

	void getImages(std::vector<UIImage*>& images) {};

	void arrangeItems() {};

	void checkForEvent(double, double, int) {};

	void calculateScreenPosition() {};

	void setVisibility(bool) {};
};

class Arrangement : public UIItem {
public:
	float spacing;

	Arrangement() = default;

	Arrangement(int orient, float px, float py, float ex, float ey, float spc) {
		setDims(px, py, ex, ey, spc);
		this->orientation = orient;
	}

	Arrangement(int orient, float px, float py, float ex, float ey, float spc, int arrangeMethod) {
		setDims(px, py, ex, ey, spc);

		this->method = arrangeMethod;

		this->orientation = orient;
	}

	Arrangement(int orient, float px, float py, float ex, float ey, float spc, int arrangeMethod, int sizeMethod) {
		setDims(px, py, ex, ey, spc);

		this->sizing = sizeMethod;
		this->method = arrangeMethod;

		this->orientation = orient;
	}

	void setSizeMethod(int sizeMethod) {
		this->sizing = sizeMethod;
	}

	void setArrangeMethod(int arrangeMethod) {
		this->method = arrangeMethod;
	}

	void removeItem(uint32_t index) {
		Items.erase(Items.begin() + index);
		arrangeItems();
	}

	void calculateScreenPosition();

	void updateDisplay();

	void arrangeItems();

	bool checkForSpace(UIItem*);

	bool isArrangement() {
		return true;
	}
	
	void getSubclasses(std::vector<UIItem*>& scs) {
		scs.push_back(this);
		for (size_t i = 0; i != Items.size(); i++) {
			std::vector<UIItem*> sscs;
			Items[i]->getSubclasses(sscs);
			for (size_t j = 0; j != sscs.size(); j++) {
				scs.push_back(sscs[j]);
			}
		}
	};

	void getImages(std::vector<UIImage*>& images, bool isUI) {
		for (size_t i = 0; i != Items.size(); i++) {
			std::vector<UIImage*> subimages;
			Items[i]->getImages(subimages, isUI);
			for (size_t j = 0; j != subimages.size(); j++) {
				images.push_back(subimages[j]);
			}
		}
	};

	void setVisibility(bool vis) {
		for (UIItem* item : Items) {
			item->setVisibility(vis);
		}
	}

	void setIsEnabled(bool enabled) {
		for (UIItem* item : Items) {
			item->setIsEnabled(enabled);
		}
	}

	bool checkForClickEvent(double mouseX, double mouseY, int eventType) {
		if (!isInArea(mouseX, mouseY)) {
			return false;
		}
		bool found = false;
		for (UIItem* sitem : Items) {
			if (sitem->checkForClickEvent(mouseX, mouseY, eventType)) {
				found = true;
				break;
			};
		}
		return found;
	}

private:
	void setDims(float px, float py, float ex, float ey, float spc) {
		this->posx = px;
		this->posy = -1.0f * py;
		this->anchorx = px;
		this->anchory = py;
		this->extentx = ex;
		this->extenty = ey;
		this->spacing = spc;

		this->sqAxisRatio = ey / ex;
	}

	void getItemProperties(float&, int&, float&, std::vector<float>&);

	void calculateVSpacing(float&, int, float&, float&, float&);
	void calculateVPositions(float, float, float, std::vector<float>, float, float);

	void calculateHSpacing(float&, int, float&, float&, float&);
	void calculateHPositions(float, float, float, std::vector<float>, float);

	int method = ARRANGE_FILL;
	int sizing = SCALE_BY_CONTAINER;
	int orientation = ORIENT_HORIZONTAL;
};

class Grid : public UIItem {
public:
	Grid() = default;

	Grid(int orient, float px, float py, float ex, float ey, float spc) {
		setDims(px, py, ex, ey, spc);
		this->orientation = orient;
	}

	void removeItem(uint32_t index) {
		Items.erase(Items.begin() + index);
		arrangeItems();
	}

	void calculateScreenPosition();

	void updateDisplay();

	void arrangeItems();

	bool isArrangement() {
		return true;
	}

	void getSubclasses(std::vector<UIItem*>& scs) {
		scs.push_back(this);
		for (size_t i = 0; i != Items.size(); i++) {
			std::vector<UIItem*> sscs;
			Items[i]->getSubclasses(sscs);
			for (size_t j = 0; j != sscs.size(); j++) {
				scs.push_back(sscs[j]);
			}
		}
	};

	void getImages(std::vector<UIImage*>& images, bool isUI) {
		for (size_t i = 0; i != Items.size(); i++) {
			std::vector<UIImage*> subimages;
			Items[i]->getImages(subimages, isUI);
			for (size_t j = 0; j != subimages.size(); j++) {
				images.push_back(subimages[j]);
			}
		}
	};

	void setVisibility(bool vis) {
		for (UIItem* item : Items) {
			item->setVisibility(vis);
		}
	}

	void setIsEnabled(bool enabled) {
		for (UIItem* item : Items) {
			item->setIsEnabled(enabled);
		}
	}

	bool checkForClickEvent(double mouseX, double mouseY, int eventType) {
		if (!isInArea(mouseX, mouseY)) {
			return false;
		}
		bool found = false;
		for (UIItem* sitem : Items) {
			if (sitem->checkForClickEvent(mouseX, mouseY, eventType)) {
				found = true;
				break;
			};
		}
		return found;
	}
private:
	void setDims(float px, float py, float ex, float ey, float spc) {
		this->posx = px;
		this->posy = -1.0f * py;
		this->anchorx = px;
		this->anchory = py;
		this->extentx = ex;
		this->extenty = ey;
		this->spacing = spc;

		this->sqAxisRatio = ey / ex;
	}

	int numArrangements = 2;

	Arrangement* mainArrangement = nullptr;

	float spacing = 0.01f;
	int orientation = ORIENT_HORIZONTAL;
};

class Slider : public UIItem {
public:

	Slider() = default;
	
	Slider(Material* mat, float xp, float yp, float xs, float ys) {
		update(xp, yp, xs, ys);
		
		image = std::make_shared<UIImage>(new UIImage);
		image->mat.emplace_back(mat);

		image->isGray = mat->isUIMat;

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		backgroundImage = std::make_shared<UIImage>(new UIImage);
		backgroundImage->mat.emplace_back(mat);

		backgroundImage->isGray = mat->isUIMat;

		backgroundImage->texWidth = backgroundImage->mat[0]->textures[0]->texWidth;
		backgroundImage->texHeight = backgroundImage->mat[0]->textures[0]->texHeight;
	}

	Slider(int orient, Material* mat, float xp, float yp, float xs, float ys) {
		update(xp, yp, xs, ys);

		image = std::make_shared<UIImage>(new UIImage);
		image->mat.emplace_back(mat);

		image->isGray = mat->isUIMat;

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		backgroundImage = std::make_shared<UIImage>(new UIImage);
		backgroundImage->mat.emplace_back(mat);

		backgroundImage->isGray = mat->isUIMat;

		backgroundImage->texWidth = backgroundImage->mat[0]->textures[0]->texWidth;
		backgroundImage->texHeight = backgroundImage->mat[0]->textures[0]->texHeight;

		this->orientation = orient;
	}

	void setSlideValues(int min, int max, int position) {
		// Use integer slider values
		valueType = SLIDER_DISCRETE;
		minValue = static_cast<float>(min);
		maxValue = static_cast<float>(max);

		slideValue = (static_cast<float>(position) - minValue) / (maxValue - minValue);
	}

	void setSlideValues(float min, float max, float position) {
		valueType = SLIDER_CONTINUOUS;
		minValue = min;
		maxValue = max;

		slideValue = (position - minValue) / (maxValue - minValue);
	}

	void setIntCallback(std::function<void(int)> function, bool onUpdate) {
		valueType = SLIDER_DISCRETE;
		floatCallback = nullptr;
		intCallback = function;
		updateOnMove = onUpdate;
		hasCallback = true;
	}

	void setFloatCallback(std::function<void(float)> function, bool onUpdate) {
		valueType = SLIDER_CONTINUOUS;
		intCallback = nullptr;
		floatCallback = function;
		updateOnMove = onUpdate;
		hasCallback = true;
	}

	void updateDisplay() {
		this->calculateScreenPosition();
		updateDisplayOnly();
	}

	void updateDisplayOnly() {
		switch (orientation) {
		case (ORIENT_HORIZONTAL):
			if (image != nullptr) {
				image->UpdateVertices((posx - extentx) + (2 * extentx * slideValue), posy, extenty*sliderWidth, extenty);
			}
			if (backgroundImage != nullptr) {
				backgroundImage->UpdateVertices(posx, posy, extentx, extenty * baseHeight);
			}
			break;
		case (ORIENT_VERTICAL):
			if (image != nullptr) {
				image->UpdateVertices(posx, (posy + extenty) - (2 * extenty * slideValue), extentx, extentx*sliderWidth);
			}
			if (backgroundImage != nullptr) {
				backgroundImage->UpdateVertices(posx, posy, extentx * baseHeight, extenty);
			}
			break;
		default:
			if (image != nullptr) {
				image->UpdateVertices((posx - extentx) + (2 * extentx * slideValue), posy, extenty*sliderWidth, extenty);
			}
			if (backgroundImage != nullptr) {
				backgroundImage->UpdateVertices(posx, posy, extentx, extenty * baseHeight);
			}
			break;
		}
	}

	void calculateScreenPosition();

	void calculateSlideValue(double, double);

	bool checkForClickEvent(double mouseX, double mouseY, int eventType) {
		if (isInArea(mouseX, mouseY) && eventType == LMB_PRESS) {
			isHeld = true;
			return true;
		}
		else if (eventType == LMB_RELEASE && isHeld) {
			isHeld = false;
			this->calculateScreenPosition();
			if (hasCallback && !updateOnMove) {
				switch (valueType) {
				case (SLIDER_CONTINUOUS):
					floatCallback(slideValue * (maxValue - minValue) + minValue);
					break;
				case (SLIDER_DISCRETE):
					intCallback(static_cast<int>(slideValue * (maxValue - minValue) + minValue));
					break;
				default:
					floatCallback(slideValue * (maxValue - minValue) + minValue);
					break;
				}
			}
			return true;
		}
		else {
			return false;
		}
	}

	bool checkForPosEvent(double mouseX, double mouseY, int eventType) {
		if (eventType == LMB_HOLD && isHeld) {
			calculateSlideValue(mouseX, mouseY);
			updateDisplayOnly();
			if (hasCallback && updateOnMove) {
				switch (valueType) {
				case (SLIDER_CONTINUOUS):
					floatCallback(slideValue * (maxValue - minValue) + minValue);
					break;
				case (SLIDER_DISCRETE):
					intCallback(static_cast<int>(slideValue * (maxValue - minValue) + minValue));
					break;
				default:
					floatCallback(slideValue * (maxValue - minValue) + minValue);
					break;
				}
			}
			return true;
		}
		return false;
	};

	void drawUI(VkCommandBuffer commandBuffer, uint32_t currentFrame) {
		if (image->isGray) {
			image->draw(commandBuffer, currentFrame);
		}
		if (backgroundImage->isGray) {
			backgroundImage->draw(commandBuffer, currentFrame);
		}
	}

	void drawImages(VkCommandBuffer commandBuffer, uint32_t currentFrame) {
		if (!image->isGray) {
			image->draw(commandBuffer, currentFrame);
		}
		if (!backgroundImage->isGray) {
			backgroundImage->draw(commandBuffer, currentFrame);
		}
	}

	void getImages(std::vector<UIImage*>& images, bool isUI) {
		if (image != nullptr && image->texHeight > 1 && image->isGray == isUI) {
			images.push_back(image.get());
		}
		if (backgroundImage != nullptr && backgroundImage->texHeight > 1 && backgroundImage->isGray == isUI) {
			images.push_back(backgroundImage.get());
		}
	};

	void cleanup() {
		image->cleanup();
		backgroundImage->cleanup();
	}

	void update(float x, float y, float xsize, float ysize) {
		this->posx = x;
		this->posy = y;
		this->anchorx = x;
		this->anchory = y;

		this->extentx = xsize;
		this->extenty = ysize;

		this->sqAxisRatio = ysize / xsize;
	};

private:
	float minValue = 0.0f;
	float maxValue = 1.0f;

	double valuePositions[2] = {};
	
	float slideValue = 0.0f; 

	std::shared_ptr<UIImage> backgroundImage;

	int orientation = ORIENT_HORIZONTAL;
	int valueType = SLIDER_CONTINUOUS;

	float sliderWidth = 0.5f;
	float baseHeight = 0.2f;

	bool isHeld = false;

	std::function<void(int)> intCallback = nullptr;
	std::function<void(float)> floatCallback = nullptr;

	bool hasCallback = false;
	bool updateOnMove = false; // If false we only perform callbacks on release, if true we perform callbacks on every movement
};

class Rotator : public UIItem {
public:

	Rotator() = default;

	Rotator(Material* mat, float xp, float yp, float xs, float ys) {
		update(xp, yp, xs, ys);

		image = std::make_shared<UIImage>(new UIImage);
		image->mat.emplace_back(mat);

		image->isGray = mat->isUIMat;

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;
	}

	void setSlideValues(int min, int max, int position) {
		// Use integer slider values
		valueType = SLIDER_DISCRETE;
		minValue = static_cast<float>(min);
		maxValue = static_cast<float>(max);

		//(1.0f - slideValue)* (maxValue - minValue) + minValue = angle
		// angle-minValue = (1.0f - slideValue);

		slideValue = 1.0f - (static_cast<float>(position) - minValue) / (maxValue - minValue);
	}

	void setSlideValues(float min, float max, float position) {
		valueType = SLIDER_CONTINUOUS;
		minValue = min;
		maxValue = max;

		slideValue = 1.0f - (position - minValue) / (maxValue - minValue);
		//slideValue = std::clamp(slideValue, 0.0f, 1.0f);
	}

	void setIntCallback(std::function<void(int)> function, bool onUpdate) {
		valueType = SLIDER_DISCRETE;
		floatCallback = nullptr;
		intCallback = function;
		updateOnMove = onUpdate;
		hasCallback = true;
	}

	void setFloatCallback(std::function<void(float)> function, bool onUpdate) {
		valueType = SLIDER_CONTINUOUS;
		intCallback = nullptr;
		floatCallback = function;
		updateOnMove = onUpdate;
		hasCallback = true;
	}

	void updateDisplay() {
		W = static_cast<float>(Engine::get()->windowWidth);
		H = static_cast<float>(Engine::get()->windowHeight);
		this->calculateScreenPosition();
		updateDisplayOnly();
	}

	void updateDisplayOnly() {
		if (image != nullptr) {
			calculateRadius();
			float x = radius * cos(OPF_PI - 2 * PI * slideValue) + this->posx;
			float y = radius * sin(OPF_PI - 2 * PI * slideValue) + this->posy;
			image->UpdateVertices(x, y, sliderWidth, sliderWidth * W / H);
		}
	}

	void calculateScreenPosition();

	void calculateSlideValue(double, double);

	float getValue() {
		return (1.0f - slideValue) * (maxValue - minValue) + minValue;
	}

	bool checkForClickEvent(double mouseX, double mouseY, int eventType) {
		if (isInArea(mouseX, mouseY) && eventType == LMB_PRESS) {
			isHeld = true;
			return true;
		}
		else if (eventType == LMB_RELEASE && isHeld) {
			isHeld = false;
			this->calculateScreenPosition();
			if (hasCallback && !updateOnMove) {
				switch (valueType) {
				case (SLIDER_CONTINUOUS):
					floatCallback((1.0f-slideValue) * (maxValue - minValue) + minValue);
					break;
				case (SLIDER_DISCRETE):
					intCallback(static_cast<int>((1.0f-slideValue) * (maxValue - minValue) + minValue));
					break;
				default:
					floatCallback((1.0f-slideValue) * (maxValue - minValue) + minValue);
					break;
				}
			}
			return true;
		}
		else {
			return false;
		}
	}

	bool checkForPosEvent(double mouseX, double mouseY, int eventType) {
		if (eventType == LMB_HOLD && isHeld) {
			calculateSlideValue(mouseX, mouseY);
			updateDisplayOnly();
			if (hasCallback && updateOnMove) {
				switch (valueType) {
				case (SLIDER_CONTINUOUS):
					floatCallback((1.0f-slideValue) * (maxValue - minValue) + minValue);
					break;
				case (SLIDER_DISCRETE):
					intCallback(static_cast<int>((1.0f-slideValue) * (maxValue - minValue) + minValue));
					break;
				default:
					floatCallback((1.0f-slideValue) * (maxValue - minValue) + minValue);
					break;
				}
			}
			return true;
		}
		return false;
	};

	void drawUI(VkCommandBuffer commandBuffer, uint32_t currentFrame) {
		if (image->isGray) {
			image->draw(commandBuffer, currentFrame);
		}
	}

	void drawImages(VkCommandBuffer commandBuffer, uint32_t currentFrame) {
		if (!image->isGray) {
			image->draw(commandBuffer, currentFrame);
		}
	}

	void cleanup() {
		image->cleanup();
	}

	void update(float x, float y, float xsize, float ysize) {
		this->posx = x;
		this->posy = y;
		this->anchorx = x;
		this->anchory = y;

		ysize *= W / H;

		this->extentx = xsize;
		this->extenty = ysize;

		this->a = (xsize > ysize) ? xsize : ysize;
		this->b = (xsize > ysize) ? ysize : xsize;

		this->e = sqrtf(1 - (b / a) * (b / a));

		this->sqAxisRatio = ysize / xsize;
	};

private:
	float minValue = 0.0f;
	float maxValue = 1.0f;

	float slideValue = 0.0f;

	float centroid[2] = {};

	int valueType = SLIDER_CONTINUOUS;

	float sliderWidth = 0.05f;
	float radius = 100.0f;

	float a, b, e = 0.0f;

	float W = static_cast<float>(Engine::get()->windowWidth);
	float H = static_cast<float>(Engine::get()->windowHeight);

	void calculateRadius() {
		float theta = 0.0f; // Theta is measured from the major axis, not the vertical, whereas slideValue is measured from the vertical
		theta = (a == extenty) ? 2 * PI * slideValue : HALF_PI - 2 * PI * slideValue;

		radius = b / sqrtf(1 - (e * cos(theta)) * (e * cos(theta)));
	}

	bool isHeld = false;

	std::function<void(int)> intCallback = nullptr;
	std::function<void(float)> floatCallback = nullptr;

	bool hasCallback = false;
	bool updateOnMove = false; // If false we only perform callbacks on release, if true we perform callbacks on every movement
};

struct Widget {
	// Individual widgets should be classes with their own setup scripts, functions etc. which are called in the application with a standard constructor
	// UI is managed based on pointers, but the widget must explicitly manage the resources so that we don't have any memory leaks

	Widget() = default;
	
	Widget(LoadList* ll) {
		loadList = ll;
	}

	std::function<bool(double, double, int)> getClickCallback() {
		measureWindowPositions();
		return std::bind(&Widget::checkForClickEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	}

	std::function<bool(double, double, int)> getPosCallback() {
		measureWindowPositions();
		return std::bind(&Widget::checkForPosEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	}

	void measureWindowPositions() {
		windowPositions[0] = 10000.0f;
		windowPositions[1] = 0.0f;
		windowPositions[2] = 10000.0f;
		windowPositions[3] = 0.0f;
		for (UIItem* item : canvas) {
			std::vector<UIItem*> scs;
			item->getSubclasses(scs);
			for (UIItem* sitem : scs) {
				sitem->calculateScreenPosition();
				windowPositions[0] = (sitem->windowPositions[0] < windowPositions[0]) ? sitem->windowPositions[0] : windowPositions[0];
				windowPositions[1] = (sitem->windowPositions[1] > windowPositions[1]) ? sitem->windowPositions[1] : windowPositions[1];
				windowPositions[2] = (sitem->windowPositions[2] < windowPositions[2]) ? sitem->windowPositions[2] : windowPositions[2];
				windowPositions[3] = (sitem->windowPositions[3] > windowPositions[3]) ? sitem->windowPositions[3] : windowPositions[3];
			}
		}
	}

	bool isInArea(double x, double y) {
		bool result = false;
		if (x >= windowPositions[0] && x <= windowPositions[1] && y >= windowPositions[2] && y <= windowPositions[3]) {
			result = true;
		}
		return result;
	};
	
	virtual void drawUI(VkCommandBuffer commandBuffer, uint32_t currentFrame) {
		for (size_t i = 0; i != canvas.size(); i++) {
			canvas[i]->drawUI(commandBuffer, currentFrame);
		}
	}

	virtual void drawImages(VkCommandBuffer commandBuffer, uint32_t currentFrame) {
		for (size_t i = 0; i != canvas.size(); i++) {
			canvas[i]->drawImages(commandBuffer, currentFrame);
		}
	}

	bool checkForClickEvent(double mouseX, double mouseY, int eventType) {
		if (!isInArea(mouseX, mouseY) && Sliders.size() == 0 && Rotators.size() == 0) {
			return false;
		}
		for (UIItem* item : canvas) {
			if (item->checkForClickEvent(mouseX, mouseY, eventType)) {
				return true;
				break;
			};
		}
		return false;
	}

	bool checkForPosEvent(double mouseX, double mouseY, int eventType) {
		for (UIItem* item : canvas) {
			std::vector<UIItem*> scs;
			item->getSubclasses(scs);
			for (UIItem* sitem : scs) {
				if (sitem->checkForPosEvent(mouseX, mouseY, eventType)) {
					return true;
					break;
				};
			}
		}
		return false;
	}

	void update() {
		for (size_t i = 0; i != canvas.size(); i++) {
			canvas[i]->updateDisplay();
		}
		measureWindowPositions();
		customUpdate();
	}

	virtual void cleanupSubClasses() {
	}

	virtual void customUpdate() {
	}

	void cleanup() {
		cleanupSubClasses();
		canvas.clear();
		for (size_t i = 0; i != imagePanels.size(); i++) {
			imagePanels[i]->cleanup();
		}
		imagePanels.clear();
		for (size_t i = 0; i != buttons.size(); i++) {
			buttons[i]->cleanup();
		}
		buttons.clear();
		for (size_t i = 0; i != checkboxes.size(); i++) {
			checkboxes[i]->cleanup();
		}
		checkboxes.clear();
		spacers.clear(); // spacers should have essentially no info so we can just delete them
		for (size_t i = 0; i != Arrangements.size(); i++) {
			Arrangements[i]->Items.clear();
			Arrangements[i]->cleanup();
		}
		Arrangements.clear();
		for (size_t i = 0; i != Sliders.size(); i++) {
			Sliders[i]->cleanup();
		}
		Sliders.clear();
		for (size_t i = 0; i != Rotators.size(); i++) {
			Rotators[i]->cleanup();
		}
		Rotators.clear();
	}

	void hide() {
		isVisible = false;
		for (size_t i = 0; i != canvas.size(); i++) {
			canvas[i]->setVisibility(false);
			canvas[i]->setIsEnabled(false);
		}
	}

	void show() {
		isVisible = true;
		for (size_t i = 0; i != canvas.size(); i++) {
			canvas[i]->setVisibility(true);
			canvas[i]->setIsEnabled(true);
		}
	}

	Material* newMaterial(imageData* imgData, std::string name) {
		if (loadList->checkForMaterial(name+"Mat")) {
			return loadList->findMatPtr(name + "Mat");
		}
		else {
			Texture* tex = nullptr;
			if (loadList->checkForTexture(name + "Tex")) {
				tex = loadList->findTexPtr(name + "Tex");
			}
			else {
				tex = loadList->getPtr(new imageTexture(imgData), name + "Tex");
			}
			return loadList->getPtr(new Material(tex), name + "Mat");
		}
	}

	UIItem* getPtr(ImagePanel* ip) {
		imagePanels.emplace_back(ip);
		return imagePanels[imagePanels.size()-1].get();
	}

	UIItem* getPtr(Button* b) {
		buttons.emplace_back(b);
		return buttons[buttons.size() - 1].get();
	}

	UIItem* getPtr(Checkbox* c) {
		checkboxes.emplace_back(c);
		return checkboxes[checkboxes.size() - 1].get();
	}

	UIItem* getPtr(spacer* s) {
		spacers.emplace_back(s);
		return spacers[spacers.size() - 1].get();
	}

	UIItem* getPtr(Arrangement* a) {
		Arrangements.emplace_back(a);
		return Arrangements[Arrangements.size() - 1].get();
	}

	UIItem* getPtr(Grid* grid) {
		Grids.emplace_back(grid);
		return Grids[Grids.size() - 1].get();
	}

	UIItem* getPtr(Slider* slider) {
		Sliders.emplace_back(slider);
		return Sliders[Sliders.size() - 1].get();
	}

	UIItem* getPtr(Rotator* rotator) {
		Rotators.emplace_back(rotator);
		return Rotators[Rotators.size() - 1].get();
	}

	std::vector<UIItem*> canvas;
	bool isSetup = false;

	LoadList* loadList = nullptr;

	int priorityLayer = 0; // Widgets are sorted in descending order, so smaller numbers will be checked later than higher ones
						   // A widget can be placed above another simply by setting the priority of the "on-top" layer higher than the below one
	bool isVisible = true;
private:
	float windowPositions[4] = { 0.0f };
	// Array of pointers which manages the actual structure of the UI

	// Widgets own all UI classes which appear in the UI, although widget functions use only pointers
	std::vector<std::shared_ptr<ImagePanel>> imagePanels;
	std::vector<std::shared_ptr<Button>> buttons;
	std::vector<std::shared_ptr<Checkbox>> checkboxes;
	std::vector<std::shared_ptr<spacer>> spacers;
	std::vector<std::shared_ptr<Arrangement>> Arrangements; 
	std::vector<std::shared_ptr<Grid>> Grids;
	std::vector<std::shared_ptr<Slider>> Sliders;
	std::vector<std::shared_ptr<Rotator>> Rotators;
};

#endif
