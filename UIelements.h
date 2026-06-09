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

#define SPACE_CHAR 32
#define TAB_CHAR 9
#define NEWLINE_CHAR 10

#define PI 3.14159265f
#define OPF_PI 4.71238898f 
#define HALF_PI 1.57079632f

struct UIImage {
	bool isVisible = true;

	int texHeight = 0;
	int texWidth = 0;

	float zp = 0.0f;
	float zp_default = 0.0f;

	std::vector<Material*> mat;
	uint32_t matidx = 0;
	
	UIMesh mesh;

	uint32_t mipLevels = 0;

	//bool isGray = true;

	bool isOpaque = true;

	void UpdateVertices(float, float, float, float);

	void cleanup() {
		mesh.cleanup();
	}

	VkCommandBuffer draw(VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t pipelineIndex) {
		if (!isVisible || mat[matidx]->pipelineLayoutIndex != pipelineIndex) {
			return commandBuffer;
		}

		Engine::get()->drawObject(commandBuffer, mesh.vertexBuffer, mesh.indexBuffer, Engine::get()->defaultPass.pipelineLayouts[mat[matidx]->pipelineLayoutIndex], mat[matidx]->descriptorSets[currentFrame], static_cast<uint32_t>(mesh.indices.size()));

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

	float baseExtentx, baseExtenty, baseSqAxisRatio = 1.0f;

	float windowPositions[4] = { 0.0f };

	float sqAxisRatio = 0.0f; // The ratio between axes if the window was perfectly square

	std::string Name = "Unlabelled";
	std::string text = "";

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

	//virtual float getHeight() {
	//	if (image != nullptr) {
	//		return image->zp;
	//	}
	//	return 0.0f;
	//}

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
	}

	virtual void getSubclasses(std::vector<UIItem*> &scs) {
		scs.push_back(this);
	};

	virtual void getImages(std::vector<UIImage*>& images) {
		if (image != nullptr && image->texHeight > 1) {
			images.push_back(image.get());
		}
	};

	virtual void getText(std::vector<UIItem*>& textboxes) {
		if (isText()) {
			textboxes.push_back(this);
		}
	}

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

	virtual bool isText() {
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

	//virtual void drawUI(VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t pipelineIndex) {
	//	std::vector<UIImage*> images;
	//	getImages(images);

	//	for (UIImage* image : images) {
	//		image->draw(commandBuffer, currentFrame, pipelineIndex);
	//	}
	//}

	//virtual void drawImages(VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t pipelineIndex) {
	//	std::vector<UIImage*> images;
	//	getImages(images);

	//	for (UIImage* image : images) {
	//		image->draw(commandBuffer, currentFrame, pipelineIndex);
	//	}
	//}

	virtual void cleanup() {
		std::vector<UIImage*> images;
		getImages(images);

		std::vector<UIItem*> textboxes;
		getText(textboxes);

		for (UIImage* image : images) {
			image->cleanup();
			image = nullptr;
		}
		for (UIItem* textbox : textboxes) {
			textbox->cleanup();
		}
	}

	virtual void addOption(std::string) {
		return;
	}

	virtual void setOptionIndex(int) {
		return;
	}
};

class TextBox : public UIItem {
public:
	TextBox(font* inFont, float px, float py, float ex, float ey, uint32_t fSize = 24, int hArrange = ARRANGE_START, int vArrange = ARRANGE_START, bool isModifiable = false) {
		setDims(px, py, ex, ey);
		textFont = inFont;
		characterSize = fSize;
		horizontalArrange = hArrange;
		verticalArrange = vArrange;

		modifiable = isModifiable;

		if (modifiable) {
			std::cout << "Modifiable text box created" << std::endl;
			typeManager = new TypeManager();
			typeManager->initCallbacks(Engine::get()->window);
			typeManager->setTypeCallback(std::bind(&TextBox::updateText, this, std::placeholders::_1));
		}

		image = std::make_shared<UIImage>(new UIImage);
		image->isVisible = false;
		image->texWidth = 2;
		image->texHeight = 2;
		image->isOpaque = false;
		image->zp_default = 0.05f;
	}

	void updateText(char c) {
		//std::cout << static_cast<int>(c) << std::endl;
		if (c == 3) {
			removeLastChar();
		}
		else if (c == 1) {
			typeManager->stopListening();
			if (clickFunct != nullptr) {
				clickFunct(this);
			}
		}
		else {
			createCharacter(c);
		}
		updateDisplay();
	}

