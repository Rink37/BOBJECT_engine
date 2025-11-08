#ifndef SURFACECONSTRUCTOR
#define SURFACECONSTRUCTOR

#include"Bobject_engine.h"
#include"Textures.h"
#include"Materials.h"
#include"Meshes.h"
#include"UIelements.h"

#include"include/BakedImages.h"

class StaticObject {
public:
	StaticObject(std::string name) {
		mesh = new StaticMesh(name);
	}

	bool isVisible = true;
	bool isWireframeVisible = true;
	StaticMesh* mesh = nullptr;
	Material* mat = nullptr;
};

class surfaceConstructor {
public:
	// Singleton management //

	static surfaceConstructor* get() {
		if (nullptr == sinstance) sinstance = new surfaceConstructor;
		return sinstance;
	}
	surfaceConstructor(const surfaceConstructor&) = delete;
	Engine& operator=(const surfaceConstructor&) = delete;

	static void destruct() {
		delete sinstance;
		sinstance = nullptr;
	}

	static surfaceConstructor* sinstance;
	surfaceConstructor() = default;
	~surfaceConstructor() = default;

	// Default state management //
	Material* webcamPtr = nullptr;

	void generateOSMap(Mesh*);
	void transitionToTS(Mesh*);
	void transitionToOS(Mesh*);

	void contextConvert();

	bool normalAvailable = false;

	Material surfaceMat;
	std::string renderPipeline = "BFShading";

	webcamTexture* webTex = nullptr;
	Texture *diffTex = nullptr; // The non-webcam texture used as the diffuse on the surface
	Texture *OSNormTex = nullptr; // The non-webcam texture used as the object-space normal map of the surface
	Texture *TSNormTex = nullptr; // The non-webcam texture used as the tangent-space normal map of the surface
	bool TSmatching = false;

	Texture surfaceNorm;

	LoadList* loadList = nullptr;

	// Materials used by the display panels //
	std::array<Material*, 2> Diffuse{}; 
	uint8_t diffuseIdx = 0;
	std::array<Material*, 3> Normal{} ;
	uint8_t normalIdx = 0;
	uint8_t normalType = 0;

	void clearSurface() {
		loadList->deleteTexture("DiffuseTex");
		loadList->deleteTexture("OSNormalTex");
		loadList->deleteTexture("TSNormalTex");
		loadList->deleteTexture("TSGenTex");
		loadList->deleteTexture("OSGenTex");
		loadList->deleteTexture("OS-TSGenTex");
		diffTex = nullptr;
		OSNormTex = nullptr;
		TSNormTex = nullptr;
		TSmatching = false;
		Diffuse = { webcamPtr, webcamPtr };
		Normal = { webcamPtr, webcamPtr, webcamPtr };
		surfaceMat.init(webTex);
		diffuseIdx = 0;
		normalIdx = 0;
		normalType = 0;
		updateSurfaceMat();
	};
	
	Material* currentDiffuse() {
		return Diffuse[diffuseIdx];
	}

	Material* currentNormal() {
		return Normal[normalIdx*(normalIdx+normalType)];
	}
	
	void toggleDiffWebcam() {
		// Simple function to flip whether the diffuse is a webcam view or not
		diffuseIdx = (diffuseIdx + 1) % 2;
		updateSurfaceMat();
	}

	void toggleNormWebcam() {
		// Simple function to flip whether the normal is a webcam view or not
		normalIdx = (normalIdx + 1) % 2;
		updateSurfaceMat();
	}

	void toggleNormType() {
		normalType = (normalType + 1) % 2;
		updateSurfaceMat();
	}

	void loadDiffuse(Texture* diffuse) {
		diffTex = loadList->replacePtr(diffuse, "DiffuseTex");
		Diffuse[1] = loadList->replacePtr(new Material(diffTex), "DiffuseMat");
		diffuseIdx = 1;
		updateSurfaceMat();
	}

