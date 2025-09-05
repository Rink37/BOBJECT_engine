#include"SurfaceConstructor.h"
#include"GenerateNormalMap.h"
#include"ImageProcessor.h"
#include"WindowsFileManager.h"

#include"include/Kuwahara.h"
#include"include/SobelX.h"
#include"include/SobelY.h"
#include"include/ReferenceKuwahara.h"

using namespace std;
using namespace cv;

void surfaceConstructor::generateOSMap(Mesh* inputMesh) {

	// This function can potentially be simplified using default image operations

	NormalGen generator(loadList);
	generator.setupOSExtractor();
	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
	commandBuffer = generator.drawOSMap(commandBuffer, inputMesh);
	Engine::get()->endSingleTimeCommands(commandBuffer);
	vkDestroyBuffer(Engine::get()->device, inputMesh->texCoordIndexBuffer, nullptr);
	vkFreeMemory(Engine::get()->device, inputMesh->texCoordIndexBufferMemory, nullptr);

	// see https://github.com/SaschaWillems/Vulkan/blob/master/examples/screenshot/screenshot.cpp 

	bool supportsBlit = true;

	VkFormatProperties formatProps;

	vkGetPhysicalDeviceFormatProperties(Engine::get()->physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
	if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
		std::cerr << "Device does not support blitting from optimal tiled images, using copy instead of blit!" << std::endl;
		supportsBlit = false;
	}

	vkGetPhysicalDeviceFormatProperties(Engine::get()->physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
	if (!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
		std::cerr << "Device does not support blitting to linear tiled images, using copy instead of blit!" << std::endl;
		supportsBlit = false;
	}

	OSNormTex = loadList->replacePtr(new imageTexture, "OSMapTex");
	OSNormTex->texWidth = generator.objectSpaceMap.colour->texWidth;
	OSNormTex->texHeight = generator.objectSpaceMap.colour->texHeight;
	OSNormTex->textureFormat = VK_FORMAT_R8G8B8A8_UNORM;
	OSNormTex->textureUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	OSNormTex->textureTiling = VK_IMAGE_TILING_LINEAR;
	OSNormTex->mipLevels = 1;

	OSNormTex->createImage(VK_SAMPLE_COUNT_1_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	VkCommandBuffer copyCmd;

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = Engine::get()->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(Engine::get()->device, &allocInfo, &copyCmd) != VK_SUCCESS) {
		throw runtime_error("failed to allocate command buffer!");
	}

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(copyCmd, &beginInfo) != VK_SUCCESS) {
		throw runtime_error("failed to begin recording command buffer!");
	}

	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcAccessMask = 0;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.image = OSNormTex->textureImage;
	imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	vkCmdPipelineBarrier(
		copyCmd,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);

	imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	imageMemoryBarrier.image = generator.objectSpaceMap.colour->textureImage;
	imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	vkCmdPipelineBarrier(
		copyCmd,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);

	if (supportsBlit)
	{
		VkOffset3D blitSize{};
		blitSize.x = generator.objectSpaceMap.colour->texWidth;
		blitSize.y = generator.objectSpaceMap.colour->texHeight;
		blitSize.z = 1;
		VkImageBlit imageBlitRegion{};
		imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.srcSubresource.layerCount = 1;
		imageBlitRegion.srcOffsets[1] = blitSize;
		imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.dstSubresource.layerCount = 1;
		imageBlitRegion.dstOffsets[1] = blitSize;

		vkCmdBlitImage(
			copyCmd,
			generator.objectSpaceMap.colour->textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			OSNormTex->textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageBlitRegion,
			VK_FILTER_NEAREST);
	}
	else
	{
		VkImageCopy imageCopyRegion{};
		imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.srcSubresource.layerCount = 1;
		imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.dstSubresource.layerCount = 1;
		imageCopyRegion.extent.width = generator.objectSpaceMap.colour->texWidth;
		imageCopyRegion.extent.height = generator.objectSpaceMap.colour->texHeight;
		imageCopyRegion.extent.depth = 1;

		vkCmdCopyImage(
			copyCmd,
			generator.objectSpaceMap.colour->textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			OSNormTex->textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageCopyRegion);
	}

	if (vkEndCommandBuffer(copyCmd) != VK_SUCCESS) {
		throw runtime_error("Failed to end command buffer");
	}

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &copyCmd;

	VkFence copyFence;
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	vkCreateFence(Engine::get()->device, &fenceInfo, nullptr, &copyFence);

	vkQueueSubmit(Engine::get()->graphicsQueue, 1, &submitInfo, copyFence);

	if (vkWaitForFences(Engine::get()->device, 1, &copyFence, VK_TRUE, UINT64_MAX) == VK_TIMEOUT) {
		throw runtime_error("Fence timeout");
	};

	vkFreeCommandBuffers(Engine::get()->device, Engine::get()->commandPool, 1, &copyCmd);

	vkDestroyFence(Engine::get()->device, copyFence, nullptr);

	OSNormTex->generateMipmaps();
	OSNormTex->textureImageView = OSNormTex->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

	//osNormMaterial.~osNormMaterial();
	//new (&osNormMaterial) Material(OSNormTex);

	Normal[1] = make_unique<Material>(OSNormTex);

	generator.cleanupOS();
}

