#include"Meshes.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include"tiny_obj_loader.h"

using namespace std;

void Mesh::createVertexBuffer() {
	Engine* engine = Engine::get();

	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	engine->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(engine->device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(engine->device, stagingBufferMemory);

	engine->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

	engine->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(engine->device, stagingBuffer, nullptr);
	vkFreeMemory(engine->device, stagingBufferMemory, nullptr);
}

void Mesh::createIndexBuffer() {
	Engine* engine = Engine::get();

	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	engine->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(engine->device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(engine->device, stagingBufferMemory);

	engine->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

	engine->copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(engine->device, stagingBuffer, nullptr);
	vkFreeMemory(engine->device, stagingBufferMemory, nullptr);
}

void Mesh::getStripChain(SeamStrip& strip, uint32_t nodeIndex, std::vector<std::array<uint32_t, 2>>& vertexPairs, std::vector<uint32_t>& deleteIndices) {
	if (nodeIndex >= vertexPairs.size() - 1 || find(deleteIndices.begin(), deleteIndices.end(), nodeIndex) != deleteIndices.end()) {
		return;
	}
	//std::cout << nodeIndex << " " << vertexPairs.size() << std::endl;

	std::array<uint32_t, 2> seamVert = vertexPairs[nodeIndex];

	strip.leftIndices.push_back(seamVert[0]);
	strip.rightIndices.push_back(seamVert[1]);

	deleteIndices.push_back(nodeIndex);

	std::vector<uint32_t> L_indexOccurrences{};
	std::vector<uint32_t> R_indexOccurrences{};
	auto L_it = find(indices.begin(), indices.end(), seamVert[0]);
	auto R_it = find(indices.begin(), indices.end(), seamVert[1]);
	while (L_it != indices.end()) {
		L_indexOccurrences.push_back(L_it - indices.begin());
		L_it = find(L_it + 1, indices.end(), seamVert[0]);
	}
	while (R_it != indices.end()) {
		R_indexOccurrences.push_back(R_it - indices.begin());
		R_it = find(R_it + 1, indices.end(), seamVert[1]);
	}

	std::vector<uint32_t> L_adjacentIndices{};
	std::vector<uint32_t> R_adjacentIndices{};
	for (uint32_t i = 0; i != L_indexOccurrences.size(); i++) {
		uint32_t L_Index = L_indexOccurrences[i];
		switch (L_Index % 3) {
		case (0):
			L_adjacentIndices.push_back(indices[L_Index + 1]);
			L_adjacentIndices.push_back(indices[L_Index + 2]);
			break;
		case (1):
			L_adjacentIndices.push_back(indices[L_Index - 1]);
			L_adjacentIndices.push_back(indices[L_Index + 1]);
			break;
		case (2):
			L_adjacentIndices.push_back(indices[L_Index - 2]);
			L_adjacentIndices.push_back(indices[L_Index - 1]);
			break;
		default:
			break;
		}
	}

	//std::cout << "Local indices to " << seamVert[0] << ":" << std::endl;
	//for (uint32_t i : L_adjacentIndices) {
	//	std::cout << i << " ";
	//}
	//std::cout << std::endl;

	for (uint32_t i = 0; i != R_indexOccurrences.size(); i++) {
		uint32_t R_Index = R_indexOccurrences[i];
		switch (R_Index % 3) {
		case (0):
			R_adjacentIndices.push_back(indices[R_Index + 1]);
			R_adjacentIndices.push_back(indices[R_Index + 2]);
			break;
		case (1):
			R_adjacentIndices.push_back(indices[R_Index - 1]);
			R_adjacentIndices.push_back(indices[R_Index + 1]);
			break;
		case (2):
			R_adjacentIndices.push_back(indices[R_Index - 2]);
			R_adjacentIndices.push_back(indices[R_Index - 1]);
			break;
		default:
			break;
		}
	}
	
	//std::cout << "Local indices to " << seamVert[1] << ":" << std::endl;
	//for (uint32_t i : R_adjacentIndices) {
	//	std::cout << i << " ";
	//}
	//std::cout << std::endl;


	std::vector<uint32_t> chainIndices{};

	for (uint32_t i = 0; i != vertexPairs.size(); i++) {
		if (find(deleteIndices.begin(), deleteIndices.end(), i) != deleteIndices.end()) {
			continue;
		}
		if (find(L_adjacentIndices.begin(), L_adjacentIndices.end(), vertexPairs[i][0]) != L_adjacentIndices.end() && find(R_adjacentIndices.begin(), R_adjacentIndices.end(), vertexPairs[i][1]) != R_adjacentIndices.end()) {
			chainIndices.push_back(i);
		}
		else if (find(L_adjacentIndices.begin(), L_adjacentIndices.end(), vertexPairs[i][1]) != L_adjacentIndices.end() && find(R_adjacentIndices.begin(), R_adjacentIndices.end(), vertexPairs[i][0]) != R_adjacentIndices.end()) {
			chainIndices.push_back(i);
			
			uint32_t vP0 = vertexPairs[i][0];
			vertexPairs[i][0] = vertexPairs[i][1];
			vertexPairs[i][1] = vP0;
		}
	}

	if (chainIndices.size() > 0) {
		for (uint32_t i : chainIndices) {
			getStripChain(strip, i, vertexPairs, deleteIndices);
		}
	}
}

void Mesh::createSeamMeshes(cv::Mat demoMat, SeamStrip& strip) {
	float distance = 0.01f;

	cv::Mat demoMatDupe = demoMat.clone();
	uint32_t width = demoMatDupe.size().width;
	uint32_t height = demoMatDupe.size().height;

	// First we do the left hand mesh
	// I'll start just by plotting points but later we can make this into an actual mesh

	std::map<uint32_t, std::vector<glm::vec2>> L_out{};
	std::map<uint32_t, std::vector<glm::vec2>> L_in{};

	for (uint32_t i = 0; i != strip.leftIndices.size() - 1; i++) {
		glm::vec2 texCoord_0 = vertices[strip.leftIndices[i]].texCoord;
		glm::vec2 texCoord_1 = vertices[strip.leftIndices[i + 1]].texCoord;

		std::vector<uint32_t> Occurences_0{};
		std::vector<uint32_t> Occurences_1{};

		auto it_0 = find(indices.begin(), indices.end(), strip.leftIndices[i]);
		auto it_1 = find(indices.begin(), indices.end(), strip.leftIndices[i + 1]);
		while (it_0 != indices.end()) {
			Occurences_0.push_back(it_0 - indices.begin());
			it_0 = find(it_0+1, indices.end(), strip.leftIndices[i]);
		}
		while (it_1 != indices.end()) {
			Occurences_1.push_back(it_1 - indices.begin());
			it_1 = find(it_1+1, indices.end(), strip.leftIndices[i+1]);
		}

		std::set<uint32_t> connectedIndices{};
		for (uint32_t j : Occurences_0) {
			switch (j % 3) {
			case (0):
				connectedIndices.insert(j + 1);
				connectedIndices.insert(j + 2);
				break;
			case (1):
				connectedIndices.insert(j - 1);
				connectedIndices.insert(j + 1);
				break;
			case (2):
				connectedIndices.insert(j - 2);
				connectedIndices.insert(j - 1);
				break;
			default:
				break;
			}
		}

		for (uint32_t j : Occurences_1) {
			switch (j % 3) {
			case (0):
				connectedIndices.insert(j + 1);
				connectedIndices.insert(j + 2);
				break;
			case (1):
				connectedIndices.insert(j - 1);
				connectedIndices.insert(j + 1);
				break;
			case (2):
				connectedIndices.insert(j - 2);
				connectedIndices.insert(j - 1);
				break;
			default:
				break;
			}
		}

		glm::vec2 thirdCoord{ 0, 0 };

		for (uint32_t j : Occurences_0) {
			auto it = find(connectedIndices.begin(), connectedIndices.end(), j);
			if (it != connectedIndices.end()) {
				connectedIndices.erase(it);
			}
		}

		for (uint32_t j : Occurences_1) {
			auto it = find(connectedIndices.begin(), connectedIndices.end(), j);
			if (it != connectedIndices.end()) {
				connectedIndices.erase(it);
			}
		}

		for (uint32_t j : connectedIndices) {
			thirdCoord += vertices[indices[j]].texCoord;
		}

		thirdCoord /= connectedIndices.size();

		cv::circle(demoMatDupe, cv::Point(thirdCoord.x * width, thirdCoord.y * height), 5, cv::Scalar(0, 0, 255), -1);

		float gradient = static_cast<float>(texCoord_1.y - texCoord_0.y) / static_cast<float>(texCoord_1.x - texCoord_0.x);
		float invGradient = -1.0f / gradient;

		glm::vec2 pointingVec = glm::vec2(1.0f, invGradient);
		pointingVec /= glm::length(pointingVec);

		glm::vec2 minCoord = -pointingVec * distance + texCoord_0;
		glm::vec2 addCoord = pointingVec * distance + texCoord_0;

		glm::vec2 outCoord;
		glm::vec2 inCoord;

		if (glm::length(minCoord - thirdCoord) < glm::length(addCoord - thirdCoord)) {
			outCoord = addCoord;
			inCoord = minCoord;
		}
		else {
			inCoord = addCoord;
			outCoord = minCoord;
		}

		if (L_out.count(i) == 0) {
			L_out.insert({ i, std::vector<glm::vec2>{outCoord} });
		}
		else {
			L_out.at(i).push_back(outCoord);
		}

		if (L_in.count(i) == 0) {
			L_in.insert({ i, std::vector<glm::vec2>{ inCoord} });
		}
		else {
			L_in.at(i).push_back(inCoord);
		}

		minCoord = -pointingVec * distance + texCoord_1;
		addCoord = pointingVec * distance + texCoord_1;

		if (glm::length(minCoord - thirdCoord) < glm::length(addCoord - thirdCoord)) {
			outCoord = addCoord;
			inCoord = minCoord;
		}
		else {
			inCoord = addCoord;
			outCoord = minCoord;
		}

		if (L_out.count(i+1) == 0) {
			L_out.insert({ i+1, std::vector<glm::vec2>{ outCoord } });
		}
		else {
			L_out.at(i+1).push_back(outCoord);
		}

		if (L_in.count(i + 1) == 0) {
			L_in.insert({ i + 1, std::vector<glm::vec2>{ inCoord } });
		}
		else {
			L_in.at(i + 1).push_back(inCoord);
		}
	}

	for (auto elem : L_out) {
		cv::Point coord(0.0f, 0.0f);
		if (elem.second.size() > 1) {
			glm::vec2 avgPoint{ 0.0f, 0.0f };
			for (glm::vec2 point : elem.second) {
				avgPoint += point;
			}
			avgPoint /= elem.second.size();
			coord.x = avgPoint.x * width;
			coord.y = avgPoint.y * height;
		}
		else {
			coord.x = elem.second[0].x * width;
			coord.y = elem.second[0].y * height;
		}
		cv::circle(demoMatDupe, cv::Point(vertices[strip.leftIndices[elem.first]].texCoord.x * width, vertices[strip.leftIndices[elem.first]].texCoord.y * height), 5, cv::Scalar(255, 0, 0), -1);
		cv::circle(demoMatDupe, coord, 5, cv::Scalar(255, 0, 0), -1);
	}

	for (auto elem : L_in) {
		cv::Point coord(0.0f, 0.0f);
		if (elem.second.size() > 1) {
			glm::vec2 avgPoint{ 0.0f, 0.0f };
			for (glm::vec2 point : elem.second) {
				avgPoint += point;
			}
			avgPoint /= elem.second.size();
			coord.x = avgPoint.x * width;
			coord.y = avgPoint.y * height;
		}
		else {
			coord.x = elem.second[0].x * width;
			coord.y = elem.second[0].y * height;
		}
		cv::circle(demoMatDupe, cv::Point(vertices[strip.leftIndices[elem.first]].texCoord.x * width, vertices[strip.leftIndices[elem.first]].texCoord.y * height), 5, cv::Scalar(0, 255, 0), -1);
		cv::circle(demoMatDupe, coord, 5, cv::Scalar(0, 255, 0), -1);
	}

	std::map<uint32_t, std::vector<glm::vec2>> R_out{};
	std::map<uint32_t, std::vector<glm::vec2>> R_in{};

	for (uint32_t i = 0; i != strip.rightIndices.size() - 1; i++) {
		glm::vec2 texCoord_0 = vertices[strip.rightIndices[i]].texCoord;
		glm::vec2 texCoord_1 = vertices[strip.rightIndices[i + 1]].texCoord;
		
		std::vector<uint32_t> Occurences_0{};
		std::vector<uint32_t> Occurences_1{};

		auto it_0 = find(indices.begin(), indices.end(), strip.rightIndices[i]);
		auto it_1 = find(indices.begin(), indices.end(), strip.rightIndices[i + 1]);
		while (it_0 != indices.end()) {
			Occurences_0.push_back(it_0 - indices.begin());
			it_0 = find(it_0 + 1, indices.end(), strip.rightIndices[i]);
		}
		while (it_1 != indices.end()) {
			Occurences_1.push_back(it_1 - indices.begin());
			it_1 = find(it_1 + 1, indices.end(), strip.rightIndices[i + 1]);
		}

		std::set<uint32_t> connectedIndices{};
		for (uint32_t j : Occurences_0) {
			switch (j % 3) {
			case (0):
				connectedIndices.insert(j + 1);
				connectedIndices.insert(j + 2);
				break;
			case (1):
				connectedIndices.insert(j - 1);
				connectedIndices.insert(j + 1);
				break;
			case (2):
				connectedIndices.insert(j - 2);
				connectedIndices.insert(j - 1);
				break;
			default:
				break;
			}
		}

		for (uint32_t j : Occurences_1) {
			switch (j % 3) {
			case (0):
				connectedIndices.insert(j + 1);
				connectedIndices.insert(j + 2);
				break;
			case (1):
				connectedIndices.insert(j - 1);
				connectedIndices.insert(j + 1);
				break;
			case (2):
				connectedIndices.insert(j - 2);
				connectedIndices.insert(j - 1);
				break;
			default:
				break;
			}
		}

		glm::vec2 thirdCoord{ 0, 0 };

		for (uint32_t j : Occurences_0) {
			auto it = find(connectedIndices.begin(), connectedIndices.end(), j);
			if (it != connectedIndices.end()) {
				connectedIndices.erase(it);
			}
		}

		for (uint32_t j : Occurences_1) {
			auto it = find(connectedIndices.begin(), connectedIndices.end(), j);
			if (it != connectedIndices.end()) {
				connectedIndices.erase(it);
			}
		}

		for (uint32_t j : connectedIndices) {
			thirdCoord += vertices[indices[j]].texCoord;
		}

		thirdCoord /= connectedIndices.size();

		cv::circle(demoMatDupe, cv::Point(thirdCoord.x * width, thirdCoord.y * height), 5, cv::Scalar(0, 0, 255), -1);
		
		float gradient = static_cast<float>(texCoord_1.y - texCoord_0.y) / static_cast<float>(texCoord_1.x - texCoord_0.x);
		float invGradient = -1.0f / gradient;

		glm::vec2 pointingVec = glm::vec2(1.0f, invGradient);
		pointingVec /= glm::length(pointingVec);

		glm::vec2 minCoord = -pointingVec * distance + texCoord_0;
		glm::vec2 addCoord = pointingVec * distance + texCoord_0;

		glm::vec2 outCoord;
		glm::vec2 inCoord;

		if (glm::length(minCoord - thirdCoord) < glm::length(addCoord - thirdCoord)) {
			outCoord = addCoord;
			inCoord = minCoord;
		}
		else {
			inCoord = addCoord;
			outCoord = minCoord;
		}


		if (R_out.count(i) == 0) {
			R_out.insert({ i, std::vector<glm::vec2>{ outCoord} });
		}
		else {
			R_out.at(i).push_back(outCoord);
		}

		if (R_in.count(i) == 0) {
			R_in.insert({ i, std::vector<glm::vec2>{ inCoord } });
		}
		else {
			R_in.at(i).push_back(inCoord);
		}

		minCoord = -pointingVec * distance + texCoord_1;
		addCoord = pointingVec * distance + texCoord_1;

		if (glm::length(minCoord - thirdCoord) < glm::length(addCoord - thirdCoord)) {
			outCoord = addCoord;
			inCoord = minCoord;
		}
		else {
			inCoord = addCoord;
			outCoord = minCoord;
		}

		if (R_out.count(i + 1) == 0) {
			R_out.insert({ i + 1, std::vector<glm::vec2>{ outCoord } });
		}
		else {
			R_out.at(i + 1).push_back(outCoord);
		}

		if (R_in.count(i + 1) == 0) {
			R_in.insert({ i + 1, std::vector<glm::vec2>{ inCoord} });
		}
		else {
			R_in.at(i + 1).push_back(inCoord);
		}
	}

	for (auto elem : R_out) {
		cv::Point coord(0.0f, 0.0f);
		if (elem.second.size() > 1) {
			glm::vec2 avgPoint{ 0.0f, 0.0f };
			for (glm::vec2 point : elem.second) {
				avgPoint += point;
			}
			avgPoint /= elem.second.size();
			coord.x = avgPoint.x * width;
			coord.y = avgPoint.y * height;
		}
		else {
			coord.x = elem.second[0].x * width;
			coord.y = elem.second[0].y * height;
		}
		cv::circle(demoMatDupe, cv::Point(vertices[strip.rightIndices[elem.first]].texCoord.x * width, vertices[strip.rightIndices[elem.first]].texCoord.y * height), 5, cv::Scalar(0, 255, 0), -1);
		cv::circle(demoMatDupe, coord, 5, cv::Scalar(0, 255, 0), -1);
	}

	for (auto elem : R_in) {
		cv::Point coord(0.0f, 0.0f);
		if (elem.second.size() > 1) {
			glm::vec2 avgPoint{ 0.0f, 0.0f };
			for (glm::vec2 point : elem.second) {
				avgPoint += point;
			}
			avgPoint /= elem.second.size();
			coord.x = avgPoint.x * width;
			coord.y = avgPoint.y * height;
		}
		else {
			coord.x = elem.second[0].x * width;
			coord.y = elem.second[0].y * height;
		}
		cv::circle(demoMatDupe, cv::Point(vertices[strip.rightIndices[elem.first]].texCoord.x * width, vertices[strip.rightIndices[elem.first]].texCoord.y * height), 5, cv::Scalar(255, 0, 0), -1);
		cv::circle(demoMatDupe, coord, 5, cv::Scalar(255, 0, 0), -1);
	}

	cv::imshow("Tex coord points", demoMatDupe);
	cv::waitKey(0);
}

void Mesh::findAdjacentStrips(cv::Mat demoMat) {
	// This function aims to find the UV seams in a given model and then return two sets of vertex indices - the vertices corresponding to the pair of triangle strips on either side of each seam
	// UV seams are denoted by any unique vertices which share the same position but have different UV coordinates
	std::vector<std::array<uint32_t, 2>> seamVertexPairs{};
	std::unordered_map<glm::vec3, uint32_t> uniquePositions{};
	for (uint32_t i = 0; i != vertices.size(); i++) {
		if (uniquePositions.count(vertices[i].pos) == 0) {
			uniquePositions.insert({ vertices[i].pos, i });
		}
		else {
			if (vertices[uniquePositions.at(vertices[i].pos)].texCoord != vertices[i].texCoord) {
				seamVertexPairs.push_back(std::array<uint32_t, 2>{uniquePositions.at(vertices[i].pos), i});
			}
		}
	}

	// We should now have a vector containing the indices of all pairs of vertices that mark the seams of the UV map
	// The aim is now to separate individual seams and determine which side of the seam each vertex falls on i.e. we construct the chain of vertices on either side of each
	// In the indices of the mesh vertices which correspond to triangles are arranged in triples; therefore if multiple seam vertices appear in the same set of three indices we know that these vertices are on the same side of a seam

	std::cout << "Found " << seamVertexPairs.size() << " vertex pairs" << std::endl;

	std::vector<SeamStrip> seamStrips{};
	
	uint32_t width = demoMat.size().width;
	uint32_t height = demoMat.size().height;

	cv::Mat cleanDemoMat = demoMat.clone();

	while (seamVertexPairs.size() > 1) {
		SeamStrip newSeamStrip;
		std::vector<uint32_t> chainIndices{};
		
		getStripChain(newSeamStrip, 0, seamVertexPairs, chainIndices);

		createSeamMeshes(cleanDemoMat, newSeamStrip);

		for (uint32_t j = 0; j != newSeamStrip.leftIndices.size(); j++) {
			uint32_t i = newSeamStrip.leftIndices[j];
			cv::circle(demoMat, cv::Point(vertices[i].texCoord.x * width, vertices[i].texCoord.y * height), 5, cv::Scalar(255 * static_cast<float>(j) / static_cast<float>(newSeamStrip.leftIndices.size()), 0, 0), -1);
		}

		for (uint32_t j = 0; j != newSeamStrip.rightIndices.size(); j++) {
			uint32_t i = newSeamStrip.rightIndices[j];
			cv::circle(demoMat, cv::Point(vertices[i].texCoord.x * width, vertices[i].texCoord.y * height), 5, cv::Scalar(0, 255 * static_cast<float>(j) / static_cast<float>(newSeamStrip.rightIndices.size()), 0), -1);
		}

		sort(chainIndices.begin(), chainIndices.end());
		uint32_t sub = 0;
		for (uint32_t i : chainIndices) {
			seamVertexPairs[i] = std::array<uint32_t, 2>{ 0,0 };
		}

		seamVertexPairs.erase(remove(seamVertexPairs.begin(), seamVertexPairs.end(), std::array<uint32_t, 2>{0, 0}), seamVertexPairs.end());

		seamStrips.push_back(newSeamStrip);
	}

	cv::imshow("Seam checker", demoMat);
	cv::waitKey(0);
}

const void Mesh::cleanup() {
	if (cleaned) {
		return;
	}

	Engine* engine = Engine::get();

	vkDestroyBuffer(engine->device, indexBuffer, nullptr);
	vkFreeMemory(engine->device, indexBufferMemory, nullptr);

	vkDestroyBuffer(engine->device, vertexBuffer, nullptr);
	vkFreeMemory(engine->device, vertexBufferMemory, nullptr);

	cleaned = true;
}

void UIMesh::UpdateVertices(float xp, float yp, float xsc, float ysc, float zp) {
	vertices.clear();

	Vertex vertex{};

	vertex.pos = { -xsc + xp, yp - ysc, zp };
	vertex.normal = { 0.0f, 0.0f, 0.0f };
	vertex.texCoord = { 0.0f, 0.0f };
	vertices.push_back(vertex);
	vertex.pos = { xsc + xp, yp - ysc, zp };
	vertex.texCoord = { 1.0f, 0.0f };
	vertices.push_back(vertex);
	vertex.pos = { xsc + xp, yp + ysc, zp };
	vertex.texCoord = { 1.0f, 1.0f };
	vertices.push_back(vertex);
	vertex.pos = { -xsc + xp, yp + ysc, zp };
	vertex.texCoord = { 0.0f, 1.0f };
	vertices.push_back(vertex);

	if (vBuffer == nullptr) {
		setup();
	}
	else {
		updateVertexBuffer();
	}
}

void UIMesh::createVertexBuffer() {

	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	Engine::get()->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(Engine::get()->device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(Engine::get()->device, stagingBufferMemory);

	Engine::get()->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, vertexBuffer, vertexBufferMemory);

	Engine::get()->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	vkMapMemory(Engine::get()->device, vertexBufferMemory, 0, bufferSize, 0, &vBuffer);

	vkDestroyBuffer(Engine::get()->device, stagingBuffer, nullptr);
	vkFreeMemory(Engine::get()->device, stagingBufferMemory, nullptr);
}

void UIMesh::updateVertexBuffer() {
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	memcpy(vBuffer, vertices.data(), (size_t)bufferSize);
}

StaticMesh::StaticMesh(string modelPath) {
	loadModel(modelPath);
	setup();
}

bool StaticMesh::loadModel(string testMODEL_PATH) {
	tinyobj::attrib_t attrib;
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string warn, err;

	if (tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, testMODEL_PATH.c_str())) {
		unordered_map<Vertex, uint32_t> uniqueVertices{};
		unordered_map<glm::vec2, uint32_t> uniqueCoords{};

		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex{};

				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};

				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				// We also want to find any vertices which share a texture coord with a vertex that has a different position

				if (uniqueCoords.count(vertex.texCoord) == 0) {
					uniqueCoords[vertex.texCoord] = static_cast<uint32_t>(vertices.size());
				}

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);

				uniqueTexindices.push_back(uniqueVertices[vertex]);

				// System attempts to seek the unique texture coordinate vertices - does not currently work

				//if (vertices[uniqueCoords[vertex.texCoord]].pos == vertices[uniqueVertices[vertex]].pos) {
					// Checks if the position of this vertex is the same as the position of the first vertex with the same texture coordinate
				//	uniqueTexindices.push_back(uniqueVertices[vertex]);
				//}

			}
		}
		computeTangents();
		return true;
	}
	return false;
}

