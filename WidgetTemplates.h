#ifndef WIDGETTEMPLATES
#define WIDGETTEMPLATES

#include"LoadLists.h"
#include"UIelements.h"

class ErrorDialog : public Widget {
public:
	ErrorDialog(LoadList* assets) {
		loadList = assets;
		priorityLayer = 5000;
		this->zp = 0.0f;
	}

	void setup(std::string, std::string);
};

class TexSelector : public Widget {
public:
	TexSelector(LoadList* assets, LoadList* texLL) {
		loadList = assets;
		textureLL = texLL;
	}

	void setText(std::string head, std::string prompt) {
		headerText = head;
		promptText = prompt;
	}

	void setup(std::string, std::function<void(std::string, std::string)>);

	int clickIndex = 0;

private:
	std::string headerText = "";
	std::string promptText = "";

	LoadList* textureLL = nullptr;

	std::string targetName = "";
	std::string refName = "";

	std::function<void(std::string, std::string)> continueCallback = nullptr;

	void setRefName(UIItem* owner) {
		refName = owner->text;
	}

	void finish(UIItem* owner);
};

class ObjSelector : public Widget {
public:
	ObjSelector(LoadList* assets, LoadList* texLL) {
		loadList = assets;
		textureLL = texLL;
	}

	void setText(std::string head) {
		headerText = head;
	}

	void setup(std::string, std::vector<std::string>, std::function<void(UIItem*)>, std::function<void(UIItem*)>);
private:
	std::string headerText = "";

	LoadList* textureLL = nullptr;
	std::string objectName = "";

	std::string textureName = "";
	std::function<Mesh* (std::string)> getMesh = nullptr;
	std::function<void(UIItem*)> finishCallback = nullptr;

	void setObject(UIItem* owner) {
		objectName = owner->text;
	}

	void finishFunc(UIItem* owner) {
		if (objectName == "") {
			if (errorFunc != nullptr) {
				errorFunc("SELECT ERROR: ", "No base object has been selected!");
			}
			return;
		}
		owner->Name = textureName;
		owner->text = objectName;
		if (finishCallback != nullptr) {
			finishCallback(owner);
		}
		selfDestruct(owner);
	}
};

#endif
