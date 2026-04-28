#include"SurfaceConstructor.h"
#include"GenerateNormalMap.h"
#include"ImageProcessor.h"
#include"WindowsFileManager.h"

#include"include/Kuwahara.h"
#include"include/SobelX.h"
#include"include/SobelY.h"
#include"include/ReferenceKuwahara.h"
#include"include/Averager.h"
#include"include/GradRemap.h"

#include"include/OS_EdgeFill.h"
#include"ImageProcessor.h"

using namespace std;
using namespace cv;

void surfaceConstructor::generateOSMap(Mesh* inputMesh) {

	NormalGen generator(loadList);
	generator.setupOSExtractor();
	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
	commandBuffer = generator.drawOSMap(commandBuffer, inputMesh);
	Engine::get()->endSingleTimeCommands(commandBuffer);

	Texture* EdgeFillImg = generator.objectSpaceMap.colour->copyTexture(generator.objectSpaceMap.colour->textureFormat, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1);
	filter OS_EdgeFill(std::vector<Texture*>({ EdgeFillImg }), new OS_EDGEFILLSHADER, VK_FORMAT_R8G8B8A8_UNORM);
	OS_EdgeFill.filterImage();
	OS_EdgeFill.filterTarget[0]->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	OS_EdgeFill.filterTarget[0]->textureLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	
	OSNormTex = loadList->replacePtr(OS_EdgeFill.filterTarget[0]->copyImage(VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_TILING_LINEAR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1), "OSNormalTex");
	
	OSNormTex->textureImageView = OSNormTex->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

	Normal[1] = loadList->replacePtr(new Material(OSNormTex), "OSNormMat");

	generator.cleanupGenOS();
	EdgeFillImg->cleanup();
	OS_EdgeFill.cleanup();
}

void surfaceConstructor::transitionToTS(Mesh* inputMesh) {

	NormalGen generator(loadList);
	generator.copyOSImage(OSNormTex);
	generator.setupTSConverter();
	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
	commandBuffer = generator.convertOStoTS(commandBuffer, inputMesh);
	Engine::get()->endSingleTimeCommands(commandBuffer);

	TSNormTex = loadList->replacePtr(generator.tangentSpaceMap.colour->copyImage(VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_TILING_LINEAR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1), "TSNormalTex");

	TSNormTex->textureImageView = TSNormTex->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

	Normal[2] = loadList->replacePtr(new Material(TSNormTex), "TSNormMat");
	
	generator.cleanupTS();
}

void surfaceConstructor::transitionToOS(Mesh* inputMesh) {
	NormalGen generator(loadList);
	generator.copyTSImage(TSNormTex);
	generator.setupOSConverter();
	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
	commandBuffer = generator.convertTStoOS(commandBuffer, inputMesh);
	Engine::get()->endSingleTimeCommands(commandBuffer);

	OSNormTex = loadList->replacePtr(generator.objectSpaceMap.colour->copyImage(VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_TILING_LINEAR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1), "OSNormalTex");

	OSNormTex->textureImageView = OSNormTex->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

	Normal[1] = loadList->replacePtr(new Material(OSNormTex), "OSNormMat");
	
	generator.cleanupOS();
}