void surfaceConstructor::contextConvert() {
	if (diffTex == nullptr || OSNormTex == nullptr) {
		return;
	}

	// Commented lines are used to save videos/images of the filter process (used for my tiktok video)

	// string baseName = "ConversionSec"; //

	// filter tempSobelX(diffTex, new SOBELXSHADER); //
	// tempSobelX.filterImage(); // 
	// filter tempSobelY(diffTex, new SOBELYSHADER); //
	// tempSobelY.filterImage(); //
	// Mat tempxgrad; // 
	// Mat tempygrad; //

	// tempSobelX.filterTarget[0]->getCVMat(); //
	// tempSobelY.filterTarget[0]->getCVMat(); //

	// cvtColor(tempSobelX.filterTarget[0]->texMat, tempxgrad, COLOR_RGB2GRAY); //
	// cvtColor(tempSobelY.filterTarget[0]->texMat, tempygrad, COLOR_RGB2GRAY); //

	// imwrite(baseName + string("NFXgrad.jpeg"), tempxgrad); //
	// imwrite(baseName + string("NFYgrad.jpeg"), tempygrad); // 

	filter Kuwahara(diffTex, new KUWAHARASHADER);
	Kuwahara.filterImage();

	// Kuwahara.filterTarget[0]->getCVMat(); //
	// Mat kuw = Kuwahara.filterTarget[0]->texMat.clone(); //

	// THIS IS A USEFUL OPENCV GAMMA CORRECTION FUNCTION - IT SHOULD BE INCLUDED IN THE TEXTURING
	
	// float gamma = 2.2; //
	// float invGamma = 1 / gamma; //

	// Mat table(1, 256, CV_8U); //
	// uchar* p = table.ptr(); //
	// for (int i = 0; i < 256; ++i) { //
	// 	p[i] = (uchar)(pow(i / 255.0, invGamma) * 255); //
	// } //

	// LUT(kuw, table, kuw); //

	// END OF GAMMA CORRECT //

	// imwrite(baseName + string("KuwFilteredDiff.jpeg"), kuw); // 
		
	filter SobelX(Kuwahara.filterTarget[0], new SOBELXSHADER);
	SobelX.filterImage();
	filter SobelY(Kuwahara.filterTarget[0], new SOBELYSHADER);
	SobelY.filterImage();
	Mat xgrad;
	Mat ygrad;

	SobelX.filterTarget[0]->getCVMat();
	SobelY.filterTarget[0]->getCVMat();

	cvtColor(SobelX.filterTarget[0]->texMat, xgrad, COLOR_RGB2GRAY);
	cvtColor(SobelY.filterTarget[0]->texMat, ygrad, COLOR_RGB2GRAY);

	// imwrite(baseName + string("FXgrad.jpeg"), xgrad); //
	// imwrite(baseName + string("FYgrad.jpeg"), ygrad); // 

	xgrad.convertTo(xgrad, CV_32F);
	ygrad.convertTo(ygrad, CV_32F);

	Kuwahara.cleanup();
	SobelX.cleanup();
	SobelY.cleanup();

	diffTex->getCVMat();

	int thresh = 15;

	OSNormTex->getCVMat();
	resize(OSNormTex->texMat, OSNormTex->texMat, Size(diffTex->texMat.cols, diffTex->texMat.rows));
	Mat convertedY = OSNormTex->texMat.clone();

	// VideoWriter convWriter; //
	// int codec = VideoWriter::fourcc('a', 'v', 'c', '1'); //
	// double fps = 30.0; //
	// string filename = baseName + string("Avg.mp4"); //
	// Size sizeFrame(convertedY.cols, convertedY.rows); //
	// convWriter.open(filename, codec, fps, sizeFrame, 1); //

	for (int x = 0; x != convertedY.cols; x++) {
		vector<Vec3f> colours;
		vector<uint32_t> indexes;
		vector<uint32_t> gradindexes;
		for (int y = 0; y != convertedY.rows; y++) {
			convertedY.at<Vec3b>(y, x) = Vec3b(0, 0, 0);
			if (ygrad.at<float>(y, x) < thresh && OSNormTex->texMat.at<Vec3b>(y, x) != Vec3b(0, 0, 0)) {
				colours.push_back(static_cast<Vec3f>(OSNormTex->texMat.at<Vec3b>(y, x)));
				indexes.push_back(y);
				ygrad.at<float>(y, x) = 0;
				if (gradindexes.size() > 0) {
					vector<float> cs;
					cs.push_back(ygrad.at<float>(gradindexes[0], x));
					for (size_t k = 1; k != gradindexes.size(); k++) {
						cs.push_back(cs[k - 1] + ygrad.at<float>(gradindexes[k], x));
					}
					float max = cs[cs.size() - 1];
					for (size_t k = 0; k != cs.size(); k++) {
						ygrad.at<float>(gradindexes[k], x) = cs[k]/max;
					}
					gradindexes.clear();
				}
			}
			else {
				gradindexes.push_back(y);
				if (indexes.size() > 0) {
					Vec3f avg = Vec3f(0.0f, 0.0f, 0.0f);
					for (int k = 0; k != indexes.size(); k++) {
						avg += colours[k];
					}
					avg = Vec3f(avg[0] / colours.size(), avg[1] / colours.size(), avg[2] / colours.size());
					for (int k : indexes) {
						convertedY.at<Vec3b>(k, x) = Vec3b(static_cast<uint8_t>(avg[0]), static_cast<uint8_t>(avg[1]), static_cast<uint8_t>(avg[2]));
						ygrad.at<float>(k, x) = 0.0f;
					}
					indexes.clear();
					colours.clear();
				}
			}
		}
		// if (x % 10 == 0) {
		// 	convWriter.write(convertedY); //
		// }
	}

	Mat converted = convertedY.clone();

	for (int y = 0; y != converted.rows; y++) {
		vector<Vec3f> colours;
		vector<uint32_t> indexes;
		vector<uint32_t> gradindexes;
		for (int x = 0; x != converted.cols; x++) {
			converted.at<Vec3b>(y, x) = Vec3b(0, 0, 0);
			if (xgrad.at<float>(y, x) < thresh && OSNormTex->texMat.at<Vec3b>(y, x) != Vec3b(0, 0, 0)) {
				if (ygrad.at<float>(y, x) == 0) {
					colours.push_back(static_cast<Vec3f>(convertedY.at<Vec3b>(y, x)));
					indexes.push_back(x);
				}
				xgrad.at<float>(y, x) = 0;
				if (gradindexes.size() > 0) {
					vector<float> cs;
					cs.push_back(xgrad.at<float>(y, gradindexes[0]));
					for (size_t k = 1; k != gradindexes.size(); k++) {
						cs.push_back(cs[k - 1] + xgrad.at<float>(y, gradindexes[k]));
					}
					float max = cs[cs.size() - 1];
					for (size_t k = 0; k != cs.size(); k++) {
						xgrad.at<float>(y, gradindexes[k]) = cs[k] / max;
					}
					gradindexes.clear();
				}
			}
			else {
				gradindexes.push_back(x);
				if (indexes.size() > 0) {
					Vec3f avg = Vec3f(0.0f, 0.0f, 0.0f);
					for (int k = 0; k != indexes.size(); k++) {
						avg += colours[k];
					}
					avg = Vec3f(avg[0] / colours.size(), avg[1] / colours.size(), avg[2] / colours.size());
					for (int k : indexes) {
						converted.at<Vec3b>(y, k) = Vec3b(static_cast<uint8_t>(avg[0]), static_cast<uint8_t>(avg[1]), static_cast<uint8_t>(avg[2]));
						xgrad.at<float>(y, k) = 0.0f;
					}
					indexes.clear();
					colours.clear();
				}
			}
		}
		// if (y % 10 == 0) {
		// 	convWriter.write(converted); //
		// }
	}

	// convWriter.release(); //

	convertedY.release();

	Mat scaleFacs = xgrad.clone();

	// VideoWriter smootheWriter; //
	// filename = baseName + string("Smoother.mp4"); //
	// smootheWriter.open(filename, codec, fps, sizeFrame, 1); //

	for (int y = 0; y != converted.rows; y++) {
		vector<uint32_t> indexes;
		Vec3b startColour = converted.at<Vec3b>(y, 0);
		Vec3b endColour = startColour;
		for (int x = 0; x != converted.cols; x++) {
			scaleFacs.at<float>(y, x) = 0.0f;
			if (OSNormTex->texMat.at<Vec3b>(y, x) == Vec3b(0, 0, 0)) {
				continue;
			}
			if (startColour == Vec3b(0, 0, 0) && converted.at<Vec3b>(y, x) != Vec3b(0, 0, 0)) {
				startColour = converted.at<Vec3b>(y, x);
			}
			if (xgrad.at<float>(y, x) != 0 || converted.at<Vec3b>(y, x) == Vec3b(0, 0, 0)) {
				indexes.push_back(x);
			}
			else {
				if (indexes.size() > 0) {
					endColour = converted.at<Vec3b>(y, x);
					if (endColour == Vec3b(0, 0, 0)) {
						endColour = startColour;
					}
					for (int index : indexes) {
						float xg = xgrad.at<float>(y, index);
						float yg = ygrad.at<float>(y, index);
						float sf;
						if (xg == 0) {
							sf = 1.0f;
						}
						else {
							sf = xg / (xg + yg);
						}
						converted.at<Vec3b>(y, index) += static_cast<Vec3b>((static_cast<Vec3f>(startColour) * (1.0f - xg) + static_cast<Vec3f>(endColour) * xg) * sf);
						scaleFacs.at<float>(y, index) += sf;
					}
					indexes.clear();
					startColour = endColour;
				}
			}
		}
		// if (y % 10 == 0) {
		// 	smootheWriter.write(converted); //
		// }
	}

	for (int x = 0; x != converted.cols; x++) {
		vector<uint32_t> indexes;
		Vec3b startColour = converted.at<Vec3b>(0, x);
		Vec3b endColour = startColour;
		for (int y = 0; y != converted.rows; y++) {
			if (OSNormTex->texMat.at<Vec3b>(y, x) == Vec3b(0, 0, 0)) {
				continue;
			}
			if (startColour == Vec3b(0, 0, 0) && converted.at<Vec3b>(y, x) != Vec3b(0, 0, 0)) {
				startColour = converted.at<Vec3b>(y, x);
			}
			if (ygrad.at<float>(y, x) != 0 || converted.at<Vec3b>(y, x) == Vec3b(0, 0, 0)) {
				indexes.push_back(y);
			}
			else {
				if (indexes.size() > 0) {
					endColour = converted.at<Vec3b>(y, x);
					if (endColour == Vec3b(0, 0, 0)) {
						endColour = startColour;
					}
					for (int index : indexes) {
						float xg = xgrad.at<float>(index, x);
						float yg = ygrad.at<float>(index, x);
						float sf;
						if (yg == 0) {
							sf = 1;
						}
						else {
							sf = yg / (xg + yg);
						}
						Vec3f value = static_cast<Vec3f>(converted.at<Vec3b>(index, x)) + (static_cast<Vec3f>(startColour) * (1.0f - yg) + static_cast<Vec3f>(endColour) * yg) * sf;
						value /= (sf + scaleFacs.at<float>(index, x));
						converted.at<Vec3b>(index, x) = static_cast<Vec3b>(value);
					}
					indexes.clear();
					startColour = endColour;
				}
			}
		}
		// if (x % 10 == 0) {
		// 	smootheWriter.write(converted); //
		// }
	}

	// smootheWriter.release(); //

	// imwrite(baseName + string("Noisy.jpeg"), converted); //

	scaleFacs.release();
	xgrad.release();
	ygrad.release();

	Mat grayConverted;
	Mat grayDilated;
	Mat kernel = getStructuringElement(MORPH_RECT, Size(100, 100));
	cvtColor(converted, grayConverted, COLOR_BGR2GRAY);
	inRange(grayConverted, 1, 255, grayConverted);
	dilate(grayConverted, grayDilated, kernel);
	subtract(grayDilated, grayConverted, grayConverted);

	inpaint(converted, grayConverted, converted, 1.0, INPAINT_TELEA);

	grayDilated.release();
	grayConverted.release();

	filter referenceKuwahara(diffTex, new imageTexture(converted, VK_FORMAT_R8G8B8A8_UNORM), new REFERENCEKUWAHARASHADER);
	referenceKuwahara.filterImage();
	referenceKuwahara.filterTarget[0]->getCVMat();

	normalType = 0;
	loadNormal(new imageTexture(referenceKuwahara.filterTarget[0]->texMat(Range(0, converted.rows), Range(0, converted.cols)), VK_FORMAT_R8G8B8A8_UNORM));
	updateSurfaceMat();

	referenceKuwahara.cleanup();
	diffTex->destroyCVMat();
	OSNormTex->destroyCVMat();
}

