#ifndef TRADPAINTERWIDGETS
#define TRADPAINTERWIDGETS

#include"include/CombineNorms.h"
#include"WindowsFileManager.h"
#include"GenerateNormalMap.h"
#include"WidgetTemplates.h"
#include"ImageProcessor.h"
#include"Objects.h"

class TextureSettings : public Widget {
public:
	TextureSettings(LoadList* assets, LoadList* textureAssets) {
		loadList = assets;
		textureLL = textureAssets;
	}

	void setup(std::string, std::function<void(UIItem*)>, std::function<void(UIItem*)>, std::function<void(UIItem*)>, std::function<void(UIItem*)>, std::function<void(UIItem*)>, std::function<void(UIItem*)>, std::function<void(UIItem*)>, std::function<void(UIItem*)>, std::function<void(UIItem*)>, std::function<void(float)>, std::function<void(UIItem*)>);
	void getUVPos(glm::vec4&);
	void updateWebcamTex();

private:
	void functionMap(UIItem*);

	std::unordered_map<std::string, std::function<void(UIItem*)>> functions{};

	LoadList* textureLL = nullptr;
	ImagePanel* imagePanel = nullptr;

	std::string imageName = "";
};


class WebcamSettings : public Widget {
public:
	WebcamSettings(LoadList* assets, LoadList* texLL) {
		loadList = assets;
		textureLL = texLL;
	}

	void setup(std::function<void(UIItem*)>, std::function<void()>);
	void customUpdate();

	std::function<void()> reload = nullptr;
	int priorityLayer = 100;

private:

	LoadList* textureLL = nullptr;

	ImagePanel* webcamView = nullptr;
	Slider* ratioSlider = nullptr;

	void calibrateWebcam(UIItem*);
	void addRotation(UIItem*);
	void subtractRotation(UIItem*);
	void indexUp(UIItem*);
	void indexDown(UIItem*);
	void revertAspectRatio(UIItem*);
	void updateAspectRatio(float);
};

class RenderMenu : public Widget {
public:
	RenderMenu(LoadList* assets) {
		loadList = assets;
	}

	void setup(std::function<void(UIItem*)>, std::function<void(UIItem*)>, std::function<void(float)>, std::function<void(float)>);
	void hideViewButtons();
	void showViewButtons();
};

class MaterialCreator : public Widget {
public:
	MaterialCreator(LoadList* assets, LoadList* textureAssets) {
		loadList = assets;
		textureLL = textureAssets;
	}

	void setup(std::string, GraphicsPass*, std::function<void(UIItem*)>);
	void setupEditMode(std::string, GraphicsPass*, std::function<void(UIItem*)>, std::string);
private:
	MaterialTemplate* matTemplate = nullptr;

	std::string shaderName = "";
	GraphicsPass* boundPass = nullptr;

	std::string newMaterialName = "";

	LoadList* textureLL = nullptr;

	std::function<void(UIItem*)> finishedCallback = nullptr;

	std::vector<int> autoIndices{};

	void setMaterialName(UIItem* owner) {
		newMaterialName = owner->text;
	}

	void createMatOptionsMenu(UIItem*);
	void setTexCallback(UIItem*);
	void exit(UIItem*);
};

class ObjectSettingsMenu : public Widget {
public:
	ObjectSettingsMenu(LoadList* assets, LoadList* textureAssets) {
		loadList = assets;
		textureLL = textureAssets;
	}

	void setup(StaticObject*, std::function<void(UIItem*)>, std::function<void(std::function<void(UIItem*)>)>, std::function<void(UIItem*)>, std::function<void(std::function<void(UIItem*)>, std::string)>);

private:
	StaticObject* obj = nullptr;
	LoadList* textureLL = nullptr;
	UIItem* matSelPtr = nullptr;
	UIItem* settingsButton = nullptr;

	std::function<void(UIItem*)> finishedCallback = nullptr;
	std::function<void(std::function<void(UIItem*)>)> openMaterialMenu = nullptr;
	std::function<void(std::function<void(UIItem*)>, std::string)> openEditMaterialMenu = nullptr;
	std::function<void(UIItem*)> closeMaterialMenu = nullptr;
	std::function<void(UIItem*)> addTextureFunc = nullptr;
	std::string OSNormName = "";

	void newMaterialMenu(UIItem*);
	void editMaterialMenu(UIItem*);
	void exitMaterialMenu(UIItem*);
	void selectMaterialCallback(UIItem*);
	void createNormalMap(UIItem*);
};

