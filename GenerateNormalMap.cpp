#include "GenerateNormalMap.h"
#include"include/ShaderDataType.h"
#include"include/OSToTSConverter.h"
#include"include/TSToOSConverter.h"

using namespace cv;
using namespace std;

void NormalGen::copyOSImage(Texture* srcTex) {
	refOS = srcTex;
}

void NormalGen::copyTSImage(Texture* srcTex) {
	refTS = srcTex;
}

void NormalGen::prepareTSMap() {
	if (refOS == nullptr) {
		throw runtime_error("Cannot convert to tangent-space map without any object-space map!");
	}
	if (OStoTSConverter != nullptr) {
		OStoTSConverter->cleanup();
	}
	OStoTSConverter = new textureWriter(refOS, new OSTOTSCONVERTERSHADER, false, true, true, true);
	OStoTSConverter->setClearColour(glm::vec4{ 0.5f, 0.5f, 1.0f, 0.0f });
}

Texture* NormalGen::convertOStoTS(Mesh* mesh) {
	return OStoTSConverter->drawMap(std::vector<Mesh*>{mesh});
}

void NormalGen::cleanupOStoTS() {
	OStoTSConverter->cleanup();
	delete OStoTSConverter;
}

void NormalGen::prepareOSMap() {
	if (refTS == nullptr) {
		throw runtime_error("Cannot convert to object-space map without any tangent-space map!");
	}
	if (TStoOSConverter != nullptr) {
		TStoOSConverter->cleanup();
	}
	TStoOSConverter = new textureWriter(refTS, new TSTOOSCONVERTERSHADER, false, true, true);
}

Texture* NormalGen::convertTStoOS(Mesh* mesh) {
	return TStoOSConverter->drawMap(std::vector<Mesh*>{mesh});
}

void NormalGen::cleanupTStoOS() {
	TStoOSConverter->cleanup();
	delete TStoOSConverter;
}

void NormalGen::prepGenerateOSMap() {
	if (refTex == nullptr) {
		refTex = new Texture;
		refTex->texWidth = DEFAULTMAPDIM;
		refTex->texHeight = DEFAULTMAPDIM;
		refTex->texChannels = 4;
		refTex->mipLevels = 1;
		refTex->textureFormat = MAP_COLOUR_FORMAT;
		refTex->textureLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		refTex->textureUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		refTex->createImage(VK_SAMPLE_COUNT_1_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		refTex->textureImageView = refTex->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
	}

	if (OStexGenerator != nullptr) {
		OStexGenerator->cleanup();
	}
	OStexGenerator = new textureWriter(refTex, new NORMALGENERATORSHADER, false, false);
}

Texture* NormalGen::drawOSMap(Mesh* mesh) {
	return OStexGenerator->drawMap(std::vector<Mesh*>{mesh});
}

void NormalGen::cleanupGenOS() {
	vkDeviceWaitIdle(Engine::get()->device);

	refTex->cleanup();
	delete refTex;
	refTex = nullptr;
	OStexGenerator->cleanup();
	delete OStexGenerator;
	OStexGenerator = nullptr;
}