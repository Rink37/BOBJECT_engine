#ifndef UI_ELEMENTS
#define UI_ELEMENTS

#include"Bobject_Engine.h"
#include"Textures.h"
#include"Materials.h"
#include"Meshes.h"
#include"TextManager.h"
#include"InputManager.h"
#include<iostream>
#include<vector>
#include<array>
#include<chrono>
#include<cmath>

#include"LoadLists.h"
#include"include/ImageDataType.h"

enum ArrangeType {
	ARRANGE_FILL = 0,
	ARRANGE_START = 1,
	ARRANGE_CENTER = 2,
	ARRANGE_END = 3
};

enum SizeType {
	SCALE_BY_CONTAINER = 0,
	SCALE_BY_DIMENSIONS = 1,
	SCALE_BY_IMAGES = 2
};

enum Orientation {
	ORIENT_VERTICAL = 0,
	ORIENT_HORIZONTAL = 1
};

enum SliderType {
	SLIDER_CONTINUOUS = 0,
	SLIDER_DISCRETE = 1
};

enum UIItemType {
	UNDEFINED = 0,
	IMAGEPANEL = 1,
	BUTTON = 2,
	CHECKBOX = 3,
	SPACER = 4,
	ARRANGEMENT = 5,
	GRID = 6,
	SLIDER = 7,
	ROTATOR = 8,
	TEXTBOX = 9,
	DROPDOWN = 10,
	BACKGROUND = 11
};

#define PI 3.14159265f
#define OPF_PI 4.71238898f 
#define HALF_PI 1.57079632f

struct UIImage {
	float zp = 0.0f;
	float zp_default = 0.0f;

	std::vector<Material*> mat;
	Mesh* mesh = nullptr;

	uint16_t texHeight = 0;
	uint16_t texWidth = 0;
	uint8_t matidx = 0;
	bool isVisible = true;
	bool isOpaque = true;

	void UpdateVertices(float, float, float, float);

	void cleanup() {
		if (mesh != nullptr) {
			mesh->cleanup();
			delete mesh;
			mesh = nullptr;
		}
	}

	VkCommandBuffer draw(VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t pipelineIndex) {
		if (!isVisible || mat[matidx]->pipelineLayoutIndex != pipelineIndex) {
			return commandBuffer;
		}

		Engine::get()->drawObject(commandBuffer, mesh->vertexBuffer, mesh->indexBuffer, Engine::get()->defaultPass.pipelineLayouts[mat[matidx]->pipelineLayoutIndex], mat[matidx]->descriptorSets[currentFrame], static_cast<uint32_t>(mesh->indices.size()));

		return commandBuffer;
	}

	~UIImage() {
		cleanup();
	}
};

struct UIItem {
	float posx, posy = 0.0f;
	float extentx, extenty = 1.0f;
	float anchorx, anchory = 0.0f;

	float baseExtentx, baseExtenty, baseSqAxisRatio = 1.0f;

	float windowPositions[4] = { 0.0f };

	float sqAxisRatio = 0.0f; // The ratio between axes if the window was perfectly square

	std::string Name = "Unlabelled";
	std::string text = "";

	UIImage* image = nullptr;

	std::vector<UIItem*> Items; // These items are managed by owning widgets, pointers reference a vector of objects

	virtual void addItem(UIItem*);

	bool isEnabled = true;
	bool activestate = false;

	uint8_t clickType = LMB_PRESS;
	uint8_t posType = MOUSE_HOVER;

	UIItemType itemType = UNDEFINED;

	std::function<void(UIItem*)> clickFunction = nullptr;

	void setClickFunction(std::function<void(UIItem*)> func, uint8_t code = LMB_PRESS) {
		clickFunction = func;
		clickType = code;
	}