class ObjectMenu : public Widget {
public:
	ObjectMenu(LoadList* assets) {
		loadList = assets;
	}

	void setup(std::function<void(UIItem*)>);
	void addObject(std::function<void(UIItem*)>, std::function<void(UIItem*)>, std::function<void(UIItem*)>, std::string);
	void clearObjects();

	std::map<std::string, int> ObjectMap = {};
private:
	UIItem* ObjectButtons = nullptr;
	Material* visibleMat = nullptr;
	Material* invisibleMat = nullptr;
	Material* wireframeMat = nullptr;
	Material* settingsMat = nullptr;
};

class NormalMixer : public Widget {
public:
	NormalMixer(LoadList* assets, LoadList* texLL) {
		loadList = assets;
		textureLL = texLL;
	}

	void setup(std::string, std::vector<std::string>, std::function<Mesh* (std::string)>, std::function<void(UIItem*)>, std::function<void(UIItem*)>);

private:
	LoadList* textureLL = nullptr;
	std::string textureName = "";
	std::string mixTexName = "";
	std::string objectName = "";
	std::vector<std::string> createdTextures{};
	std::function<Mesh* (std::string)> getMesh = nullptr;
	std::function<void(UIItem*)> exitCallback = nullptr;

	void createRefNormal(std::string&);
	void changeNormalMapType(std::string);
	void setMixTex(UIItem*);
	void setObject(UIItem*);
	void mixNorms(UIItem*);
};

class SpaceTransitionMenu : public Widget {
public:
	SpaceTransitionMenu(LoadList* assets, LoadList* texLL) {
		loadList = assets;
		textureLL = texLL;
	}

	void setup(std::string, std::vector<std::string>, std::function<void(UIItem*)>, std::function<Mesh* (std::string)>, std::function<void(UIItem*)>);
private:
	LoadList* textureLL = nullptr;
	UIItem* overwriteToggle = nullptr;
	std::string texName = "";
	std::string objectName = "";
	std::string outNormalName = "";
	std::function<void(UIItem*)> addTextureFunc = nullptr;
	std::function<void(UIItem*)> exitCallback = nullptr;
	std::function<Mesh* (std::string)> getMesh = nullptr;
	bool overwrite = true;

	void setObjName(UIItem*);
	void exit(UIItem*);
	void changeNormalMapType(UIItem*);
};

class TextureLoadMenu : public Widget {
public:
	TextureLoadMenu(LoadList* assets, LoadList* texLL) {
		loadList = assets;
		textureLL = texLL;
	}

	void setup(std::function<void(UIItem*)>);

private:
	LoadList* textureLL = nullptr;
	std::string textureName = "";
	std::string textureExt = "";
	std::string fileName = "";
	UIItem* normalTypeToggle = nullptr;
	bool isNormalized = false;
	bool normalType = false;

	std::function<void(UIItem*)> updateTextureMenu = nullptr;

	void setOptionCallback(UIItem*);
	void finishCallback(UIItem*);
};

class TextureMenu : public Widget {
public:
	TextureMenu(LoadList* assets, LoadList* textureLL) {
		loadList = assets;
		textureLoadList = textureLL;
	}

	void setup(std::function<void(UIItem*)>, std::function<void(std::function<void(UIItem*)>)>, std::function<void(UIItem*)>);

	std::function<void(UIItem*)> getAddTexCallback() {
		return std::bind(&TextureMenu::addTexture, this, std::placeholders::_1);
	}

private:
	std::function<void(std::function<void(UIItem*)>)> loadTextureCallback = nullptr;

	void loadTexture(UIItem*);
	void addTexture(UIItem*);
	void saveTexture(UIItem*);

	UIItem* TextureButtons = nullptr;
	LoadList* textureLoadList = nullptr;
	font* objectFont = nullptr;
	Material* settingsMat = nullptr;
	Material* saveMat = nullptr;
	std::function<void(UIItem*)> modifyCallback = nullptr;
	std::function<void(UIItem*)> saveCallback = nullptr;
};

class WebcamMenu : public Widget {
public:
	WebcamMenu(LoadList* assets) {
		loadList = assets;
	}

	void setup(std::function<void(UIItem*)>, std::function<void(UIItem*)>);
private:
	void toggleWebcam(UIItem* owner) {
		if (webcamTexture::get()->webCam != nullptr) {
			webcamTexture::get()->webCam->shouldUpdate = owner->activestate;
		}
	}
};

#endif