//void surfaceConstructor::contextConvert() {
//	if (diffTex == nullptr || OSNormTex == nullptr) {
//		// We need both of these images to perform context conversion
//		return;
//	}
//	// OpenCV functions are used for conversion so we use CV matrices instead of engine images
//	OSNormTex->getCVMat();
//	diffTex->getCVMat();
//	NormalGen generator;
//	generator.OSNormalMap = OSNormTex->texMat;
//	generator.contextualConvertMap(diffTex->texMat);
//	//imageTexture* convertedOS = new imageTexture(generator.OSNormalMap);
//	//loadNormal(convertedOS);
//}

void surfaceConstructor::transitionToTS(Mesh* inputMesh) {
	NormalGen generator(loadList);
	OSNormTex->getCVMat();
	generator.createOSImageFromMat(OSNormTex->texMat);
	generator.setupTSExtractor();
	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
	commandBuffer = generator.convertOStoTS(commandBuffer, inputMesh);
	Engine::get()->endSingleTimeCommands(commandBuffer);
	vkDestroyBuffer(Engine::get()->device, inputMesh->texCoordIndexBuffer, nullptr);
	vkFreeMemory(Engine::get()->device, inputMesh->texCoordIndexBufferMemory, nullptr);

	// see https://github.com/SaschaWillems/Vulkan/blob/master/examples/screenshot/screenshot.cpp 

	bool supportsBlit = true;

	VkFormatProperties formatProps;

	vkGetPhysicalDeviceFormatProperties(Engine::get()->physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
	if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
		std::cerr << "Device does not support blitting from optimal tiled images, using copy instead of blit!" << std::endl;
		supportsBlit = false;
	}

	vkGetPhysicalDeviceFormatProperties(Engine::get()->physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
	if (!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
		std::cerr << "Device does not support blitting to linear tiled images, using copy instead of blit!" << std::endl;
		supportsBlit = false;
	}

	TSNormTex = loadList->getPtr(new imageTexture, "TSMapTex");
	TSNormTex->texWidth = generator.tangentSpaceMap.colour->texWidth;
	TSNormTex->texHeight = generator.tangentSpaceMap.colour->texHeight;
	TSNormTex->textureFormat = VK_FORMAT_R8G8B8A8_UNORM;
	TSNormTex->textureUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	TSNormTex->textureTiling = VK_IMAGE_TILING_LINEAR;
	TSNormTex->mipLevels = 1;

	TSNormTex->createImage(VK_SAMPLE_COUNT_1_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	VkCommandBuffer copyCmd;

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = Engine::get()->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(Engine::get()->device, &allocInfo, &copyCmd) != VK_SUCCESS) {
		throw runtime_error("failed to allocate command buffer!");
	}

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(copyCmd, &beginInfo) != VK_SUCCESS) {
		throw runtime_error("failed to begin recording command buffer!");
	}

	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcAccessMask = 0;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.image = TSNormTex->textureImage;
	imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	vkCmdPipelineBarrier(
		copyCmd,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);

	imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	imageMemoryBarrier.image = generator.tangentSpaceMap.colour->textureImage;
	imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	vkCmdPipelineBarrier(
		copyCmd,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);

	if (supportsBlit)
	{
		VkOffset3D blitSize{};
		blitSize.x = generator.tangentSpaceMap.colour->texWidth;
		blitSize.y = generator.tangentSpaceMap.colour->texHeight;
		blitSize.z = 1;
		VkImageBlit imageBlitRegion{};
		imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.srcSubresource.layerCount = 1;
		imageBlitRegion.srcOffsets[1] = blitSize;
		imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.dstSubresource.layerCount = 1;
		imageBlitRegion.dstOffsets[1] = blitSize;

		vkCmdBlitImage(
			copyCmd,
			generator.tangentSpaceMap.colour->textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			TSNormTex->textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageBlitRegion,
			VK_FILTER_NEAREST);
	}
	else
	{
		VkImageCopy imageCopyRegion{};
		imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.srcSubresource.layerCount = 1;
		imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.dstSubresource.layerCount = 1;
		imageCopyRegion.extent.width = generator.tangentSpaceMap.colour->texWidth;
		imageCopyRegion.extent.height = generator.tangentSpaceMap.colour->texHeight;
		imageCopyRegion.extent.depth = 1;

		vkCmdCopyImage(
			copyCmd,
			generator.tangentSpaceMap.colour->textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			TSNormTex->textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageCopyRegion);
	}

	if (vkEndCommandBuffer(copyCmd) != VK_SUCCESS) {
		throw runtime_error("Failed to end command buffer");
	}

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &copyCmd;

	VkFence copyFence;
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	vkCreateFence(Engine::get()->device, &fenceInfo, nullptr, &copyFence);

	vkQueueSubmit(Engine::get()->graphicsQueue, 1, &submitInfo, copyFence);

	if (vkWaitForFences(Engine::get()->device, 1, &copyFence, VK_TRUE, UINT64_MAX) == VK_TIMEOUT) {
		throw runtime_error("Fence timeout");
	};

	vkFreeCommandBuffers(Engine::get()->device, Engine::get()->commandPool, 1, &copyCmd);

	vkDestroyFence(Engine::get()->device, copyFence, nullptr);

	TSNormTex->generateMipmaps();
	TSNormTex->textureImageView = TSNormTex->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

	//tsNormMaterial.~tsNormMaterial();
	//new (&tsNormMaterial) Material(TSNormTex);

	Normal[2] = make_unique<Material>(TSNormTex);
	generator.cleanupTS();
}

