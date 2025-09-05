#include"Bobject_Engine.h"

#ifndef UI_ELEMENTS
#define UI_ELEMENTS

#include"Textures.h"
#include"Materials.h"
#include"Meshes.h"
#include<iostream>
#include<vector>
#include<array>
#include<chrono>

#include"LoadLists.h"
#include"include/ImageDataType.h"

struct UIImage {
	bool isVisible = true;

	int texHeight = 0;
	int texWidth = 0;

	std::vector<Material*> mat;
	uint32_t matidx = 0;
	
	UIMesh mesh;

	uint32_t mipLevels = 0;

	void UpdateVertices(float, float, float, float);

	void cleanup() {
		//for (Material* indMat : mat) {
		//	indMat->cleanup();
		//}
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

	virtual void getImages(std::vector<UIImage*>& images) {
		if (image != nullptr && image->texHeight > 1) {
			images.push_back(image.get());
		}
	};

	virtual void arrangeItems() {

	};

	virtual void checkForEvent(double, double, int) {

	};

	virtual void calculateScreenPosition();

	virtual bool isSpacer() {
		return false;
	}

	virtual void setVisibility(bool vis) {
		image->isVisible = vis;
	}

	virtual void setIsEnabled(bool enabled) {
		isEnabled = enabled;
	}

	virtual void draw(VkCommandBuffer commandBuffer, uint32_t currentFrame) {
		std::vector<UIImage*> images;
		getImages(images);

		for (UIImage* image : images) {
			image->draw(commandBuffer, currentFrame);
		}
	}

	virtual void cleanup() {
		std::vector<UIImage*> images;
		getImages(images);

		for (UIImage* image : images) {
			//for (Material* mat : image->mat) {
			//	if (mat != nullptr && !mat->cleaned) {
			//		mat->cleanup();
			//		mat = nullptr;
			//	}
			//}
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

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

		this->isWebcam = iW;
	}
	
	ImagePanel(float x, float y, float xsize, float ysize, Material* surf, bool iW) {
		update(x, -1.0f * y, xsize, ysize);

		image = std::make_shared<UIImage>(new UIImage);
		image->mat.emplace_back(surf);

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = ysize / xsize;

		this->isWebcam = iW;
	}

	ImagePanel() = default;

	//void cleanup() {
	//	std::vector<UIImage*> images;
	//	getImages(images);

	//	for (UIImage* image : images) {
	//		image->mesh.cleanup();
	//	}
	//}
};

class Button : public UIItem // Here a button is just a rectangle area in screen space which can be queried with coordinates to check if it has been pressed
{
public:
	std::function<void(UIItem*)> clickFunction = nullptr;
	//std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

	Button() = default;

	Button(Material* mat, std::function<void(UIItem*)> func) {
		image = std::make_shared<UIImage>(new UIImage);
		image->mat.push_back(mat);

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

		clickFunction = func;

		update(0.0f, 0.0f, 1.0f, 1.0f * this->sqAxisRatio);
	}

	Button(Material* mat) {
		image = std::make_shared<UIImage>(new UIImage);
		image->mat.push_back(mat);

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

		update(0.0f, 0.0f, 1.0f, 1.0f * this->sqAxisRatio);
	}
	
	//Button(float x, float y, float xsize, float ysize, imageData* iDpointer) {
	//	this->sqAxisRatio = ysize / xsize;

	//	Texture* tex = new imageTexture(iDpointer);

	//	image = std::make_shared<UIImage>(new UIImage);
	//	image->mat.push_back(new Material(tex));
		
	//	image->texWidth = image->mat[0]->textures[0]->texWidth;
	//	image->texHeight = image->mat[0]->textures[0]->texHeight;

	//	update(x, -1.0f * y, xsize, ysize);
	//};

	//Button(imageData* iDpointer) {
	//	Texture* tex = new imageTexture(iDpointer);

	//	image = std::make_shared<UIImage>(new UIImage);
	//	image->mat.push_back(new Material(tex));

	//	image->texWidth = image->mat[0]->textures[0]->texWidth;
	//	image->texHeight = image->mat[0]->textures[0]->texHeight;

	//	this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

	//	update(0.0f, 0.0f, 1.0f, 1.0f * this->sqAxisRatio);
	//}

	//Button(imageData* iDpointer, std::function<void(UIItem*)> func) {
	//	Texture* tex = new imageTexture(iDpointer);

	//	image = std::make_shared<UIImage>(new UIImage);
	//	image->mat.push_back(new Material(tex));

	//	image->texWidth = image->mat[0]->textures[0]->texWidth;
	//	image->texHeight = image->mat[0]->textures[0]->texHeight;

	//	this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

	//	clickFunction = func;

	//	update(0.0f, 0.0f, 1.0f, 1.0f * this->sqAxisRatio);
	//}



	bool isInArea(double x, double y) {
		bool result = false;
		if (x >= windowPositions[0] && x <= windowPositions[1] && y >= windowPositions[2] && y <= windowPositions[3]) {
			result = true;
		}
		return result;
	};

	void setClickFunction(std::function<void(UIItem*)> func) {
		clickFunction = func;
	}

	void checkForEvent(double mousex, double mousey, int state) {
		if (clickFunction != nullptr && isEnabled) {
			if (isInArea(mousex, mousey)) {
				clickFunction(this);
			}
		}
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

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

		update(0.0f, 0.0f, 1.0f, 1.0f * sqAxisRatio);

		clickFunction = func;
	}

	//Checkbox(float x, float y, float xsize, float ysize, imageData* iDon, imageData* iDoff) {
	//	this->sqAxisRatio = ysize / xsize;

	//	Texture* onTex = new imageTexture(iDon);
	//	Texture* offTex = new imageTexture(iDoff);

	//	image = std::unique_ptr<UIImage>(new UIImage);
	//	image->mat.push_back(new Material(onTex));
	//	image->mat.push_back(new Material(offTex));
	//	image->matidx = 0;
	//	this->activestate = true;

	//	image->texWidth = image->mat[0]->textures[0]->texWidth;
	//	image->texHeight = image->mat[0]->textures[0]->texHeight;

	//	update(x, -1.0f * y, xsize, ysize);
	//};

	//Checkbox(imageData* iDon, imageData* iDoff) {
	//	Texture* onTex = new imageTexture(iDon);
	//	Texture* offTex = new imageTexture(iDoff);

	//	image = std::unique_ptr<UIImage>(new UIImage);
	//	image->mat.push_back(new Material(onTex));
	//	image->mat.push_back(new Material(offTex));
	//	image->matidx = 0;
	//	this->activestate = true;

	//	image->texWidth = image->mat[0]->textures[0]->texWidth;
	//	image->texHeight = image->mat[0]->textures[0]->texHeight;

	//	this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

	//	update(0.0f, 0.0f, 1.0f, 1.0f * sqAxisRatio);
	//}

	//Checkbox(imageData* iDon, imageData* iDoff, std::function<void(UIItem*)> func) {
	//	Texture* onTex = new imageTexture(iDon);
	//	Texture* offTex = new imageTexture(iDoff);

	//	image = std::unique_ptr<UIImage>(new UIImage);
	//	image->mat.push_back(new Material(onTex));
	//	image->mat.push_back(new Material(offTex));
	//	image->matidx = 0;
	//	this->activestate = true;

	//	image->texWidth = image->mat[0]->textures[0]->texWidth;
	//	image->texHeight = image->mat[0]->textures[0]->texHeight;

	//	this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);

	//	update(0.0f, 0.0f, 1.0f, 1.0f * sqAxisRatio);

	//	clickFunction = func;
	//}

	bool isInArea(double x, double y) {
		bool result = false;
		if (x >= windowPositions[0] && x <= windowPositions[1] && y >= windowPositions[2] && y <= windowPositions[3]) {
			result = true;
		}
		return result;
	};

	void setClickFunction(std::function<void(UIItem*)> func) {
		clickFunction = func;
	}

	void checkForEvent(double mousex, double mousey, int state) {
		bool check = isInArea(mousex, mousey);
		if (check && state == 1 && isEnabled) {
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

class hArrangement : public UIItem {
public:
	float spacing;
	
	hArrangement() = default;

	hArrangement(float px, float py, float ex, float ey, float spc) {
		this->posx = px;
		this->posy = -1.0f * py;
		this->anchorx = px;
		this->anchory = py;
		this->extentx = ex;
		this->extenty = ey;
		this->spacing = spc;

		this->sqAxisRatio = ey / ex;
	}

	void calculateScreenPosition();

	void removeItem(uint32_t index) {
		Items.erase(Items.begin() + index);
		arrangeItems();
	}

	void updateDisplay();

	void arrangeItems(); // Should be called any time there is a layout change

	void getSubclasses(std::vector<UIItem*> &scs) {
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

private:

	int method = 0;
};

class vArrangement : public UIItem{
public:
	float spacing;

	vArrangement() = default;

	vArrangement(float px, float py, float ex, float ey, float spc) {
		posx = px;
		posy = -1.0f * py;
		anchorx = px;
		anchory = py;
		extentx = ex;
		extenty = ey;
		spacing = spc;

		this->sqAxisRatio = ey / ex;
	}

	void calculateScreenPosition();

	void updateDisplay();

	void arrangeItems(); // Should be called any time there is a layout change

	void removeItem(uint32_t index) {
		Items.erase(Items.begin() + index);
		arrangeItems();
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

private:

	int method = 0;
};

struct Widget {
	// Individual widgets should be classes with their own setup scripts, functions etc. which are called in the application with a standard constructor
	// UI is managed based on pointers, but the widget must explicitly manage the resources so that we don't have any memory leaks

	void draw(VkCommandBuffer commandBuffer, uint32_t currentFrame) {
		for (size_t i = 0; i != canvas.size(); i++) {
			canvas[i]->draw(commandBuffer, currentFrame);
		}
	}

	void checkForEvent(double mouseX, double mouseY, bool state) {
		for (UIItem* item : canvas) {
			std::vector<UIItem*> scs;
			item->getSubclasses(scs);
			for (UIItem* sitem : scs) {
				sitem->checkForEvent(mouseX, mouseY, state);
			}
		}
	}

	void update() {
		for (size_t i = 0; i != canvas.size(); i++) {
			canvas[i]->updateDisplay();
		}
	}

	void cleanup() {
		//for (size_t i = 0; i != canvas.size(); i++) {
		//	canvas[i]->cleanup();
		//}
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
		for (size_t i = 0; i != vArrangements.size(); i++) {
			vArrangements[i]->Items.clear();
			vArrangements[i]->cleanup();
		}
		vArrangements.clear();
		for (size_t i = 0; i != hArrangements.size(); i++) {
			hArrangements[i]->Items.clear();
			hArrangements[i]->cleanup();
		}
		hArrangements.clear();
	}

	void hide() {
		for (size_t i = 0; i != canvas.size(); i++) {
			canvas[i]->setVisibility(false);
			canvas[i]->setIsEnabled(false);
		}
	}

	void show() {
		for (size_t i = 0; i != canvas.size(); i++) {
			canvas[i]->setVisibility(true);
			canvas[i]->setIsEnabled(false);
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

	UIItem* getPtr(vArrangement* v) {
		vArrangements.emplace_back(v);
		return vArrangements[vArrangements.size() - 1].get();
	}

	UIItem* getPtr(hArrangement* h) {
		hArrangements.emplace_back(h);
		return hArrangements[hArrangements.size() - 1].get();
	}

	std::vector<UIItem*> canvas;
	bool isSetup = false;

	LoadList* loadList = nullptr;
private:
	// Array of pointers which manages the actual structure of the UI

	// Widgets own all UI classes which appear in the UI, although widget functions use only pointers
	std::vector<std::shared_ptr<ImagePanel>> imagePanels;
	std::vector<std::shared_ptr<Button>> buttons;
	std::vector<std::shared_ptr<Checkbox>> checkboxes;
	std::vector<std::shared_ptr<spacer>> spacers;
	std::vector<std::shared_ptr<vArrangement>> vArrangements;
	std::vector<std::shared_ptr<hArrangement>> hArrangements;
};

#endif