void SurfaceMenu::setup(surfaceConstructor* surfConst, std::vector<StaticObject>* objects, std::function<void(UIItem*)> remapFunc) {

	remapCallback = remapFunc;

	sConst = surfConst;
	sConst->loadList = loadList;
	staticObjects = objects;

	std::function<void(UIItem*)> addNormalButton = bind(&SurfaceMenu::createNormalMenu, this, placeholders::_1);
	std::function<void(UIItem*)> toggleDiffuse = bind(&SurfaceMenu::toggleDiffuseCam, this, placeholders::_1);
	std::function<void(UIItem*)> loadDiffuse = bind(&SurfaceMenu::loadDiffuseImage, this, placeholders::_1);
	std::function<void(UIItem*)> saveWebcam = bind(&SurfaceMenu::saveDiffuseImage, this, placeholders::_1);

	imageData diffuse = DIFFUSETEXT;
	Material* diffuseMat = newMaterial(&diffuse, "DiffuseBtn");

	imageData normal = NORMALTEXT;
	Material* normalMat = newMaterial(&normal, "NormalBtn");

	imageData webcamOn = WEBCAMONBUTTON;
	Material* webcamOnMat = newMaterial(&webcamOn, "WebcamOnBtn");

	imageData webcamOff = WEBCAMOFFBUTTON;
	Material* webcamOffMat = newMaterial(&webcamOff, "WebcamOffBtn");

	imageData OpenButton = OPENBUTTON;
	Material* openMat = newMaterial(&OpenButton, "OpenBtn");

	imageData SaveButton = SAVEBUTTON;
	Material* saveMat = newMaterial(&SaveButton, "SaveBtn");

	imageData plusButton = PLUSBUTTON;
	Material* plusMat = newMaterial(&plusButton, "PlusBtn");

	Button* diffuseTextPanel = new Button(diffuseMat);

	diffuseTog = new Checkbox(webcamOnMat, webcamOffMat, toggleDiffuse);
	diffuseTog->Name = "ToggleDiffuseWebcam";
	diffuseTog->setClickFunction(toggleDiffuse);

	Button* diffLoad = new Button(openMat, loadDiffuse);
	Button* diffSave = new Button(saveMat, saveWebcam);

	Arrangement* DiffuseButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.2f, 0.01f);

	DiffuseButtons->addItem(getPtr(diffuseTextPanel));
	DiffuseButtons->addItem(getPtr(diffuseTog));
	DiffuseButtons->addItem(getPtr(new spacer));
	DiffuseButtons->addItem(getPtr(diffLoad));
	DiffuseButtons->addItem(getPtr(diffSave));

	SurfacePanel = new Arrangement(ORIENT_VERTICAL, 1.0f, 0.0f, 0.25f, 0.8f, 0.01f);

	diffuseView = new ImagePanel(sConst->currentDiffuse(), true);
	SurfacePanel->addItem(getPtr(DiffuseButtons));
	SurfacePanel->addItem(getPtr(diffuseView));
	
	if (normalsEnabled) {
		Button* normalTextPanel = new Button(normalMat);
		Button* normalPlus = new Button(plusMat, addNormalButton);

		NormalButtons = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.2f, 0.01f);

		NormalButtons->addItem(getPtr(normalTextPanel));
		NormalButtons->addItem(getPtr(normalPlus));
		NormalButtons->addItem(getPtr(new spacer));
		SurfacePanel->addItem(getPtr(NormalButtons));
	}
	
	SurfacePanel->addItem(getPtr(new spacer));

	SurfacePanel->updateDisplay();

	canvas.push_back(getPtr(SurfacePanel));

	hasNormal = false;
}

void SurfaceMenu::removeNormalMenu(UIItem* owner) {
	if (!hasNormal || !normalsEnabled) {
		return;
	}
	// Clear UI related to the normal component of the surface panel
	SurfacePanel->Items.at(SurfacePanel->Items.size() - 2)->cleanup();
	SurfacePanel->removeItem(static_cast<uint32_t>(SurfacePanel->Items.size() - 2));

	NormalButtons->cleanup();
	NormalButtons->Items.clear();

	std::function<void(UIItem*)> addNormalButton = bind(&SurfaceMenu::createNormalMenu, this, placeholders::_1);

	imageData normal = NORMALTEXT;
	Material* normalMat = newMaterial(&normal, "NormalBtn");

	imageData plusButton = PLUSBUTTON;
	Material* plusMat = newMaterial(&plusButton, "PlusBtn");

	Button* normalTextPanel = new Button(normalMat);
	Button* normalPlus = new Button(plusMat, addNormalButton);

	NormalButtons->addItem(getPtr(normalTextPanel));
	NormalButtons->addItem(getPtr(normalPlus));
	NormalButtons->addItem(getPtr(new spacer));

	SurfacePanel->updateDisplay();
	hasNormal = false;

	update();
}

void SurfaceMenu::toggleDiffuseCam(UIItem* owner) {
	sConst->toggleDiffWebcam();
	diffuseView->image->mat[0] = sConst->currentDiffuse();
}

void SurfaceMenu::loadDiffuseImage(UIItem* owner) {
	string fileName = winFile::OpenFileDialog();
	if (fileName != string("fail")) {
		imageTexture* loadedTexture = new imageTexture(fileName, VK_FORMAT_R8G8B8A8_SRGB);
		sConst->loadDiffuse(loadedTexture);
		sConst->diffuseIdx = 1;
		diffuseView->image->mat[0] = sConst->currentDiffuse();
		diffuseView->image->texHeight = diffuseView->image->mat[0]->textures[0]->texHeight;
		diffuseView->image->texWidth = diffuseView->image->mat[0]->textures[0]->texWidth;
		diffuseView->sqAxisRatio = static_cast<float>(diffuseView->image->texHeight) / static_cast<float>(diffuseView->image->texWidth);
		diffuseTog->activestate = false;
		diffuseTog->image->matidx = 1;
		sConst->updateSurfaceMat();
		SurfacePanel->arrangeItems();
		session::get()->currentStudio.diffusePath = fileName;
		update();
	}
}