void SurfaceMenu::setup(surfaceConstructor* surfConst, std::vector<StaticObject>* objects) {

	sConst = surfConst;
	sConst->loadList = loadList;
	staticObjects = objects;

	std::function<void(UIItem*)> addNormalButton = bind(&SurfaceMenu::createNormalMenu, this, placeholders::_1);
	std::function<void(UIItem*)> toggleDiffuse = bind(&SurfaceMenu::toggleDiffuseCam, this, placeholders::_1);
	std::function<void(UIItem*)> loadDiffuse = bind(&SurfaceMenu::loadDiffuseImage, this, placeholders::_1);
	std::function<void(UIItem*)> saveWebcam = bind(&SurfaceMenu::saveDiffuseImage, this, placeholders::_1);

	imageData diffuse = DIFFUSETEXT;
	Material* diffuseMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&diffuse), "DiffuseBtnTex")), "DiffuseBtnMat");

	imageData normal = NORMALTEXT;
	Material* normalMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&normal), "NormalBtnTex")), "NormalBtnMat");

	imageData webcamOn = WEBCAMONBUTTON;
	Material* webcamOnMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&webcamOn), "WebcamOnBtnTex")), "WebcamOnBtnMat");

	imageData webcamOff = WEBCAMOFFBUTTON;
	Material* webcamOffMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&webcamOff), "WebcamOnBtnTex")), "WebcamOnBtnMat");

	imageData OpenButton = OPENBUTTON;
	Material* openMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&OpenButton), "OpenBtnTex")), "OpenBtnMat");

	imageData SaveButton = SAVEBUTTON;
	Material* saveMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&SaveButton), "SaveBtnTex")), "SaveBtnMat");

	imageData plusButton = PLUSBUTTON;
	Material* plusMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&plusButton), "PlusBtnTex")), "PlusBtnMat");

	Button* diffuseTextPanel = new Button(diffuseMat);

	diffuseTog = new Checkbox(webcamOnMat, webcamOffMat, toggleDiffuse);
	diffuseTog->Name = "ToggleDiffuseWebcam";
	diffuseTog->setClickFunction(toggleDiffuse);

	Button* diffLoad = new Button(openMat, loadDiffuse);
	Button* diffSave = new Button(saveMat, saveWebcam);

	hArrangement* DiffuseButtons = new hArrangement(0.0f, 0.0f, 1.0f, 0.2f, 0.01f);

	Button* normalTextPanel = new Button(normalMat);
	Button* normalPlus = new Button(plusMat, addNormalButton);

	NormalButtons = new hArrangement(0.0f, 0.0f, 1.0f, 0.2f, 0.01f);

	NormalButtons->addItem(getPtr(normalTextPanel));
	NormalButtons->addItem(getPtr(normalPlus));
	NormalButtons->addItem(getPtr(new spacer));

	DiffuseButtons->addItem(getPtr(diffuseTextPanel));
	DiffuseButtons->addItem(getPtr(diffuseTog));
	DiffuseButtons->addItem(getPtr(new spacer));
	DiffuseButtons->addItem(getPtr(diffLoad));
	DiffuseButtons->addItem(getPtr(diffSave));

	SurfacePanel = new vArrangement(1.0f, 0.0f, 0.25f, 0.8f, 0.01f);

	diffuseView = new ImagePanel(sConst->currentDiffuse().get(), true);
	SurfacePanel->addItem(getPtr(DiffuseButtons));
	SurfacePanel->addItem(getPtr(diffuseView));
	SurfacePanel->addItem(getPtr(NormalButtons));
	SurfacePanel->addItem(getPtr(new spacer));

	SurfacePanel->updateDisplay();

	canvas.push_back(getPtr(SurfacePanel));

	hasNormal = false;
}

