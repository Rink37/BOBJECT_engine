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

#endif
