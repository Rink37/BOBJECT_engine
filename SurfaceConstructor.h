#ifndef SURFACECONSTRUCTOR
#define SURFACECONSTRUCTOR

#include"Bobject_engine.h"
#include"Textures.h"
#include"Materials.h"
#include"Meshes.h"

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
	Material webcamMaterial;
	Material diffuseMaterial;
	Material osNormMaterial;
	Material tsNormMaterial;

	void generateOSMap(Mesh*);
	void transitionToTS(Mesh*);

	void contextConvert();

	bool normalAvailable = false;

	Material surfaceMat;
	std::string renderPipeline = "BFShading";

	webcamTexture* webTex = nullptr;
	Texture *diffTex = nullptr; // The non-webcam texture used as the diffuse on the surface
	Texture *OSNormTex = nullptr; // The non-webcam texture used as the object-space normal map of the surface
	Texture *TSNormTex = nullptr; // The non-webcam texture used as the tangent-space normal map of the surface
	bool TSmatching = false;

	// Materials used by the display panels //
	std::array<Material*, 2> Diffuse{}; 
	uint8_t diffuseIdx = 0;
	std::array<Material*, 3> Normal{} ;
	uint8_t normalIdx = 0;
	uint8_t normalType = 0; 

	void clearSurface() {
		if (diffTex != nullptr) {
			diffTex->cleanup();
			delete diffTex;
			diffTex = nullptr;
		}
		if (OSNormTex != nullptr) {
			OSNormTex->cleanup();
			delete OSNormTex;
			OSNormTex = nullptr;
		}
		if (TSNormTex != nullptr) {
			TSNormTex->cleanup();
			delete TSNormTex;
			TSNormTex = nullptr;
		}
		Diffuse = { &webcamMaterial, &webcamMaterial };
		Normal = { &webcamMaterial, &webcamMaterial, &webcamMaterial };
		surfaceMat.init(webTex);
		diffuseIdx = 0;
		normalIdx = 0;
		normalType = 0;
	};
	
	Material* currentDiffuse() {
		return Diffuse[diffuseIdx];
	}

	Material* currentNormal() {
		return Normal[normalIdx];
	}
	
	void toggleDiffWebcam() {
		// Simple function to flip whether the diffuse is a webcam view or not
		diffuseIdx = (diffuseIdx + 1) % 2;
		updateSurfaceMat();
	}

	void toggleNormWebcam() {
		// Simple function to flip whether the normal is a webcam view or not
		if (!normalIdx) {
			normalIdx += 1 + normalType;
		}
		else {
			normalIdx = 0;
		}
		updateSurfaceMat();
	}

	void toggleNormType() {
		normalType = (normalType + 1) % 2;
		if (normalIdx != 0) {
			normalIdx = 1 + normalType;
		}
		updateSurfaceMat();
	}

	void loadDiffuse(Texture* diffuse) {
		if (diffTex != nullptr) {
			diffTex->cleanup();
			delete diffTex;
			diffTex = nullptr;
		}
		diffTex = diffuse;
		diffuseMaterial.init(diffTex);
		Diffuse[1] = &diffuseMaterial;
		diffuseIdx = 1;
	}

	void loadNormal(Texture* normal) {
		if (!normalType) {
			if (OSNormTex != nullptr) {
				OSNormTex->cleanup();
				delete OSNormTex;
				OSNormTex = nullptr;
			}
			OSNormTex = normal;
			osNormMaterial.init(OSNormTex);
			Normal[1] = &osNormMaterial;
			normalIdx = 1;
			TSmatching = false;
		}
		else {
			if (TSNormTex != nullptr) {
				TSNormTex->cleanup();
				delete TSNormTex;
				TSNormTex = nullptr;
			}
			TSNormTex = normal;
			tsNormMaterial.init(TSNormTex);
			Normal[2] = &tsNormMaterial; 
			normalIdx = 2;
		}
	}

	void setupSurfaceConstructor() {
		webTex = webcamTexture::get();
		webTex->setup();
		webcamMaterial.init(webTex);
		Diffuse = { &webcamMaterial, &webcamMaterial };
		Normal = { &webcamMaterial, &webcamMaterial, &webcamMaterial };
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
			switch (normalIdx) {
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
		if (diffTex != nullptr) {
			diffTex->cleanup();
			delete diffTex;
			diffTex = nullptr;
		}
		if (OSNormTex != nullptr) {
			OSNormTex->cleanup();
			delete OSNormTex;
			OSNormTex = nullptr;
		}
		if (TSNormTex != nullptr) {
			TSNormTex->cleanup();
			delete TSNormTex;
			TSNormTex = nullptr;
		}
		diffuseMaterial.cleanupDescriptor();
		osNormMaterial.cleanupDescriptor();
		tsNormMaterial.cleanupDescriptor();
		surfaceMat.cleanupDescriptor();
		webcamTexture::get()->cleanup();
		webcamMaterial.cleanupDescriptor();
	}
};

#endif