void SurfaceMenu::removeNormalMenu(UIItem* owner) {
	if (!hasNormal) {
		return;
	}
	// Clear UI related to the normal component of the surface panel
	SurfacePanel->Items[SurfacePanel->Items.size() - 2]->cleanup();
	SurfacePanel->removeItem(static_cast<uint32_t>(SurfacePanel->Items.size() - 2));

	NormalButtons->cleanup();
	NormalButtons->Items.clear();

	std::function<void(UIItem*)> addNormalButton = bind(&SurfaceMenu::createNormalMenu, this, placeholders::_1);

	imageData normal = NORMALTEXT;
	Material* normalMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&normal), "NormalBtnTex")), "NormalBtnMat");

	imageData plusButton = PLUSBUTTON;
	Material* plusMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&plusButton), "PlusBtnTex")), "PlusBtnMat");

	Button* normalTextPanel = new Button(normalMat);
	Button* normalPlus = new Button(plusMat, addNormalButton);

	NormalButtons->addItem(getPtr(normalTextPanel));
	NormalButtons->addItem(getPtr(normalPlus));
	NormalButtons->addItem(getPtr(new spacer));

	SurfacePanel->updateDisplay();
	hasNormal = false;
}

void SurfaceMenu::toggleDiffuseCam(UIItem* owner) {
	sConst->toggleDiffWebcam();
	diffuseView->image->mat[0] = sConst->currentDiffuse().get();
}

