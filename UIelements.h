#include"Bobject_Engine.h"

#ifndef UI_ELEMENTS
#define UI_ELEMENTS

#include"Textures.h"
#include"Materials.h"
#include"Meshes.h"
#include"SurfaceConstructor.h"
#include<iostream>
#include<vector>
#include<array>
#include<chrono>

#include"include/ImageDataType.h"

struct UIImage {
	bool isVisible = true;

	int texHeight = 0;
	int texWidth = 0;

	std::vector<Material*> mat = { nullptr };
	uint32_t matidx = 0;
	UIMesh mesh;

	uint32_t mipLevels = 0;

	void UpdateVertices(float, float, float, float);

	void cleanup() {
		for (Material* indMat : mat) {
			indMat->cleanup();
		}
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
	uint32_t minPixWidth, minPixHeight; // Defines the minimum size of the element on the screen - the maximum size is the image resolution

	float buffer = 50.0;
	
	float posx, posy;
	float extentx, extenty;
	float anchorx, anchory;

	float windowPositions[4] = { 0.0f };

	float sqAxisRatio; // The ratio between axes if the window was perfectly square

	std::string Name = "Unlabelled";

	UIImage* image = new UIImage;

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

		if (image->texWidth > 1) {
			this->sqAxisRatio = static_cast<float>(image->texHeight) / static_cast<float>(image->texWidth);
		}
		else {
			this->sqAxisRatio = ysize / xsize;
		}
	};

	virtual void updateDisplay() {
		this->calculateScreenPosition();
		image->UpdateVertices(posx, posy, extentx, extenty);
	}

	virtual void getSubclasses(std::vector<UIItem*> &scs) {
		scs.push_back(this);
	};

	virtual void getImages(std::vector<UIImage*>& images) {
		images.push_back(image);
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

	virtual void cleanup() {
		std::vector<UIImage*> images;
		getImages(images);

		for (UIImage* image : images) {
			for (Material* mat : image->mat) {
				if (mat != surfaceConstructor::get()->webcamMaterial && !mat->cleaned) {
					mat->cleanup();
				}
			}
			image->mesh.cleanup();
		}
	}
};

class ImagePanel : public UIItem {
// Represents only a webcam view
public:
	bool isWebcam;

	ImagePanel(float x, float y, float xsize, float ysize, Material* surf, bool iW) {
		update(x, -1.0f * y, xsize, ysize);
		image->mat[0] = surf;

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = ysize / xsize;

		this->isWebcam = iW;
	}
};

class Button : public UIItem // Here a button is just a rectangle area in screen space which can be queried with coordinates to check if it has been pressed
{
public:
	std::function<void(UIItem*)> clickFunction = nullptr;
	//std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

	Button(float x, float y, float xsize, float ysize, imageData* iDpointer) {
		this->sqAxisRatio = ysize / xsize;

		Texture* tex = new imageTexture(iDpointer);
		image->mat[0] = new Material(tex);
		
		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		update(x, -1.0f * y, xsize, ysize);
	};

	Button(imageData* iDpointer) {
		Texture* tex = new imageTexture(iDpointer);
		image->mat[0] = new Material(tex);

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = image->texHeight / image->texWidth;

		update(0.0f, 0.0f, 1.0f, 1.0f * this->sqAxisRatio);
	}

	Button(imageData* iDpointer, std::function<void(UIItem*)> func) {
		Texture* tex = new imageTexture(iDpointer);
		image->mat[0] = new Material(tex);

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = image->texHeight / image->texWidth;

		clickFunction = func;

		update(0.0f, 0.0f, 1.0f, 1.0f * this->sqAxisRatio);
	}

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

	Checkbox(float x, float y, float xsize, float ysize, imageData* iDon, imageData* iDoff) {
		this->sqAxisRatio = ysize / xsize;

		Texture* onTex = new imageTexture(iDon);
		Texture* offTex = new imageTexture(iDoff);
		image->mat[0] = new Material(onTex);
		image->mat.push_back(new Material(offTex));
		image->matidx = 0;
		this->activestate = true;

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		update(x, -1.0f * y, xsize, ysize);
	};

	Checkbox(imageData* iDon, imageData* iDoff, std::function<void(UIItem*)> func) {
		Texture* onTex = new imageTexture(iDon);
		Texture* offTex = new imageTexture(iDoff);
		image->mat[0] = new Material(onTex);
		image->mat.push_back(new Material(offTex));
		image->matidx = 0;
		this->activestate = true;

		image->texWidth = image->mat[0]->textures[0]->texWidth;
		image->texHeight = image->mat[0]->textures[0]->texHeight;

		this->sqAxisRatio = image->texHeight / image->texWidth;

		update(0.0f, 0.0f, 1.0f, 1.0f * sqAxisRatio);

		clickFunction = func;
	}

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

	std::vector<UIItem *> Items;

	void addItem(UIItem *item);

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

	std::vector<UIItem*> Items;

	void addItem(UIItem* item);

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

#endif