	void updateDisplay();

	void calculateScreenPosition();

	bool isArrangement() {
		return true;
	}

	bool isText() {
		return true;
	}

	void createCharacter(int unicodeCharacter) {
		fontMesh* newMesh = nullptr;
		switch (unicodeCharacter) {
		case (SPACE_CHAR):
			newMesh = new fontMesh(33, textFont);
			newMesh->isVisible = false;
			newMesh->unicodeCharacter = SPACE_CHAR;
			characters.push_back(newMesh);
			break;
		case (TAB_CHAR):
			newMesh = new fontMesh(33, textFont);
			newMesh->isVisible = false;
			newMesh->unicodeCharacter = TAB_CHAR;
			newMesh->advanceWidth *= 6.0f;
			characters.push_back(newMesh);
			break;
		case (NEWLINE_CHAR):
			newMesh = new fontMesh(33, textFont);
			newMesh->isVisible = false;
			newMesh->unicodeCharacter = NEWLINE_CHAR;
			newMesh->advanceWidth = 0.0f;
			characters.push_back(newMesh);
			break;
		default:
			fontMesh* newMesh = new fontMesh(unicodeCharacter, textFont);
			characters.push_back(newMesh);
			break;
		}
		text += static_cast<char>(unicodeCharacter);
	}

	void removeLastChar() {
		vkDeviceWaitIdle(Engine::get()->device);
		uint32_t index = characters.size() - 1;
		characters[index]->cleanup();
		delete characters[index];
		characters.erase(characters.begin() + index);
		text.erase(text.begin() + index);
	}

	void clearText() {
		vkDeviceWaitIdle(Engine::get()->device);
		for (fontMesh* mesh : characters) {
			mesh->cleanup();
			delete mesh;
		}
		characters.clear();
		text = "";
	}

	void addText(std::string text) {
		for (int i = 0; i != text.size(); i++) {
			createCharacter(text[i]);
		}
		updateDisplay();
	}

	VkCommandBuffer draw(VkCommandBuffer commandBuffer, uint32_t currentFrame) {
		if (!isVisible) {
			return commandBuffer;
		}

		for (fontMesh* mesh : characters) {
			if (mesh->isVisible) {
				Engine::get()->drawObject(commandBuffer, mesh->vertexBuffer, mesh->indexBuffer, Engine::get()->defaultPass.pipelineLayouts[Engine::get()->defaultPass.layoutMap.at("1_0_")], textFont->fontMat->descriptorSets[currentFrame], static_cast<uint32_t>(mesh->indices.size()));
			}
		}

		return commandBuffer;
	}

	void setVisibility(bool vis) {
		for (fontMesh* mesh : characters) {
			mesh->isVisible = vis;
		}
		setIsEnabled(vis);
	}

	void cleanup() {
		if (typeManager != nullptr) {
			typeManager->~TypeManager();
		}
		for (fontMesh* mesh : characters) {
			mesh->cleanup();
		}
		characters.clear();
	}

	bool isVisible = true;

	std::vector<fontMesh*> characters;
	font* textFont;

	uint32_t characterSize = 24; // Size in pixels

	void setClickFunction(std::function<void(UIItem*)> func) {
		clickFunct = func;
	}

	bool checkForClickEvent(double mousex, double mousey, int clicktype) {
		if (!isEnabled) {
			return false;
		}
		if (!modifiable) {
			if (clickFunct != nullptr && clicktype == LMB_PRESS && isInArea(mousex, mousey)) {
				clickFunct(this);
				return true;
			}
			return false;
		}
		else {
			//std::cout << "Checking modifiable box" << std::endl;
			if (clicktype == LMB_PRESS && isInArea(mousex, mousey)) {
				std::cout << "Try typing" << std::endl;
				typeManager->startListening();
			}
		}
	};

private:
	bool modifiable = false;

	TypeManager* typeManager = nullptr;

	int horizontalArrange = ARRANGE_START;
	int verticalArrange = ARRANGE_START;

	std::function<void(UIItem*)> clickFunct = nullptr;
};

class ImagePanel : public UIItem {
// Represents only a webcam view
public:
	bool isWebcam;