void SurfaceMenu::loadDiffuseImage(UIItem* owner) {
	string fileName = winFile::OpenFileDialog();
	if (fileName != string("fail")) {
		imageTexture* loadedTexture = new imageTexture(fileName, VK_FORMAT_R8G8B8A8_SRGB);
		sConst->loadDiffuse(loadedTexture);
		sConst->diffuseIdx = 1;
		diffuseView->image->mat[0] = sConst->currentDiffuse().get();
		diffuseView->image->texHeight = diffuseView->image->mat[0]->textures[0]->texHeight;
		diffuseView->image->texWidth = diffuseView->image->mat[0]->textures[0]->texWidth;
		diffuseView->sqAxisRatio = static_cast<float>(diffuseView->image->texHeight) / static_cast<float>(diffuseView->image->texWidth);
		diffuseTog->activestate = false;
		diffuseTog->image->matidx = 1;
		sConst->updateSurfaceMat();
		SurfacePanel->arrangeItems();
		session::get()->currentStudio.diffusePath = fileName;
	}
}

void SurfaceMenu::saveDiffuseImage(UIItem* owner) {
	Mat saveDiffuse;
	if (sConst->diffuseIdx == 0) {
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
	normalView->image->mat[0] = sConst->currentNormal().get();
}

void SurfaceMenu::toggleNormalType(UIItem* owner) {
	sConst->toggleNormType();
	if (sConst->normalType == 1 && sConst->OSNormTex != nullptr && !sConst->TSmatching) {
		if (sConst->TSNormTex != nullptr) {
			sConst->TSNormTex->cleanup();
		}
		sConst->transitionToTS((*staticObjects)[staticObjects->size() - 1].mesh);
		sConst->TSmatching = true;
		normalView->image->mat[0] = sConst->currentNormal().get();
	}
	else {
		normalView->image->mat[0] = sConst->currentNormal().get();
	}
}

void SurfaceMenu::loadNormalImage(UIItem* owner) {
	string fileName = winFile::OpenFileDialog();
	if (fileName != string("fail")) {
		imageTexture* loadedTexture = new imageTexture(fileName, VK_FORMAT_R8G8B8A8_UNORM);
		sConst->loadNormal(loadedTexture);
		sConst->normalIdx = 1 + sConst->normalType;
		normalView->image->mat[0] = sConst->currentNormal().get();
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
		if (sConst->normalIdx == 1) {
			session::get()->currentStudio.OSPath = saveName;
		}
		else if (sConst->normalIdx == 2) {
			session::get()->currentStudio.TSPath = saveName;
		}
		imwrite(saveName, saveNormal);
	}
}

void SurfaceMenu::contextConvertMap(UIItem* owner) {
	sConst->contextConvert();
	sConst->normalIdx = 1 + sConst->normalType;
	normalView->image->mat[0] = sConst->currentNormal().get();
	normalTog->activestate = false;
	normalTog->image->matidx = 1;
}

void SurfaceMenu::createNormalMenu(UIItem* owner) {

	if (staticObjects->size() == 0) {
		return;
	}

	std::function<void(UIItem*)> toggleWebcam = bind(&SurfaceMenu::toggleNormalCam, this, placeholders::_1);
	std::function<void(UIItem*)> toggleType = bind(&SurfaceMenu::toggleNormalType, this, placeholders::_1);
	std::function<void(UIItem*)> saveNorm = bind(&SurfaceMenu::saveNormalImage, this, placeholders::_1);
	std::function<void(UIItem*)> loadNorm = bind(&SurfaceMenu::loadNormalImage, this, placeholders::_1);
	std::function<void(UIItem*)> convertImg = bind(&SurfaceMenu::contextConvertMap, this, placeholders::_1);

	sConst->normalType = 0;
	if (sConst->OSNormTex == nullptr) {
		sConst->generateOSMap((*staticObjects)[staticObjects->size() - 1].mesh);
	}
	sConst->normalAvailable = true;
	webcamTexture::get()->changeFormat(VK_FORMAT_R8G8B8A8_UNORM);
	sConst->normalIdx = 1;
	sConst->updateSurfaceMat();

	SurfacePanel->removeItem(3);

	vector<UIImage*> images;
	NormalButtons->getImages(images);

	for (UIImage* image : images) {
		image->cleanup();
	}

	NormalButtons->Items.clear();

	imageData normal = NORMALTEXT;
	Material* normalMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&normal), "NormalBtnTex")), "NormalBtnMat");

	imageData webcamOn = WEBCAMONBUTTON;
	Material* webcamOnMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&webcamOn), "WebcamOnBtnTex")), "WebcamOnBtnMat");

	imageData webcamOff = WEBCAMOFFBUTTON;
	Material* webcamOffMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&webcamOff), "WebcamOnBtnTex")), "WebcamOnBtnMat");

	imageData OpenButton = OPENBUTTON;
	Material* openMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&OpenButton), "OpenBtnTex")), "OpenBtnMat");

	imageData SaveButton = SAVEBUTTON;
	Material* saveMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&SaveButton), "SaveBtnTex")), "SaveBtnMat");

	imageData plusButton = PLUSBUTTON;
	Material* plusMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&plusButton), "PlusBtnTex")), "PlusBtnMat");

	imageData osType = OSBUTTON;
	Material* osMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&osType), "OSBtnTex")), "OSBtnMat");

	imageData tsType = TANGENTSPACE;
	Material* tsMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&tsType), "TSBtnTex")), "TSBtnMat");

	imageData diffToNorm = D2NBUTTON;
	Material* dtnMat = loadList->getPtr(new Material(loadList->getPtr(new imageTexture(&diffToNorm), "DiffToNormBtnTex")), "DiffToNormBtnMat");

	Button* normalText = new Button(normalMat);

	normalTog = new Checkbox(webcamOnMat, webcamOffMat, toggleWebcam);
	normalTog->activestate = false;
	normalTog->image->matidx = 1;

	Checkbox* mapTypeToggle = new Checkbox(osMat, tsMat, toggleType);
	Button* copyLayout = new Button(dtnMat, convertImg);
	Button* normalLoad = new Button(openMat, loadNorm);
	Button* normalSave = new Button(saveMat, saveNorm);

	NormalButtons->addItem(getPtr(normalText));
	NormalButtons->addItem(getPtr(normalTog));
	NormalButtons->addItem(getPtr(mapTypeToggle));
	NormalButtons->addItem(getPtr(copyLayout));
	NormalButtons->addItem(getPtr(normalLoad));
	NormalButtons->addItem(getPtr(normalSave));

	normalView = new ImagePanel(sConst->currentNormal().get(), true);
	normalView->image->texHeight = diffuseView->image->texHeight;
	normalView->image->texWidth = diffuseView->image->texWidth;
	normalView->sqAxisRatio = static_cast<float>(normalView->image->texHeight) / static_cast<float>(normalView->image->texWidth);

	SurfacePanel->addItem(getPtr(normalView));
	SurfacePanel->addItem(getPtr(new spacer));
	SurfacePanel->updateDisplay();

	hasNormal = true;
}