void StaticMesh::computeTangents() {
	// First we initialise all the tangents and bitangents
	for (Vertex vert : vertices) {
		vert.tangent = glm::vec4(0, 0, 0, 0);
		//vert.biTangent = glm::vec3(0, 0, 0);
	}

	vector<glm::vec3> biTangents;
	for (size_t i = 0; i != vertices.size(); i++) {
		biTangents.push_back(glm::vec3(0, 0, 0));
	}

	// Then we calculate the tangents and bitangents described by the plane of each triangle
	for (size_t i = 0; i != indices.size(); i+=3) {

		size_t i0 = indices[i + 0];
		size_t i1 = indices[i + 1];
		size_t i2 = indices[i + 2];
		
		glm::vec3& v0 = vertices[i0].pos;
		glm::vec3& v1 = vertices[i1].pos;
		glm::vec3& v2 = vertices[i2].pos;

		glm::vec2& uv0 = vertices[i0].texCoord;
		glm::vec2& uv1 = vertices[i1].texCoord;
		glm::vec2& uv2 = vertices[i2].texCoord;

		glm::vec3 deltaPos1 = v1 - v0;
		glm::vec3 deltaPos2 = v2 - v0;

		glm::vec2 deltaUV1 = uv1 - uv0;
		glm::vec2 deltaUV2 = uv2 - uv0;

		float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
		glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
		glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
		glm::vec4 fourTan = glm::vec4(tangent, 1.0);

		vertices[i0].tangent += fourTan;
		vertices[i1].tangent += fourTan;
		vertices[i2].tangent += fourTan;

		biTangents[i0] += bitangent;
		biTangents[i1] += bitangent;
		biTangents[i2] += bitangent;
	}

	// Finally we compute the normalized tangent vectors as well as the facing direction
	// w describes whether the normals need to be inverted or not (for vertices that have been mirrored and share UV coordinates)

	for (size_t i = 0; i != vertices.size(); i++) {
		glm::vec3 n = vertices[i].normal;
		glm::vec3 t0 = vertices[i].tangent;
		glm::vec3 t1 = biTangents[i];

		glm::vec3 t = t0 - (n * dot(n, t0));
		t = normalize(t); // Disabling this normalization will scale the vectors based on the scale of the vertices (perhaps this is more ideal?)

		glm::vec3 c = cross(n, t0);
		float w = (dot(c, t1) < 0) ? -1.0f : 1.0f;
		vertices[i].tangent = glm::vec4(t.x, t.y, t.z, w);

		t = cross(glm::vec3(vertices[i].tangent.x, vertices[i].tangent.y, vertices[i].tangent.z), vertices[i].normal);
		//vertices[i].biTangent = t;
	}
}