	ImagePanel(Material* surf, bool iW) {
		setDims(0.0f, 0.0f, 1.0f, 1.0f);

		image = std::make_shared<UIImage>(new UIImage);
		image->mat.emplace_back(surf);

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

		this->extenty = this->extentx * this->sqAxisRatio;

		baseExtentx = extentx;
		baseExtenty = extenty;
		baseSqAxisRatio = sqAxisRatio;

		this->isWebcam = iW;
	}
	
	ImagePanel(float x, float y, float xsize, float ysize, Material* surf, bool iW) {
		setDims(x, y, xsize, ysize);

		image = std::make_shared<UIImage>(new UIImage);
		image->mat.emplace_back(surf);

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = ysize / xsize;

		this->isWebcam = iW;
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

	ImagePanel() = default;

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

	float UVextentx = 0.0f;
};

class Button : public UIItem // Here a button is just a rectangle area in screen space which can be queried with coordinates to check if it has been pressed
{
public:
	std::function<void(UIItem*)> clickFunction = nullptr;

	Button() = default;

	Button(Material* mat, std::function<void(UIItem*)> func) {

		image = std::make_shared<UIImage>(new UIImage);
		image->mat.push_back(mat);

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

		clickFunction = func;

		setDims(0.0f, 0.0f, 1.0f, 1.0f * this->sqAxisRatio);

		baseExtentx = extentx;
		baseExtenty = extenty;
		baseSqAxisRatio = sqAxisRatio;
	}

	Button(Material* mat, std::function<void(UIItem*)> func, int code) {

		image = std::make_shared<UIImage>(new UIImage);
		image->mat.push_back(mat);

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

		clickFunction = func;
		clickType = code;

		setDims(0.0f, 0.0f, 1.0f, 1.0f * this->sqAxisRatio);

		baseExtentx = extentx;
		baseExtenty = extenty;
		baseSqAxisRatio = sqAxisRatio;
	}

	Button(Material* mat) {

		image = std::make_shared<UIImage>(new UIImage);
		image->mat.push_back(mat);

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

		setDims(0.0f, 0.0f, 1.0f, 1.0f * this->sqAxisRatio);

		baseExtentx = extentx;
		baseExtenty = extenty;
		baseSqAxisRatio = sqAxisRatio;
	}

	void setClickFunction(std::function<void(UIItem*)> func) {
		clickFunction = func;
	}

