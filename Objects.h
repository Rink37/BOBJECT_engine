#ifndef OBJDEFS
#define OBJDEFS

#include"Bobject_VKMem.h"
#include"Bobject_Core.h"
#include"Meshes.h"
#include"Materials.h"

#define NORM_SIZE 1.5f

class StaticObject {
public:
	StaticObject(std::string name, uint8_t swizzle, float& scaleFac) {
		mesh = new StaticMesh(name, swizzle, scaleFac);
	}

	void setMat(Material* m, std::string shader) {
		mat = m;
		unlitMat = m;
		shaderName = shader;
	}

	bool isVisible = true;
	bool isWireframeVisible = true;
	StaticMesh* mesh = nullptr;
	Material* mat = nullptr;
	Material* unlitMat = nullptr;

	std::string objectName = "";
	std::string materialName = "Webcam Material";
	std::string shaderName = "BF";
};

class PlaneObject {
public:
	PlaneObject(uint32_t width, uint32_t height) {
		mesh = new PlaneMesh(width, height);
	}

	bool isVisible = true;
	bool isWireframeVisible = true;
	PlaneMesh* mesh = nullptr;
	Material* mat = nullptr;
};


#endif