void PlaneMesh::constructMesh() {
	Vertex vertex{};

	float xsc = size;
	float ysc = size * aspectRatio;
	float xp = 0.0f;
	float yp = 0.0f;

	vertex.pos = { -xsc + xp, yp - ysc, 0.0f };
	vertex.normal = { 0.0f, 0.0f, 1.0f };
	vertex.texCoord = { 0.0f, 1.0f };
	vertices.push_back(vertex);
	vertex.pos = { xsc + xp, yp - ysc, 0.0f };
	vertex.texCoord = { 1.0f, 1.0f };
	vertices.push_back(vertex);
	vertex.pos = { xsc + xp, yp + ysc, 0.0f };
	vertex.texCoord = { 1.0f, 0.0f };
	vertices.push_back(vertex);
	vertex.pos = { -xsc + xp, yp + ysc, 0.0f };
	vertex.texCoord = { 0.0f, 0.0f };
	vertices.push_back(vertex);


	indices = { 0, 1, 2, 2, 3, 0 };
}

void PlaneMesh::computeTangents() {
	// First we initialise all the tangents and bitangents
	for (Vertex vert : vertices) {
		vert.tangent = glm::vec4(0, 0, 0, 0);
		//vert.biTangent = glm::vec3(0, 0, 0);
	}

	vector<glm::vec3> biTangents;
	for (size_t i = 0; i != vertices.size(); i++) {
		biTangents.push_back(glm::vec3(0, 0, 0));
	}

	// Then we calculate the tangents and bitangents described by the plane of each triangle
	for (size_t i = 0; i != indices.size(); i += 3) {

		size_t i0 = indices[i + 0];
		size_t i1 = indices[i + 1];
		size_t i2 = indices[i + 2];

		glm::vec3& v0 = vertices[i0].pos;
		glm::vec3& v1 = vertices[i1].pos;
		glm::vec3& v2 = vertices[i2].pos;

		glm::vec2& uv0 = vertices[i0].texCoord;
		glm::vec2& uv1 = vertices[i1].texCoord;
		glm::vec2& uv2 = vertices[i2].texCoord;

		glm::vec3 deltaPos1 = v1 - v0;
		glm::vec3 deltaPos2 = v2 - v0;

		glm::vec2 deltaUV1 = uv1 - uv0;
		glm::vec2 deltaUV2 = uv2 - uv0;

		float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
		glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
		glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
		glm::vec4 fourTan = glm::vec4(tangent, 1.0);

		vertices[i0].tangent += fourTan;
		vertices[i1].tangent += fourTan;
		vertices[i2].tangent += fourTan;

		biTangents[i0] += bitangent;
		biTangents[i1] += bitangent;
		biTangents[i2] += bitangent;
	}

	// Finally we compute the normalized tangent vectors as well as the facing direction
	// w describes whether the normals need to be inverted or not (for vertices that have been mirrored and share UV coordinates)

	for (size_t i = 0; i != vertices.size(); i++) {
		glm::vec3 n = vertices[i].normal;
		glm::vec3 t0 = vertices[i].tangent;
		glm::vec3 t1 = biTangents[i];

		glm::vec3 t = t0 - (n * dot(n, t0));
		t = normalize(t); // Disabling this normalization will scale the vectors based on the scale of the vertices (perhaps this is more ideal?)

		glm::vec3 c = cross(n, t0);
		float w = (dot(c, t1) < 0) ? -1.0f : 1.0f;
		vertices[i].tangent = glm::vec4(t.x, t.y, t.z, w);

		t = cross(glm::vec3(vertices[i].tangent.x, vertices[i].tangent.y, vertices[i].tangent.z), vertices[i].normal);
		//vertices[i].biTangent = t;
	}
}