	void setClickFunction(std::function<void(UIItem*)> func, int code) {
		clickFunction = func;
		clickType = code;
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
	std::function<void(UIItem*)> clickFunction = nullptr;

	Checkbox() = default;

	Checkbox(Material* onMat, Material* offMat) {

		image = std::unique_ptr<UIImage>(new UIImage);
		image->mat.push_back(onMat);
		image->mat.push_back(offMat);
		image->matidx = 0;
		this->activestate = true;

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

		update(0.0f, 0.0f, 1.0f, 1.0f * sqAxisRatio);

		baseExtentx = extentx;
		baseExtenty = extenty;
		baseSqAxisRatio = sqAxisRatio;
	}

	Checkbox(Material* onMat, Material* offMat, std::function<void(UIItem*)> func) {
		
		image = std::unique_ptr<UIImage>(new UIImage);
		image->mat.push_back(onMat);
		image->mat.push_back(offMat);
		image->matidx = 0;
		this->activestate = true;

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

		update(0.0f, 0.0f, 1.0f, 1.0f * sqAxisRatio);

		clickFunction = func;

		baseExtentx = extentx;
		baseExtenty = extenty;
		baseSqAxisRatio = sqAxisRatio;
	}

	Checkbox(Material* onMat, Material* offMat, std::function<void(UIItem*)> func, int code) {

		image = std::unique_ptr<UIImage>(new UIImage);
		image->mat.push_back(onMat);
		image->mat.push_back(offMat);
		image->matidx = 0;
		this->activestate = true;

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

		update(0.0f, 0.0f, 1.0f, 1.0f * sqAxisRatio);

		clickFunction = func;
		clickType = code;

		baseExtentx = extentx;
		baseExtenty = extenty;
		baseSqAxisRatio = sqAxisRatio;
	}

	void setClickFunction(std::function<void(UIItem*)> func) {
		clickFunction = func;
	}

	void setClickFunction(std::function<void(UIItem*)> func, int code) {
		clickFunction = func;
		clickType = code;
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

	void getText(std::vector<UIItem*>& textboxes) {
		for (size_t i = 0; i != Items.size(); i++) {
			std::vector<UIItem*> subitems;
			Items[i]->getText(subitems);
			for (size_t j = 0; j != subitems.size(); j++) {
				textboxes.push_back(subitems[j]);
			}
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
		if (!isInArea(mouseX, mouseY) || !isEnabled) {
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

	void getImages(std::vector<UIImage*>& images) {
		for (size_t i = 0; i != Items.size(); i++) {
			std::vector<UIImage*> subimages;
			Items[i]->getImages(subimages);
			for (size_t j = 0; j != subimages.size(); j++) {
				images.push_back(subimages[j]);
			}
		}
	};

	void getText(std::vector<UIItem*>& textboxes) {
		for (size_t i = 0; i != Items.size(); i++) {
			std::vector<UIItem*> subitems;
			Items[i]->getText(subitems);
			for (size_t j = 0; j != subitems.size(); j++) {
				textboxes.push_back(subitems[j]);
			}
		}
	}

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
		if (!isInArea(mouseX, mouseY) || !isEnabled) {
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
	int orientation = ORIENT_HORIZONTAL;
};

class Slider : public UIItem {
public:

	Slider() = default;
	
	Slider(Material* mat, float xp, float yp, float xs, float ys) {
		setDims(xp, yp, xs, ys);
		
		image = std::make_shared<UIImage>(new UIImage);
		image->mat.emplace_back(mat);

		backgroundImage = std::make_shared<UIImage>(new UIImage);
		backgroundImage->mat.emplace_back(mat);

		if (mat->textures.size() > 0) {
			image->texWidth = image->mat[0]->textures[0]->texWidth;
			image->texHeight = image->mat[0]->textures[0]->texHeight;
			backgroundImage->texWidth = backgroundImage->mat[0]->textures[0]->texWidth;
			backgroundImage->texHeight = backgroundImage->mat[0]->textures[0]->texHeight;
		} else {
			image->texWidth = 256;
			image->texHeight = 256;
			backgroundImage->texWidth = 256;
			backgroundImage->texHeight = 256;
		}

		baseExtentx = extentx;
		baseExtenty = extenty;
		baseSqAxisRatio = sqAxisRatio;
	}

	Slider(int orient, Material* mat, float xp, float yp, float xs, float ys) {
		setDims(xp, yp, xs, ys);

		image = std::make_shared<UIImage>(new UIImage);
		image->mat.emplace_back(mat);

		backgroundImage = std::make_shared<UIImage>(new UIImage);
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
			images.push_back(image.get());
		}
		if (backgroundImage != nullptr && backgroundImage->texHeight > 1) {
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
		setDims(xp, yp, xs, ys);

		image = std::make_shared<UIImage>(new UIImage);
		image->mat.emplace_back(mat);

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		baseExtentx = extentx;
		baseExtenty = extenty;
		baseSqAxisRatio = sqAxisRatio;
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
	float minValue = 0.0f;
	float maxValue = 1.0f;

	float slideValue = 0.0f;

	float centroid[2] = {};

	int valueType = SLIDER_CONTINUOUS;

	float sliderWidth = 0.05f;
	float radius = 100.0f;

	float a = 0.0f, b = 0.0f, e = 0.0f;

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

class Background : public UIItem {
public:
	Background(Material* mat, UIItem* par) {
		parent = par;
		setDims(parent->posx, parent->posy * -1.0f, parent->extentx, parent->extenty);

		image = std::make_shared<UIImage>(new UIImage);
		image->mat.emplace_back(mat);

		image->texHeight = 100;
		image->texWidth = 100;
	}

	Background(Material* mat) {
		image = std::make_shared<UIImage>(new UIImage);
		image->mat.emplace_back(mat);

		image->texHeight = 100;
		image->texWidth = 100;
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
		float W = static_cast<float>(Engine::get()->windowWidth);
		float H = static_cast<float>(Engine::get()->windowHeight);

		this->windowPositions[0] = (((posx - extentx) / 2.0f) + 0.5f) * W; // left position
		this->windowPositions[1] = (((posx + extentx) / 2.0f) + 0.5f) * W; // right position
		this->windowPositions[2] = (((posy - extenty) / 2.0f) + 0.5f) * H; // top position
		this->windowPositions[3] = (((posy + extenty) / 2.0f) + 0.5f) * H; // bottom position
	}
private:
	UIItem* parent = nullptr;

	bool isParentWidget = false;
};

class DropdownMenu : public UIItem {
public:
	DropdownMenu(float x, float y, float xsize, float ysize, Material* dropButtonMat, Material* raiseButtonMat, Material* bg,  font* inFont) {
		setDims(x, y, xsize, ysize);
		Arrangement* mainArranger = new Arrangement(ORIENT_HORIZONTAL, x, y, xsize, ysize, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);
		selectedTextBox = new TextBox(inFont, 0.0f, 0.0f, (xsize * 0.8f) / ysize, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
		mainArranger->addItem(selectedTextBox);
		
		bgMat = bg;

		std::function<void(UIItem*)> dropdownFunc = std::bind(&DropdownMenu::optionsToggle, this, std::placeholders::_1);

		mainArranger->addItem(new spacer());
		mainArranger->addItem(new Checkbox(dropButtonMat, raiseButtonMat, dropdownFunc));
		mainArranger->arrangeItems();
		
		addItem(mainArranger);
		textFont = inFont;

		image = std::make_shared<UIImage>(new UIImage);
		image->isVisible = false;
		image->texWidth = 2;
		image->texHeight = 2;
		image->isOpaque = false;
		image->zp_default = 0.05f;
		image->zp = 0.05f;
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
			std::cout << "Invalid option index" << std::endl;
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
		//setOptionIndex(options.size() - 1);
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
		images.push_back(image.get());
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
			float W = static_cast<float>(Engine::get()->windowWidth);
			float H = static_cast<float>(Engine::get()->windowHeight);

			float boxHeight = options.size() * (selectedTextBox->characterSize) / H;

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

	void drawText(VkCommandBuffer commandbuffer, uint32_t currentFrame) {
		selectedTextBox->draw(commandbuffer, currentFrame);
		if (optionsArrangement != nullptr) {
			for (UIItem* item : optionsArrangement->Items) {
				TextBox* itemText = static_cast<TextBox*>(item);
				itemText->draw(commandbuffer, currentFrame);
			}
		}
	}

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

	TextBox* selectedTextBox = nullptr;

	Arrangement* optionsArrangement = nullptr;
	Background* background = nullptr;

	font* textFont = nullptr;

	Material* bgMat = nullptr;

	int optionIndex = -1;

	//bool optionsVisible = false;

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
		std::cout << "Creating options" << std::endl;

		float W = static_cast<float>(Engine::get()->windowWidth);
		float H = static_cast<float>(Engine::get()->windowHeight);

		float boxHeight = options.size() * (selectedTextBox->characterSize) / H; // The box size in pixels
		optionsArrangement = new Arrangement(ORIENT_VERTICAL, posx - 0.2f * extentx, -(posy + boxHeight + extenty), extentx * 0.8f, boxHeight, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		std::function<void(UIItem*)> optionSelectFunct = std::bind(&DropdownMenu::optionSelect, this, std::placeholders::_1);

		uint32_t optionIndex = 0;
		for (std::string option : options) {
			TextBox* optionText = new TextBox(textFont, 0.0f, 0.0f, (extentx * 0.8f), selectedTextBox->characterSize / H, selectedTextBox->characterSize, ARRANGE_START, ARRANGE_START);
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

	void measureWindowPositions() {
		windowPositions[0] = 10000.0f;
		windowPositions[1] = 0.0f;
		windowPositions[2] = 10000.0f;
		windowPositions[3] = 0.0f;
		for (UIItem* item : canvas) {
			std::vector<UIItem*> scs;
			item->getSubclasses(scs);
			for (UIItem* sitem : scs) {
				if (sitem->isSpacer() || (sitem->isArrangement() && !sitem->isText()) || sitem == thisBG) {
					continue;
				}
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

	void addBackground(Material* mat) {
		Background* bg = new Background(mat);
		bg->setHeight(this->zp - 0.01f);
		canvas.push_back(getPtr(bg));
		thisBG = canvas[canvas.size() - 1];
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
		for (auto elem : imagePipelines) {
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
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *currentPass->GraphicsPipelines[currentPass->pipelineMap.at("UIText")]);
		drawText(commandBuffer, currentFrame);
	}

	virtual void drawText(VkCommandBuffer commandBuffer, uint32_t currentFrame) {
		for (size_t i = 0; i != DropdownMenus.size(); i++) {
			DropdownMenus[i]->drawText(commandBuffer, currentFrame);
		}
		for (size_t i = 0; i != TextBoxes.size(); i++) {
			TextBoxes[i]->draw(commandBuffer, currentFrame);
		}
	}

	bool checkForClickEvent(double mouseX, double mouseY, int eventType) {
		if (!isVisible) {
			return false;
		}
		if (!isInArea(mouseX, mouseY) && Sliders.size() == 0 && Rotators.size() == 0) {
			return false;
		}
		for (UIItem* item : canvas) {
			if (item->checkForClickEvent(mouseX, mouseY, eventType)) {
				update();
				return true;
			};
		}
		return false;
	}

	bool checkForPosEvent(double mouseX, double mouseY, int eventType) {
		if (!isVisible) {
			return false;
		}
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
		if (isVisible) {
			sortImages();
			for (size_t i = 0; i != canvas.size(); i++) {
				canvas[i]->updateDisplay();
			}
			measureWindowPositions();
			if (thisBG != nullptr) {
				float W = static_cast<float>(Engine::get()->windowWidth);
				float H = static_cast<float>(Engine::get()->windowHeight);
				
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
		canvas.clear();
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
		isSetup = false;
	}

	void hide() {
		isVisible = false;
	}

	void show() {
		isVisible = true;
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

	UIItem* getPtr(TextBox* textBox) {
		TextBoxes.emplace_back(textBox);
		return TextBoxes[TextBoxes.size() - 1].get();
	}

	UIItem* getPtr(DropdownMenu* dropdown) {
		DropdownMenus.emplace_back(dropdown);
		return DropdownMenus[DropdownMenus.size() - 1].get();
	}

	UIItem* getPtr(Background* background) {
		Backgrounds.emplace_back(background);
		return Backgrounds[Backgrounds.size() - 1].get();
	}

	std::vector<UIItem*> canvas;
	bool isSetup = false;

	LoadList* loadList = nullptr;

	int priorityLayer = 0; // Widgets are sorted in descending order, so smaller numbers will be checked later than higher ones
						   // A widget can be placed above another simply by setting the priority of the "on-top" layer higher than the below one
	bool isVisible = true;

	size_t clickIndex = 0;
	size_t posIndex = 0;

	std::map<std::string, std::vector<UIImage*>> imagePipelines{};
	std::vector<UIImage*> transparentImages{};
	std::vector<std::string> transparentImagePipelines{};

	float posx = 0.0f, posy = 0.0f, extentx = 0.0f, extenty = 0.0f, zp = 0.0f;
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

	UIItem* thisBG = nullptr;

	virtual void sortImages() {
		imagePipelines.clear();
		transparentImages.clear();
		transparentImagePipelines.clear();
		std::map<float, std::vector<UIImage*>, std::greater<float>> imageHeights{};
		std::vector<UIImage*> allImages{};
		for (size_t i = 0; i != canvas.size(); i++) {
			std::vector<UIImage*> images{};
			canvas[i]->getImages(images);
			allImages.insert(allImages.end(), images.begin(), images.end());
		}
		for (UIImage* image : allImages) {
			if (image->isVisible && image->isOpaque) {
				std::string key = image->mat[image->matidx]->shaderName;
				if (imagePipelines.count(key) == 0) {
					imagePipelines.insert({ key, std::vector<UIImage*>{image} });
				}
				else {
					imagePipelines.at(key).push_back(image);
				}
			}
			else if (image->isVisible) {
				// If the image is not opaque then we need to draw them sorted from back to front and associate their pipelines properly
				transparentImages.push_back(image);
			}
			float heightKey = image->zp_default;
			if (imageHeights.count(heightKey) == 0) {
				imageHeights.insert({ heightKey, std::vector<UIImage*>{image} });
			}
			else {
				imageHeights.at(heightKey).push_back(image);
			}
		}

		float separation = 0.01f;
		float pos = 0.0f;
		//std::cout << "Starting sort" << std::endl;
		for (auto elem : imageHeights) {
			//std::cout << elem.first << " " << pos << std::endl;
			for (UIImage* image : elem.second) {
				image->zp = pos;
			}
			pos += separation;
		}

		if (transparentImages.size() > 0) {
			std::sort(transparentImages.begin(), transparentImages.end(), [](UIImage* a, UIImage* b) {return a->zp > b->zp; });
		}
		for (UIImage* transparentImage : transparentImages) {
			transparentImagePipelines.push_back(transparentImage->mat[transparentImage->matidx]->shaderName);
		}
	}
};

#endif