	virtual void update(float x, float y, float xsize, float ysize) {
		this->posx = x;
		this->posy = y;

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

	virtual void setHeight(float z) {
		if (image != nullptr) {
			image->zp_default = z;
			image->zp = z;
		}
	}

	virtual void addText(std::string text) {
		return;
	}

	virtual void setDims(float px, float py, float ex, float ey) {
		this->posx = px;
		this->posy = -1.0f * py;
		this->anchorx = this->posx;
		this->anchory = this->posy;
		this->extentx = ex;
		this->extenty = ey;

		this->sqAxisRatio = ey / ex;

		baseExtentx = extentx;
		baseExtenty = extenty;
		baseSqAxisRatio = sqAxisRatio;
	}

	virtual void updateArrangedPosition(float px, float py, float ex, float ey) {
		this->posx = px;
		this->posy = py;
		this->anchorx = this->posx;
		this->anchory = this->posy;
		this->extentx = ex;
		this->extenty = ey;

		this->sqAxisRatio = ey / ex;
	}

	virtual void updateDisplay() {
		this->calculateScreenPosition();
		if (image != nullptr) {
			image->UpdateVertices(posx, posy, extentx, extenty);
		}
		for (size_t i = 0; i != Items.size(); i++) {
			Items[i]->updateDisplay();
		}
	}

	virtual void getSubclasses(std::vector<UIItem*> &scs) {
		scs.push_back(this);
	};

	virtual void getImages(std::vector<UIImage*>& images) {
		if (image != nullptr && image->texHeight > 1) {
			images.push_back(image);
		}
		for (size_t i = 0; i != Items.size(); i++) {
			std::vector<UIImage*> subimages;
			Items[i]->getImages(subimages);
			for (size_t j = 0; j != subimages.size(); j++) {
				images.push_back(subimages[j]);
			}
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
		if (image != nullptr) {
			image->isVisible = vis;
		}
		setIsEnabled(vis);
	}

	virtual void setIsEnabled(bool enabled) {
		isEnabled = enabled;
	}

	virtual void cleanup() {
		std::vector<UIImage*> images;
		getImages(images);

		for (UIImage* image : images) {
			image->cleanup();
			image = nullptr;
		}
	}

	~UIItem() {
		cleanup();
	}
};

class Background : public UIItem {
public:
	Background(Material* mat, UIItem* par = nullptr) {
		parent = par;
		if (parent != nullptr) {
			setDims(parent->posx, parent->posy * -1.0f, parent->extentx, parent->extenty);
		}

		image = new UIImage;
		image->mesh = new UIMesh;
		image->mat.emplace_back(mat);

		image->texHeight = 100;
		image->texWidth = 100;

		itemType = BACKGROUND;
	}

	void updateDisplay() {
		if (parent != nullptr) {
			setDims(parent->posx, parent->posy * -1.0f, parent->extentx, parent->extenty);
			if (image != nullptr && parent->image != nullptr) {
				image->zp_default = parent->image->zp_default + 0.01f;
			}
		}
		if (image != nullptr) {
			image->UpdateVertices(posx, posy, extentx, extenty);
		}
	}

	void calculateScreenPosition() {
		float W = static_cast<float>(Core::get()->windowWidth);
		float H = static_cast<float>(Core::get()->windowHeight);

		this->windowPositions[0] = (((posx - extentx) / 2.0f) + 0.5f) * W; // left position
		this->windowPositions[1] = (((posx + extentx) / 2.0f) + 0.5f) * W; // right position
		this->windowPositions[2] = (((posy - extenty) / 2.0f) + 0.5f) * H; // top position
		this->windowPositions[3] = (((posy + extenty) / 2.0f) + 0.5f) * H; // bottom position
	}
private:
	UIItem* parent = nullptr;
};

class TextBox : public UIItem {
public:
	TextBox(font* inFont, float px, float py, float ex, float ey, uint32_t fSize = 24, ArrangeType hArrange = ARRANGE_START, ArrangeType vArrange = ARRANGE_START, bool isModifiable = false) {
		setDims(px, py, ex, ey);
		textFont = inFont;
		characterSize = fSize;
		horizontalArrange = hArrange;
		verticalArrange = vArrange;

		modifiable = isModifiable;

		if (modifiable) {
			typeManager = new TypeManager(Engine::get()->window);
			typeManager->setTypeCallback(std::bind(&TextBox::updateText, this, std::placeholders::_1));
		}

		image = new UIImage;

		image->mesh = new fontMesh;
		image->mat.emplace_back(inFont->fontMat);
		image->isVisible = true;
		image->texWidth = 2;
		image->texHeight = 2;
		image->isOpaque = false;
		image->zp_default = 0.05f;

		itemType = TEXTBOX;
	}

	bool isModifiable() const {
		return modifiable;
	}

	void updateText(char c) {
		if (c == 3) {
			removeLastChar();
		}
		else if (c == 1) {
			typeManager->stopListening();
			if (clickFunction != nullptr) {
				clickFunction(this);
			}
		}
		else {
			createCharacter(c);
		}
		updateDisplay();
	}

	void updateDisplay();

	void calculateScreenPosition();

	void getVertices(char, std::vector<Vertex>&, float xp, float yp, float xsc, float windowRatio, float zp) const;

	void createCharacter(int unicodeCharacter) {
		characters.push_back(unicodeCharacter);
		text += unicodeCharacter;
	}

	void removeLastChar() {
		vkDeviceWaitIdle(Engine::get()->device);
		uint32_t index = static_cast<uint32_t>(characters.size()) - 1;
		characters.erase(characters.begin() + index);
		text.erase(text.begin() + index);
	}

	void clearText() {
		vkDeviceWaitIdle(Engine::get()->device);
		characters.clear();
		text = "";
		updateDisplay();
	}

	void addText(std::string text) {
		for (int i = 0; i != text.size(); i++) {
			createCharacter(text[i]);
		}
		updateDisplay();
	}

	void setVisibility(bool vis) {
		setVisibility(vis);
		setIsEnabled(vis);
	}

	void cleanup() {
		if (typeManager != nullptr) {
			typeManager->~TypeManager();
		}
		std::vector<UIImage*> images;
		getImages(images);
		for (UIImage* image : images) {
			image->cleanup();
			image = nullptr;
		}
		characters.clear();
	}

	std::vector<char> characters{};
	font* textFont;

	bool checkForClickEvent(double mousex, double mousey, int clicktype) {
		if (!isEnabled) {
			return false;
		}
		if (!modifiable) {
			if (clickFunction != nullptr && clicktype == LMB_PRESS && isInArea(mousex, mousey)) {
				clickFunction(this);
				return true;
			}
			return false;
		}
		else {
			if (clicktype == LMB_PRESS && isInArea(mousex, mousey)) {
				typeManager->startListening();
				return true;
			}
		}
		return false;
	};

private:
	TypeManager* typeManager = nullptr;

	ArrangeType horizontalArrange = ARRANGE_START;
	ArrangeType verticalArrange = ARRANGE_START;
	
	uint8_t characterSize = 24; // Size in pixels
	bool modifiable = false;
};

class ImagePanel : public UIItem {
public:
	float UVextentx = 0.0f;
	bool isWebcam;

	ImagePanel(Material* surf, bool iW) {
		setDims(0.0f, 0.0f, 1.0f, 1.0f);

		image = new UIImage;
		image->mesh = new UIMesh;
		image->mat.emplace_back(surf);

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

		this->extenty = this->extentx * this->sqAxisRatio;

		baseExtentx = extentx;
		baseExtenty = extenty;
		baseSqAxisRatio = sqAxisRatio;

		this->isWebcam = iW;

		itemType = IMAGEPANEL;
	}
	
	ImagePanel(float x, float y, float xsize, float ysize, Material* surf, bool iW) {
		setDims(x, y, xsize, ysize);

		image = new UIImage;
		image->mesh = new UIMesh;
		image->mat.emplace_back(surf);

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = ysize / xsize;

		this->isWebcam = iW;

		itemType = IMAGEPANEL;
	}

	void updateDisplay() {
		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

		this->extenty = this->extentx * this->sqAxisRatio;

		UVextentx = extentx;
		baseExtenty = baseExtentx * sqAxisRatio;
		baseSqAxisRatio = sqAxisRatio;

		if (this->extenty > this->extentx) {
			float sf = this->extentx / this->extenty;
			this->extenty = this->extentx;
			baseExtenty = baseExtentx;
			baseSqAxisRatio = 1.0f;
			this->sqAxisRatio = 1.0f;
			this->calculateScreenPosition();
			UVextentx = extentx * sf;
			if (image != nullptr) {
				image->UpdateVertices(posx, posy, UVextentx, extenty);
			}
		}
		else {
			this->calculateScreenPosition();
			if (image != nullptr) {
				image->UpdateVertices(posx, posy, extentx, extenty);
			}
		}
	}

	ImagePanel() {
		itemType = IMAGEPANEL;
		this->isWebcam = false;
	}

	void setDims(float px, float py, float ex, float ey) {
		this->posx = px;
		this->posy = -1.0f * py;
		this->anchorx = this->posx;
		this->anchory = this->posy;
		this->extentx = ex;
		this->extenty = ey;

		this->sqAxisRatio = ey / ex;

		baseExtentx = extentx;
		baseExtenty = extenty;
		baseSqAxisRatio = sqAxisRatio;
	}
};

class Button : public UIItem // Here a button is just a rectangle area in screen space which can be queried with coordinates to check if it has been pressed
{
public:
	Button() {
		itemType = BUTTON;
	}

	Button(Material* mat, std::function<void(UIItem*)> func = nullptr) {

		image = new UIImage;
		image->mesh = new UIMesh;
		image->mat.push_back(mat);

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

		clickFunction = func;

		setDims(0.0f, 0.0f, 1.0f, 1.0f * this->sqAxisRatio);

		baseExtentx = extentx;
		baseExtenty = extenty;
		baseSqAxisRatio = sqAxisRatio;

		itemType = BUTTON;
	}

	Button(font* inFont, Material* bgMat, std::string text, std::function<void(UIItem*)> func) {
		addBackground(bgMat);
		
		TextBox* tb = new TextBox(inFont, 0.0f, 0.0f, 5.0f, 1.0f, 18, ARRANGE_CENTER, ARRANGE_CENTER);
		tb->addText(text);
		addItem(tb);

		clickFunction = func;

		setDims(0.0f, 0.0f, 5.0f, 1.0f);

		this->sqAxisRatio = 0.2f;

		baseExtentx = extentx;
		baseExtenty = extenty;
		baseSqAxisRatio = sqAxisRatio;

		itemType = BUTTON;
	}

	void updateDisplay() {
		this->calculateScreenPosition();
		if (image != nullptr) {
			image->UpdateVertices(posx, posy, extentx, extenty);
		}
		for (size_t i = 0; i != Items.size(); i++) {
			Items[i]->updateArrangedPosition(posx, posy, extentx, extenty);
			Items[i]->updateDisplay();
		}
	}

	void addBackground(Material* mat) {
		this->addItem(new Background(mat, this));
	}

	bool checkForClickEvent(double mousex, double mousey, int eventType) {
		if (!isEnabled) {
			return false;
		}
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
	Checkbox() {
		itemType = CHECKBOX;
	}

	Checkbox(Material* onMat, Material* offMat, std::function<void(UIItem*)> func = nullptr, bool state = true) {
		
		image = new UIImage;
		image->mesh = new UIMesh;
		image->mat.push_back(onMat);
		image->mat.push_back(offMat);
		if (state) {
			image->matidx = 0;
		}
		else {
			image->matidx = 1;
		}
		this->activestate = state;

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

		update(0.0f, 0.0f, 1.0f, 1.0f * sqAxisRatio);

		clickFunction = func;

		baseExtentx = extentx;
		baseExtenty = extenty;
		baseSqAxisRatio = sqAxisRatio;

		itemType = CHECKBOX;
	}

	bool checkForClickEvent(double mousex, double mousey, int eventType) {
		if (!isEnabled) {
			return false;
		}
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
	spacer() {
		itemType = SPACER;
	}

	bool isSpacer() {
		return true;
	}

	void update(float x, float y, float xsize, float ysize) {};

	void updateDisplay() {};

	void getImages(std::vector<UIImage*>& images) {};

	void arrangeItems() {};

	bool checkForClickEvent(double, double, int) {
		return false;
	};

	bool checkForPosEvent(double, double, int) {
		return false;
	};

	void calculateScreenPosition() {};

	void setVisibility(bool) {};
};

class FreeArrangement : public UIItem {
public:
	bool isArrangement() {
		return true;
	}

	void calculateScreenPosition() {};

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

	void getImages(std::vector<UIImage*>& images) {
		for (size_t i = 0; i != Items.size(); i++) {
			std::vector<UIImage*> subimages;
			Items[i]->getImages(subimages);
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
		if (!isEnabled) {
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
};

class Arrangement : public UIItem {
public:
	float spacing;
	bool expanding = true;

	Arrangement(Orientation orient, float px, float py, float ex, float ey, float spc, ArrangeType arrangeMethod = ARRANGE_START, SizeType sizeMethod = SCALE_BY_CONTAINER) {
		setDims(px, py, ex, ey, spc);
		this->sizing = sizeMethod;
		this->method = arrangeMethod;
		this->orientation = orient;
		itemType = ARRANGEMENT;
	}

	void setSizeMethod(SizeType sizeMethod) {
		this->sizing = sizeMethod;
	}

	void setArrangeMethod(ArrangeType arrangeMethod) {
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

	void setHeight(float z) {
		for (UIItem* item : Items) {
			setHeight(z);
		}
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

	void getImages(std::vector<UIImage*>& images) {
		for (size_t i = 0; i != Items.size(); i++) {
			std::vector<UIImage*> subimages;
			Items[i]->getImages(subimages);
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
		if (!isEnabled) {
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
		this->anchorx = this->posx;
		this->anchory = this->posy;
		this->extentx = ex;
		this->extenty = ey;
		this->spacing = spc;

		this->sqAxisRatio = ey / ex;

		baseExtentx = extentx;
		baseExtenty = extenty;
		baseSqAxisRatio = sqAxisRatio;
	}

	void getItemProperties(float&, int&, float&, std::vector<float>&);

	void calculateVSpacing(float&, int, float&, float&, float&);
	void calculateVPositions(float, float, float, std::vector<float>, float, float);

	void calculateHSpacing(float&, int, float&, float&, float&);
	void calculateHPositions(float, float, float, std::vector<float>, float);

	ArrangeType method = ARRANGE_FILL;
	SizeType sizing = SCALE_BY_CONTAINER;
	Orientation orientation = ORIENT_HORIZONTAL;
};

class Grid : public UIItem {
public:
	Grid() {
		itemType = GRID;
	}

	Grid(Orientation orient, float px, float py, float ex, float ey, float spc) {
		setDims(px, py, ex, ey, spc);
		this->orientation = orient;

		itemType = GRID;
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

	void getImages(std::vector<UIImage*>& images) {
		for (size_t i = 0; i != Items.size(); i++) {
			std::vector<UIImage*> subimages;
			Items[i]->getImages(subimages);
			for (size_t j = 0; j != subimages.size(); j++) {
				images.push_back(subimages[j]);
			}
		}
	};

	void setHeight(float z) {
		for (UIItem* item : Items) {
			item->setHeight(z);
		}
	}

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
		if (!isEnabled) {
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

	void setDims(float px, float py, float ex, float ey) {
		this->posx = px;
		this->posy = -1.0f * py;
		this->anchorx = this->posx;
		this->anchory = this->posy;
		this->extentx = ex;
		this->extenty = ey;

		this->sqAxisRatio = ey / ex;

		baseExtentx = extentx;
		baseExtenty = extenty;
		baseSqAxisRatio = sqAxisRatio;
	}

private:
	void setDims(float px, float py, float ex, float ey, float spc) {
		this->posx = px;
		this->posy = -1.0f * py;
		this->anchorx = this->posx;
		this->anchory = this->posy;
		this->extentx = ex;
		this->extenty = ey;
		this->spacing = spc;

		this->sqAxisRatio = ey / ex;

		baseExtentx = extentx;
		baseExtenty = extenty;
		baseSqAxisRatio = sqAxisRatio;
	}

	int numArrangements = 2;

	Arrangement* mainArrangement = nullptr;

	float spacing = 0.01f;
	Orientation orientation = ORIENT_HORIZONTAL;
};

class Slider : public UIItem {
public:
	Slider() {
		itemType = SLIDER;
	}

	Slider(Material* mat, float xp, float yp, float xs, float ys, Orientation orient = ORIENT_HORIZONTAL) {
		setDims(xp, yp, xs, ys);

		image = new UIImage;
		image->mesh = new UIMesh;
		image->mat.emplace_back(mat);

		backgroundImage = new UIImage;
		backgroundImage->mesh = new UIMesh;
		backgroundImage->mat.emplace_back(mat);

		if (mat->textures.size() > 0) {
			image->texWidth = image->mat[0]->textures[0]->texWidth;
			image->texHeight = image->mat[0]->textures[0]->texHeight;
			backgroundImage->texWidth = backgroundImage->mat[0]->textures[0]->texWidth;
			backgroundImage->texHeight = backgroundImage->mat[0]->textures[0]->texHeight;
		}
		else {
			image->texWidth = 256;
			image->texHeight = 256;
			backgroundImage->texWidth = 256;
			backgroundImage->texHeight = 256;
		}

		this->orientation = orient;

		baseExtentx = extentx;
		baseExtenty = extenty;
		baseSqAxisRatio = sqAxisRatio;

		itemType = SLIDER;
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
		if (!isEnabled) {
			return false;
		}
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
		if (!isEnabled) {
			return false;
		}
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

	void getImages(std::vector<UIImage*>& images) {
		if (image != nullptr && image->texHeight > 1) {
			images.push_back(image);
		}
		if (backgroundImage != nullptr && backgroundImage->texHeight > 1) {
			images.push_back(backgroundImage);
		}
	};

	void cleanup() {
		image->cleanup();
		backgroundImage->cleanup();
	}

	void update(float x, float y, float xsize, float ysize) {
		this->posx = x;
		this->posy = y;

		this->extentx = xsize;
		this->extenty = ysize;

		this->sqAxisRatio = ysize / xsize;
	};

private:
	float minValue = 0.0f;
	float maxValue = 1.0f;

	double valuePositions[2] = {};

	UIImage* backgroundImage = nullptr;

	Orientation orientation = ORIENT_HORIZONTAL;
	SliderType valueType = SLIDER_CONTINUOUS;

	float sliderWidth = 0.5f;
	float baseHeight = 0.2f;

	std::function<void(int)> intCallback = nullptr;
	std::function<void(float)> floatCallback = nullptr;

	float slideValue = 0.0f;
	bool isHeld = false;
	bool hasCallback = false;
	bool updateOnMove = false; // If false we only perform callbacks on release, if true we perform callbacks on every movement
};

class Rotator : public UIItem {
public:
	Rotator() {
		itemType = ROTATOR;
	}

	Rotator(Material* mat, float xp, float yp, float xs, float ys) {
		setDims(xp, yp, xs, ys);

		image = new UIImage;
		image->mesh = new UIMesh;
		image->mat.emplace_back(mat);

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		baseExtentx = extentx;
		baseExtenty = extenty;
		baseSqAxisRatio = sqAxisRatio;

		itemType = ROTATOR;
	}

	void setSlideValues(int min, int max, int position) {
		// Use integer slider values
		valueType = SLIDER_DISCRETE;
		minValue = static_cast<float>(min);
		maxValue = static_cast<float>(max);

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
		W = static_cast<float>(Core::get()->windowWidth);
		H = static_cast<float>(Core::get()->windowHeight);
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

	float getValue() const {
		return (1.0f - slideValue) * (maxValue - minValue) + minValue;
	}

	bool checkForClickEvent(double mouseX, double mouseY, int eventType) {
		if (!isEnabled) {
			return false;
		}
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
		if (!isEnabled) {
			return false;
		}
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

	void cleanup() {
		image->cleanup();
	}

	void update(float x, float y, float xsize, float ysize) {
		this->posx = x;
		this->posy = y;

		ysize *= W / H;

		this->extentx = xsize;
		this->extenty = ysize;

		this->a = (xsize > ysize) ? xsize : ysize;
		this->b = (xsize > ysize) ? ysize : xsize;

		this->e = sqrtf(1 - (b / a) * (b / a));

		this->sqAxisRatio = ysize / xsize;
	};

	void updateArrangedPosition(float x, float y, float xsize, float ysize) {
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
	std::function<void(int)> intCallback = nullptr;
	std::function<void(float)> floatCallback = nullptr;

	float centroid[2] = {};
	float minValue = 0.0f;
	float maxValue = 1.0f;
	float slideValue = 0.0f;

	float sliderWidth = 0.05f;
	float radius = 100.0f;

	float a = 0.0f, b = 0.0f, e = 0.0f;

	float W = static_cast<float>(Core::get()->windowWidth);
	float H = static_cast<float>(Core::get()->windowHeight);

	SliderType valueType = SLIDER_CONTINUOUS;

	void calculateRadius() {
		float theta = 0.0f; // Theta is measured from the major axis, not the vertical, whereas slideValue is measured from the vertical
		theta = (a == extenty) ? 2 * PI * slideValue : HALF_PI - 2 * PI * slideValue;

		radius = b / sqrtf(1 - (e * cos(theta)) * (e * cos(theta)));
	}

	bool isHeld = false;
	bool hasCallback = false;
	bool updateOnMove = false; // If false we only perform callbacks on release, if true we perform callbacks on every movement
};

class DropdownMenu : public UIItem {
public:
	DropdownMenu(float x, float y, float xsize, float ysize, Material* dropButtonMat, Material* raiseButtonMat, Material* bg,  font* inFont) {
		setDims(x, y, xsize, ysize);
		Arrangement* mainArranger = new Arrangement(ORIENT_HORIZONTAL, x, y, xsize, ysize, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);
		selectedTextBox = new TextBox(inFont, 0.0f, 0.0f, (xsize * 0.8f) / ysize, 1.0f, characterSize, ARRANGE_START, ARRANGE_CENTER);
		mainArranger->addItem(selectedTextBox);
		
		bgMat = bg;

		std::function<void(UIItem*)> dropdownFunc = std::bind(&DropdownMenu::optionsToggle, this, std::placeholders::_1);

		mainArranger->addItem(new spacer());
		mainArranger->addItem(new Checkbox(dropButtonMat, raiseButtonMat, dropdownFunc));
		mainArranger->arrangeItems();
		
		addItem(mainArranger);
		textFont = inFont;

		image = new UIImage;
		image->mesh = new UIMesh;
		image->isVisible = false;
		image->texWidth = 2;
		image->texHeight = 2;
		image->isOpaque = false;
		image->zp_default = 0.05f;
		image->zp = 0.05f;

		itemType = DROPDOWN;
	}

	void setBlankText(std::string string) {
		optionIndex = -1;
		selectedTextBox->clearText();
		selectedTextBox->addText(string);
		this->text = "";

		arrangeItems();
		updateDisplay();
	}

	void setOptionIndex(int index) {
		if (index >= options.size()) {
			//std::cout << "Invalid option index" << std::endl;
			return;
		}
		optionIndex = index;
		selectedTextBox->clearText();
		selectedTextBox->addText(options[index]);
		this->text = options[index];

		arrangeItems();
		updateDisplay();
	}

	void execCallback() {
		if (selectCallback != nullptr) {
			selectCallback(this);
		}
	}

	void addOptions(std::vector<std::string> inOptions) {
		options.insert(options.end(), inOptions.begin(), inOptions.end());
	}

	void addOption(std::string inOption) {
		options.push_back(inOption);
	}

	void setSelectCallback(std::function<void(UIItem*)> cFunct) {
		selectCallback = cFunct;
	}

	void setVisibility(bool vis) {
		for (UIItem* item : Items) {
			item->setVisibility(vis);
		}
		if (vis) {
			updateDisplay();
		}
	}

	void getImages(std::vector<UIImage*>& images) {
		images.push_back(image);
		for (size_t i = 0; i != Items.size(); i++) {
			std::vector<UIImage*> subimages;
			Items[i]->getImages(subimages);
			for (size_t j = 0; j != subimages.size(); j++) {
				images.push_back(subimages[j]);
			}
		}
	};

	void updateDisplay() {
		this->calculateScreenPosition();
		for (UIItem* item : Items) {
			item->updateDisplay();
		}
		if (optionsArrangement != nullptr) {
			float W = static_cast<float>(Core::get()->windowWidth);
			float H = static_cast<float>(Core::get()->windowHeight);

			float boxHeight = options.size() * (characterSize) / H;

			optionsArrangement->updateArrangedPosition(posx - 0.2f * extentx, posy + boxHeight + extenty, extentx * 0.8f, boxHeight);
		}
	}

	void arrangeItems() {
		for (UIItem* item : Items) {
			item->arrangeItems();
		}
	};

	void updateArrangedPosition(float px, float py, float ex, float ey) {
		this->posx = px;
		this->posy = py;
		this->anchorx = this->posx;
		this->anchory = this->posy;
		this->extentx = ex;
		this->extenty = ey;

		this->sqAxisRatio = ey / ex;

		Items[0]->updateArrangedPosition(px, py, ex, ey);
		Items[0]->arrangeItems();
	}

	void calculateScreenPosition();

	bool isArranger() {
		return true;
	}

	bool isText() {
		return true;
	}

	bool checkForClickEvent(double mousex, double mousey, int clickType) {
		if (!isEnabled) {
			return false;
		}
		bool event = false;
		for (UIItem* item : Items) {
			event = item->checkForClickEvent(mousex, mousey, clickType);
			if (event) {
				return event;
			}
		}
		return event;
	};

	void cleanup() {
		for (UIItem* item : Items) {
			item->cleanup();
		}
	}

	std::vector<std::string> options{};

private:
	uint8_t characterSize = 18;

	TextBox* selectedTextBox = nullptr;

	Arrangement* optionsArrangement = nullptr;
	Background* background = nullptr;

	font* textFont = nullptr;

	Material* bgMat = nullptr;

	int optionIndex = -1;

	std::function<void(UIItem*)> selectCallback = nullptr;

	void optionSelect(UIItem* owner) {
		int optionIndex = stoi(owner->Name);
		setOptionIndex(optionIndex);
		closeOptions(owner);
		if (selectCallback != nullptr) {
			selectCallback(this);
		}
	}

	void optionsToggle(UIItem* owner) {
		if (!owner->activestate) {
			openOptions(owner);
		}
		else {
			closeOptions(owner);
		}
	}

	void openOptions(UIItem* owner) {
		float W = static_cast<float>(Core::get()->windowWidth);
		float H = static_cast<float>(Core::get()->windowHeight);

		float boxHeight = options.size() * (characterSize) / H; // The box size in pixels
		optionsArrangement = new Arrangement(ORIENT_VERTICAL, posx - 0.2f * extentx, -(posy + boxHeight + extenty), extentx * 0.8f, boxHeight, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		std::function<void(UIItem*)> optionSelectFunct = std::bind(&DropdownMenu::optionSelect, this, std::placeholders::_1);

		uint32_t optionIndex = 0;
		for (std::string option : options) {
			TextBox* optionText = new TextBox(textFont, 0.0f, 0.0f, (extentx * 0.8f), characterSize / H, characterSize, ARRANGE_START, ARRANGE_START);
			optionText->setHeight(image->zp_default + 0.021f);
			optionText->addText(option);
			optionText->Name = std::to_string(optionIndex);
			optionText->setClickFunction(optionSelectFunct);
			optionsArrangement->addItem(optionText);
			optionIndex++;
		}
		optionsArrangement->arrangeItems();

		addItem(optionsArrangement);
		if (bgMat != nullptr) {
			background = new Background(bgMat, optionsArrangement);
			background->setHeight(image->zp_default + 0.011f);
			background->updateDisplay();
			addItem(background);
		}
	}

	void closeOptions(UIItem* owner) {
		vkDeviceWaitIdle(Engine::get()->device);
		if (optionsArrangement != nullptr) {
			optionsArrangement->cleanup();
			delete optionsArrangement;
			optionsArrangement = nullptr;
			if (background != nullptr) {
				background->cleanup();
				delete background;
				background = nullptr;
			}
			Items.erase(Items.begin() + Items.size() - 1);
			Items.erase(Items.begin() + Items.size() - 1);
			Items[0]->Items[2]->activestate = true;
			Items[0]->Items[2]->image->matidx = 0;
		}
	}
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

	void measureWindowPositions();

	bool isInArea(double x, double y) const {
		bool result = false;
		if (x >= windowPositions[0] && x <= windowPositions[1] && y >= windowPositions[2] && y <= windowPositions[3]) {
			result = true;
		}
		return result;
	};

	void addBackground(Material* mat) {
		Background* bg = new Background(mat);
		bg->setHeight(this->zp - 0.01f);
		thisBG = getPtr(bg);
		measureWindowPositions();
		thisBG->updateDisplay();
	}

	virtual void drawAll(VkCommandBuffer commandBuffer, uint32_t currentFrame, GraphicsPass* currentPass) {
		if (!isVisible) {
			return;
		}
		if (imagePipelines.empty()) {
			update();
		}
		for (std::pair<std::string, std::vector<UIImage*>> elem : imagePipelines) {
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *currentPass->GraphicsPipelines[currentPass->pipelineMap.at(elem.first)]);
			for (UIImage* image : elem.second) {
				image->draw(commandBuffer, currentFrame, image->mat[image->matidx]->pipelineLayoutIndex);
			}
		}
		if (transparentImages.size() > 0) {
			for (uint32_t i = 0; i != transparentImages.size(); i++) {
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *currentPass->GraphicsPipelines[currentPass->pipelineMap.at(transparentImagePipelines[i])]);
				transparentImages[i]->draw(commandBuffer, currentFrame, transparentImages[i]->mat[transparentImages[i]->matidx]->pipelineLayoutIndex);
			}
		}
	}

	bool checkForClickEvent(double, double, int);

	bool checkForPosEvent(double, double, int);

	void update() {
		if (markedForDestruction) {
			markedForDestruction = false;
			deleteSelf(this->mappedName);
			return;
		}
		if (isVisible && isSetup) {
			sortImages();
			canvas->updateDisplay();
			measureWindowPositions();
			if (thisBG != nullptr) {
				float W = static_cast<float>(Core::get()->windowWidth);
				float H = static_cast<float>(Core::get()->windowHeight);
				
				float posx = (windowPositions[1] / W + windowPositions[0] / W) - 1.0f;
				float extentx = (windowPositions[1] / W - 0.5f) * 2.0f - posx + 0.02f;
				float posy = (windowPositions[3] / H + windowPositions[2] / H) - 1.0f;
				float extenty = (windowPositions[3] / H - 0.5f) * 2.0f - posy + 0.02f;

				thisBG->updateArrangedPosition(posx, posy, extentx, extenty);
				thisBG->updateDisplay();
			}
			customUpdate();
		}
	}

	virtual void cleanupSubClasses() {
	}

	virtual void customUpdate() {
	}

	void cleanup() {
		cleanupSubClasses();
		//canvas.clear();
		imagePipelines.clear();
		thisBG = nullptr;
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
		spacers.clear();
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
		for (size_t i = 0; i != TextBoxes.size(); i++) {
			TextBoxes[i]->cleanup();
		}
		TextBoxes.clear();
		for (size_t i = 0; i != DropdownMenus.size(); i++) {
			DropdownMenus[i]->cleanup();
		}
		DropdownMenus.clear();
		for (size_t i = 0; i != Backgrounds.size(); i++) {
			Backgrounds[i]->cleanup();
		}
		Backgrounds.clear();
		for (size_t i = 0; i != freeArrangements.size(); i++) {
			freeArrangements[i]->cleanup();
		}
		freeArrangements.clear();
		isSetup = false;
		//canvas->cleanup();
		//delete canvas;
	}

	void hide() {
		isVisible = false;
	}

	void show() {
		isVisible = true;
	}

	UIItem* getPtr(FreeArrangement* fa) {
		freeArrangements.emplace_back(fa);
		return freeArrangements[freeArrangements.size() - 1].get();
	}

	UIItem* getPtr(ImagePanel* ip) {
		imagePanels.emplace_back(ip);
		return imagePanels[imagePanels.size()-1].get();
	}

	UIItem* getPtr(Button* b) {
		hasClickEvents = true;
		buttons.emplace_back(b);
		return buttons[buttons.size() - 1].get();
	}

	UIItem* getPtr(Checkbox* c) {
		hasClickEvents = true;
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
		hasPosEvents = true;
		Sliders.emplace_back(slider);
		return Sliders[Sliders.size() - 1].get();
	}

	UIItem* getPtr(Rotator* rotator) {
		hasPosEvents = true;
		Rotators.emplace_back(rotator);
		return Rotators[Rotators.size() - 1].get();
	}

	UIItem* getPtr(TextBox* textBox) {
		if (textBox->isModifiable()) {
			hasClickEvents = true;
		}
		TextBoxes.emplace_back(textBox);
		return TextBoxes[TextBoxes.size() - 1].get();
	}

	UIItem* getPtr(DropdownMenu* dropdown) {
		hasClickEvents = true;
		DropdownMenus.emplace_back(dropdown);
		return DropdownMenus[DropdownMenus.size() - 1].get();
	}

	UIItem* getPtr(Background* background) {
		Backgrounds.emplace_back(background);
		return Backgrounds[Backgrounds.size() - 1].get();
	}

	void setErrorFunc(std::function<void(std::string, std::string)> fnc) {
		errorFunc = fnc;
	}

	void selfDestruct(UIItem* owner) {
		if (deleteSelf != nullptr) {
			markedForDestruction = true;
		}
	}

	void destruct() const {
		if (deleteSelf != nullptr) {
			deleteSelf(mappedName);
		}
	}

	UIItem* canvas = nullptr; // Canvas must now be a single arrangement

	LoadList* loadList = nullptr;

	uint8_t priorityLayer = 0; // Widgets are sorted in descending order, so smaller numbers will be checked later than higher ones
						   // A widget can be placed above another simply by setting the priority of the "on-top" layer higher than the below one
	bool isVisible = true;
	bool hasClickEvents = false;
	bool hasPosEvents = false;
	bool isSetup = false;

	uint8_t clickIndex = 0;
	uint8_t posIndex = 0;
	bool markedForDestruction = false;

	// UIItem* mainArrangement = nullptr;

	std::map<std::string, std::vector<UIImage*>> imagePipelines{};
	std::vector<UIImage*> transparentImages{};
	std::vector<std::string> transparentImagePipelines{};

	float posx = 0.0f, posy = 0.0f, extentx = 0.0f, extenty = 0.0f, zp = 0.003f;

	std::string mappedName = "";
	std::function<void(std::string)> deleteSelf = nullptr;
	std::function<void(std::string, std::string)> errorFunc = nullptr;
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
	std::vector<std::shared_ptr<TextBox>> TextBoxes;
	std::vector<std::shared_ptr<DropdownMenu>> DropdownMenus;
	std::vector<std::shared_ptr<Background>> Backgrounds;
	std::vector<std::shared_ptr<FreeArrangement>> freeArrangements;

	UIItem* thisBG = nullptr;

	virtual void sortImages();
};

#endif