	void loadNormal(Texture* normal) {
		normalIdx = 1;
		if (!normalType) {
			OSNormTex = loadList->replacePtr(normal, "OSNormalTex");
			Normal[1] = loadList->replacePtr(new Material(OSNormTex), "OSNormMat");
			TSmatching = false;
		}
		else {
			TSNormTex = loadList->replacePtr(normal, "TSNormalTex"); 
			Normal[2] = loadList->replacePtr(new Material(TSNormTex), "TSNormMat");
			TSmatching = false;
		}
		updateSurfaceMat();
	}

	void setupSurfaceConstructor() {
		webTex = webcamTexture::get();
		webTex->setup();
		webcamPtr = new Material(webTex);
		Diffuse = { webcamPtr, webcamPtr };
		Normal = { webcamPtr, webcamPtr, webcamPtr };
		surfaceMat.init(webTex);
	}
	
	void updateSurfaceMat() {
		Texture* d = nullptr;
		if (diffuseIdx == 0 || diffTex == nullptr) {
			d = webTex;
		}
		else {
			d = diffTex;
		}
		if (normalAvailable) {
			Texture* n = nullptr;
			switch (normalType) {
			case 0:
				renderPipeline = "OSNormBF";
				break;
			case 1:
				renderPipeline = "TSNormBF";
				break;
			default:
				renderPipeline = "OSNormBF";
				break;
			}
			switch (normalIdx*(normalIdx + normalType)) {
			case 1:
				if (OSNormTex != nullptr) {
					n = OSNormTex;
				}
				else {
					n = webTex;
				}
				break;
			case 2:
				if (TSNormTex != nullptr) {
					n = TSNormTex;
				}
				else {
					n = webTex;
				}
				break;
			default:
				n = webTex;
				break;
			}
			surfaceMat.init(d, n);
		}
		else {
			renderPipeline = "BFShading";
			surfaceMat.init(d);
		}
	}

	void cleanup() {
		loadList->cleanup("DiffuseTex");
		loadList->cleanup("OSNormalTex");
		loadList->cleanup("TSNormalTex");
		diffTex = nullptr;
		OSNormTex = nullptr;
		TSNormTex = nullptr;
		for (Material* mat : Diffuse) {
			if (mat != Diffuse[0]) {
				mat->cleanupDescriptor();
			}
		}
		for (Material* mat : Normal) {
			if (mat != Normal[0]) {
				mat->cleanupDescriptor();
			}
		}
		surfaceMat.cleanupDescriptor();

		webcamTexture::get()->cleanup();
		webcamPtr->cleanupDescriptor();
	}
};

class SurfaceMenu : public Widget {
public:
	SurfaceMenu(LoadList* assets) {
		loadList = assets;
	}

	void setup(surfaceConstructor*, std::vector<StaticObject>*);

	void createNormalMenu(UIItem*);

	void removeNormalMenu(UIItem*);

	void setDiffuse(Material* img) {
		diffuseView->image->mat[0] = img;
	}

	void resetDiffuseTog(bool state) {
		diffuseTog->activestate = !state;
		diffuseTog->image->matidx = static_cast<int>(state);
	}

	void setNormal(Material* img) {
		normalView->image->mat[0] = img;
	}

	void resetNormalTog(bool state) {
		normalTog->activestate = !state;
		normalTog->image->matidx = static_cast<int>(state);
	}

	void toggleNormalState(bool state) {
		NormalButtons->Items[2]->activestate = state;
		NormalButtons->Items[2]->image->matidx = static_cast<int>(!state);
	}

	ImagePanel* diffuseView = nullptr;

private:
	std::vector<StaticObject>* staticObjects = nullptr;

	bool hasNormal = false;
	bool normalsEnabled = true; // DEBUG - lets us disable any UI for normals

	Checkbox* diffuseTog = nullptr;
	Checkbox* normalTog = nullptr;

	ImagePanel* normalView = nullptr;

	Arrangement* NormalButtons = nullptr;
	Arrangement* SurfacePanel = nullptr;

	surfaceConstructor* sConst = nullptr;

	void toggleDiffuseCam(UIItem*);

	void loadDiffuseImage(UIItem*);

	void saveDiffuseImage(UIItem*);

	void toggleNormalCam(UIItem*);

	void toggleNormalType(UIItem*);

	void loadNormalImage(UIItem*);

	void saveNormalImage(UIItem*);

	void contextConvertMap(UIItem*);
};

#endif