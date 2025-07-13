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

	void setupSurfaceConstructor() {
		webcamTexture::get()->setup();
		webcamMaterial = new Material(webcamTexture::get());
		Diffuse = { webcamMaterial, webcamMaterial };
		Normal = { webcamMaterial, webcamMaterial, webcamMaterial };
		surfaceMat = webcamMaterial;
	}

	// Default state management //
	Material* webcamMaterial = nullptr;

	Texture* diffTex = nullptr; // The non-webcam texture used as the diffuse on the surface
	Texture* OSNormTex = nullptr; // The non-webcam texture used as the object-space normal map of the surface
	Texture* TSNormTex = nullptr; // The non-webcam texture used as the tangent-space normal map of the surface
	bool TSmatching = false;

	// Materials used by the display panels //
	std::array<Material*, 2> Diffuse = { }; // The material used as a diffuse - none if we don't load any image
	uint8_t diffuseIdx = 0;
	std::array<Material*, 3> Normal = { } ; // The material used as the normal map
	uint8_t normalIdx = 0;
	uint8_t normalType = 0; // 0 represents the OS normal, 1 represents the TS normal - we can index the correct normal as 1+normalType

	Material* currentDiffuse() {
		return Diffuse[diffuseIdx];
	}

	Material* currentNormal() {
		return Normal[normalIdx];
	}

	void contextConvert();
	
	void toggleDiffWebcam() {
		// Simple function to flip whether the diffuse is a webcam view or not
		diffuseIdx = (diffuseIdx + 1) % 2;
	}

	void toggleNormWebcam() {
		// Simple function to flip whether the normal is a webcam view or not
		if (!normalIdx) {
			normalIdx += 1 + normalType;
		}
		else {
			normalIdx = 0;
		}
	}

	void toggleNormType() {
		normalType = (normalType + 1) % 2;
		if (normalIdx != 0) {
			normalIdx = 1 + normalType;
		}
	}

	void loadDiffuse(Texture* diffuse) {
		if (diffTex != nullptr) {
			diffTex->cleanup();
		}
		diffTex = diffuse;
		if (Diffuse[1] != webcamMaterial) {
			Diffuse[1]->cleanupDescriptor();
		}
		Diffuse[1] = new Material(diffTex);
		diffuseIdx = 1;
	}

	void loadNormal(Texture* normal) {
		if (!normalType) {
			if (OSNormTex != nullptr) {
				OSNormTex->cleanup();
			}
			if (Normal[1] != webcamMaterial) {
				Normal[1]->cleanupDescriptor();
			}
			OSNormTex = normal;
			//OSNormTex->textureFormat = VK_FORMAT_R8G8B8A8_UNORM;
			Normal[1] = new Material(OSNormTex);
			normalIdx = 1;
			TSmatching = false;
		}
		else {
			if (TSNormTex != nullptr) {
				TSNormTex->cleanup();
			}
			if (Normal[2] != webcamMaterial) {
				Normal[2]->cleanupDescriptor();
			}
			TSNormTex = normal;
			//TSNormTex->textureFormat = VK_FORMAT_R8G8B8A8_UNORM;
			Normal[2] = new Material(TSNormTex);
			normalIdx = 2;
		}
	}

	void generateOSMap(Mesh*);
	void transitionToTS(Mesh*);

	bool normalAvailable = false;

	Material* surfaceMat = nullptr;
	std::string renderPipeline = "BFShading";

	void updateSurfaceMat() {
		if (surfaceMat != nullptr && surfaceMat != webcamMaterial) {
			surfaceMat->cleanupDescriptor();
		}
		Texture* d;
		if (diffuseIdx == 0 || diffTex == nullptr) {
			d = webcamTexture::get();
		}
		else {
			d = diffTex;
		}
		if (normalAvailable) {
			Texture* n = nullptr;
			switch (normalIdx) {
			case 0:
				n = webcamTexture::get();
				break;
			case 1:
				if (OSNormTex != nullptr) {
					n = OSNormTex;
				}
				else {
					n = webcamTexture::get();
				}
				renderPipeline = "OSNormBF";
				break;
			case 2:
				if (TSNormTex != nullptr) {
					n = TSNormTex;
				}
				else {
					n = webcamTexture::get();
				}
				renderPipeline = "TSNormBF";
				break;
			default:
				n = webcamTexture::get();
				break;
			}
			surfaceMat = new Material(d, n);
		}
		else {
			renderPipeline = "BFShading";
			surfaceMat = new Material(d);
		}
	}

	void cleanup() {
		if (diffTex != nullptr && !diffTex->cleaned) {
			diffTex->cleanup();
		}
		if (OSNormTex != nullptr && !OSNormTex->cleaned) {
			OSNormTex->cleanup();
		}
		if (TSNormTex != nullptr && !TSNormTex->cleaned) {
			TSNormTex->cleanup();
		}
		for (Material* material : Diffuse) {
			if (material != webcamMaterial && !material->cleaned) {
				material->cleanupDescriptor();
			}
		}
		for (Material* material : Normal) {
			if (material != webcamMaterial && !material->cleaned) {
				material->cleanupDescriptor();
			}
		}
		surfaceMat->cleanupDescriptor();
		webcamTexture::get()->cleanup();
		webcamMaterial->cleanupDescriptor();
	}
};


#endif