void SurfaceMenu::saveDiffuseImage(UIItem* owner) {
	Mat saveDiffuse;
	if (sConst->diffuseIdx == 0 || sConst->diffTex == nullptr) {
		saveDiffuse = webcamTexture::get()->webCam->webcamFrame;
	}
	else {
		sConst->diffTex->getCVMat();
		saveDiffuse = sConst->diffTex->texMat.clone();
		sConst->diffTex->destroyCVMat();
	}
	string saveName = winFile::SaveFileDialog();
	if (saveName != string("fail")) {
		session::get()->currentStudio.diffusePath = saveName;
		imwrite(saveName, saveDiffuse);
	}
}

void SurfaceMenu::toggleNormalCam(UIItem* owner) {
	sConst->toggleNormWebcam();
	normalView->image->mat[0] = sConst->currentNormal();
}

void SurfaceMenu::toggleNormalType(UIItem* owner) {
	if (staticObjects->size() == 0) {
		toggleNormalState(!NormalButtons->Items[2]->activestate);
		return;
	}
	sConst->toggleNormType();
	if (sConst->normalType == 1 && sConst->OSNormTex != nullptr && !sConst->TSmatching) {
		if (sConst->TSNormTex != nullptr) {
			loadList->deleteTexture("TSNormalTex");
		}
		sConst->transitionToTS(staticObjects->at(staticObjects->size() - 1).mesh);
		sConst->TSmatching = true;
	}
	if (sConst->normalType == 0 && sConst->TSNormTex != nullptr && !sConst->TSmatching) {
		if (sConst->OSNormTex != nullptr) {
			loadList->deleteTexture("OSNormalTex");
		}
		sConst->transitionToOS(staticObjects->at(staticObjects->size() - 1).mesh);
		sConst->TSmatching = true;
	}
	sConst->updateSurfaceMat();
	setNormal(sConst->currentNormal());
	update();
}

void SurfaceMenu::loadNormalImage(UIItem* owner) {
	string fileName = winFile::OpenFileDialog();
	if (fileName != string("fail")) {
		imageTexture* loadedTexture = new imageTexture(fileName, VK_FORMAT_R8G8B8A8_UNORM);
		sConst->loadNormal(loadedTexture);
		sConst->normalIdx = 1;
		normalView->image->mat[0] = sConst->currentNormal();
		normalView->image->texHeight = diffuseView->image->mat[0]->textures[0]->texHeight;
		normalView->image->texWidth = diffuseView->image->mat[0]->textures[0]->texWidth;
		normalView->sqAxisRatio = static_cast<float>(normalView->image->texHeight) / static_cast<float>(normalView->image->texWidth);
		normalTog->activestate = false;
		normalTog->image->matidx = 1;
		sConst->updateSurfaceMat();
		SurfacePanel->arrangeItems();
		if (!sConst->normalType) {
			session::get()->currentStudio.OSPath = fileName;
		}
		else {
			session::get()->currentStudio.TSPath = fileName;
		}
		update();
	}
}

void SurfaceMenu::saveNormalImage(UIItem* owner) {
	Mat saveNormal;
	if (sConst->normalIdx == 0) {
		saveNormal = webcamTexture::get()->webCam->webcamFrame;
	}
	else {
		if (sConst->normalType) {
			sConst->TSNormTex->getCVMat();
			saveNormal = sConst->TSNormTex->texMat.clone();
			sConst->TSNormTex->destroyCVMat();
		}
		else {
			sConst->OSNormTex->getCVMat();
			saveNormal = sConst->OSNormTex->texMat.clone();
			sConst->OSNormTex->destroyCVMat();
		}
	}
	string saveName = winFile::SaveFileDialog();
	if (saveName != string("fail")) {
		if (sConst->normalType == 0) {
			session::get()->currentStudio.OSPath = saveName;
		}
		else if (sConst->normalType == 1) {
			session::get()->currentStudio.TSPath = saveName;
		}
		imwrite(saveName, saveNormal);
	}
}

void SurfaceMenu::createNormalMenu(UIItem* owner) {

	if (staticObjects->size() == 0 && sConst->OSNormTex == nullptr && sConst->TSNormTex == nullptr) {
		return;
	}

	std::function<void(UIItem*)> toggleWebcam = bind(&SurfaceMenu::toggleNormalCam, this, placeholders::_1);
	std::function<void(UIItem*)> toggleType = bind(&SurfaceMenu::toggleNormalType, this, placeholders::_1);
	std::function<void(UIItem*)> saveNorm = bind(&SurfaceMenu::saveNormalImage, this, placeholders::_1);
	std::function<void(UIItem*)> loadNorm = bind(&SurfaceMenu::loadNormalImage, this, placeholders::_1);

	sConst->normalType = 0;

	if (staticObjects->size() > 0 && sConst->OSNormTex == nullptr) {
		sConst->generateOSMap(staticObjects->at(staticObjects->size() - 1).mesh); // This function is definitely the source of the memory problems - I just don't know why
	}
	else if (sConst->TSNormTex != nullptr) {
		sConst->normalType = 1;
	}

	sConst->normalAvailable = true;
	webcamTexture::get()->changeFormat(VK_FORMAT_R8G8B8A8_UNORM);
	sConst->normalIdx = 1;
	sConst->updateSurfaceMat();

	SurfacePanel->removeItem(3);

	vector<UIImage*> images;
	NormalButtons->getImages(images, true);
	NormalButtons->getImages(images, false);

	for (UIImage* image : images) {
		image->cleanup();
	}

	NormalButtons->Items.clear();

	imageData normal = NORMALTEXT;
	Material* normalMat = newMaterial(&normal, "NormalBtn"); 

	imageData webcamOn = WEBCAMONBUTTON;
	Material* webcamOnMat = newMaterial(&webcamOn, "WebcamOnBtn");

	imageData webcamOff = WEBCAMOFFBUTTON;
	Material* webcamOffMat = newMaterial(&webcamOff, "WebcamOffBtn");

	imageData OpenButton = OPENBUTTON;
	Material* openMat = newMaterial(&OpenButton, "OpenBtn");

	imageData SaveButton = SAVEBUTTON;
	Material* saveMat = newMaterial(&SaveButton, "SaveBtn");

	imageData plusButton = PLUSBUTTON;
	Material* plusMat = newMaterial(&plusButton, "PlusBtn");

	imageData osType = OSBUTTON;
	Material* osMat = newMaterial(&osType, "OSBtn");

	imageData tsType = TANGENTSPACE;
	Material* tsMat = newMaterial(&tsType, "TSBtn");

	imageData diffToNorm = D2NBUTTON;
	Material* dtnMat = newMaterial(&diffToNorm, "D2NBtn");

	Button* normalText = new Button(normalMat);

	normalTog = new Checkbox(webcamOnMat, webcamOffMat, toggleWebcam);
	normalTog->activestate = false;
	normalTog->image->matidx = 1;

	Checkbox* mapTypeToggle = new Checkbox(osMat, tsMat, toggleType);
	if (sConst->normalType == 1){
		bool state = false;
		mapTypeToggle->activestate = state;
		mapTypeToggle->image->matidx = static_cast<int>(!state);
	}
	Button* copyLayout = new Button(dtnMat, remapCallback);
	Button* normalLoad = new Button(openMat, loadNorm);
	Button* normalSave = new Button(saveMat, saveNorm);

	NormalButtons->addItem(getPtr(normalText));
	NormalButtons->addItem(getPtr(normalTog));
	NormalButtons->addItem(getPtr(mapTypeToggle));
	NormalButtons->addItem(getPtr(copyLayout));
	NormalButtons->addItem(getPtr(normalLoad));
	NormalButtons->addItem(getPtr(normalSave));

	normalView = new ImagePanel(sConst->currentNormal(), true);
	normalView->image->texHeight = diffuseView->image->texHeight;
	normalView->image->texWidth = diffuseView->image->texWidth;
	normalView->sqAxisRatio = static_cast<float>(normalView->image->texHeight) / static_cast<float>(normalView->image->texWidth);

	SurfacePanel->addItem(getPtr(normalView));
	SurfacePanel->addItem(getPtr(new spacer));
	SurfacePanel->updateDisplay();

	hasNormal = true;

	update();
}