#include"SeamFixer.h"

#include"ImageProcessor.h"
#include"include/ShaderDataType.h"
#include"include/SeamFix_Colour.h"
#include"include/SeamFix_Alpha.h"
#include"include/AlphaOver.h"
#include"include/IPAlphaOver.h"
#include"WindowsFileManager.h"

void findLocalIndices(std::vector<uint32_t>& localIndices, std::vector<uint32_t> indices, uint32_t index) {
	auto it = find(indices.begin(), indices.end(), index);

	std::vector<uint32_t> indexOccurrences{};
	while (it != indices.end()) {
		indexOccurrences.push_back(it - indices.begin());
		it = find(it + 1, indices.end(), index);
	}

	for (uint32_t i = 0; i != indexOccurrences.size(); i++) {
		uint32_t L_Index = indexOccurrences[i];
		switch (L_Index % 3) {
		case (0):
			localIndices.push_back(indices[L_Index + 1]);
			localIndices.push_back(indices[L_Index + 2]);
			break;
		case (1):
			localIndices.push_back(indices[L_Index - 1]);
			localIndices.push_back(indices[L_Index + 1]);
			break;
		case (2):
			localIndices.push_back(indices[L_Index - 2]);
			localIndices.push_back(indices[L_Index - 1]);
			break;
		default:
			break;
		}
	}
}

void SeamFixer::getStripChain(SeamStrip& strip, uint32_t nodeIndex, std::vector<std::array<uint32_t, 2>>& vertexPairs, std::vector<uint32_t>& deleteIndices) {
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

	std::vector<uint32_t> indices = target->indices;

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

void SeamFixer::sortSeamIndices(SeamStrip& strip) {
	// Sorts both sets of indices based on the layout of the left indices
	// We need to modify this so that it's based on the layout of both the left and right indices
	std::map<uint32_t, std::vector<uint32_t>> localIndicesMap{};
	std::map<uint32_t, std::vector<uint32_t>> rightLocalIndicesMap{};
	for (uint32_t i = 0; i != strip.leftIndices.size(); i++) {
		std::vector<uint32_t> localIndices{};
		localIndicesMap.insert({ i, std::vector<uint32_t>{} });
		rightLocalIndicesMap.insert({ i, std::vector<uint32_t>{} });
		findLocalIndices(localIndices, target->indices, strip.leftIndices[i]);
		for (uint32_t index : localIndices) {
			auto it = find(strip.leftIndices.begin(), strip.leftIndices.end(), index);
			if (it != strip.leftIndices.end()) {
				localIndicesMap.at(i).push_back(it - strip.leftIndices.begin());
			}
		}
		if (localIndicesMap.at(i).size() == 0) {
			return;
		}
		findLocalIndices(localIndices, target->indices, strip.rightIndices[i]);
		for (uint32_t index : localIndices) {
			auto it = find(strip.rightIndices.begin(), strip.rightIndices.end(), index);
			if (it != strip.rightIndices.end()) {
				rightLocalIndicesMap.at(i).push_back(it - strip.rightIndices.begin());
			}
		}
		if (rightLocalIndicesMap.at(i).size() == 0) {
			return;
		}
	}
	bool closed = true;
	uint32_t currentIndex = 0;
	for (auto elem : localIndicesMap) {
		if (elem.second.size() == 1 || rightLocalIndicesMap.at(elem.first).size() == 1) {
			closed = false;
			strip.leftClosed = !(elem.second.size() == 1);
			strip.rightClosed = !(rightLocalIndicesMap.at(elem.first).size() == 1);
			currentIndex = elem.first;
			break;
		}
	}
	uint32_t rightCurrentIndex = currentIndex;
	std::vector<uint32_t> sortedSeamPositions{};
	sortedSeamPositions.push_back(currentIndex);
	std::vector<uint32_t> local = localIndicesMap.at(currentIndex);
	if (local.size() == 1) {
		currentIndex = localIndicesMap.at(currentIndex)[0];
	}
	else {
		currentIndex = ((local[0] - currentIndex) * (local[0] - currentIndex) < (local[1] - currentIndex) * (local[1] - currentIndex)) ? local[0] : local[1];
	}
	sortedSeamPositions.push_back(currentIndex);
	while (true) {
		std::vector<uint32_t> localIndices = localIndicesMap.at(currentIndex);
		if (localIndices.size() <= 1) {
			break;
		}
		auto it_0 = find(sortedSeamPositions.begin(), sortedSeamPositions.end(), localIndices[0]);
		auto it_1 = find(sortedSeamPositions.begin(), sortedSeamPositions.end(), localIndices[1]);
		if (it_0 == it_1) {
			currentIndex = ((localIndices[0] - currentIndex) * (localIndices[0] - currentIndex) < (localIndices[1] - currentIndex) * (localIndices[1] - currentIndex)) ? localIndices[0] : localIndices[1];
			sortedSeamPositions.push_back(currentIndex);
			continue;
		}
		if (it_0 == sortedSeamPositions.end()) {
			currentIndex = localIndices[0];
			sortedSeamPositions.push_back(localIndices[0]);
			continue;
		}
		if (it_1 == sortedSeamPositions.end()) {
			currentIndex = localIndices[1];
			sortedSeamPositions.push_back(localIndices[1]);
			continue;
		}
		break;
	}

	std::vector<uint32_t> rightSortedSeamPositions{};
	rightSortedSeamPositions.push_back(rightCurrentIndex);
	local = rightLocalIndicesMap.at(rightCurrentIndex);
	if (local.size() == 1) {
		rightCurrentIndex = rightLocalIndicesMap.at(rightCurrentIndex)[0];
	}
	else {
		rightCurrentIndex = ((local[0] - rightCurrentIndex) * (local[0] - rightCurrentIndex) < (local[1] - rightCurrentIndex) * (local[1] - rightCurrentIndex)) ? local[0] : local[1];
	}
	rightSortedSeamPositions.push_back(rightCurrentIndex);
	while (true) {
		std::vector<uint32_t> localIndices = rightLocalIndicesMap.at(rightCurrentIndex);
		if (localIndices.size() <= 1) {
			break;
		}
		auto it_0 = find(rightSortedSeamPositions.begin(), rightSortedSeamPositions.end(), localIndices[0]);
		auto it_1 = find(rightSortedSeamPositions.begin(), rightSortedSeamPositions.end(), localIndices[1]);
		if (it_0 == it_1) {
			rightCurrentIndex = ((localIndices[0] - rightCurrentIndex) * (localIndices[0] - rightCurrentIndex) < (localIndices[1] - rightCurrentIndex) * (localIndices[1] - rightCurrentIndex)) ? localIndices[0] : localIndices[1];
			rightSortedSeamPositions.push_back(rightCurrentIndex);
			continue;
		}
		if (it_0 == rightSortedSeamPositions.end()) {
			rightCurrentIndex = localIndices[0];
			rightSortedSeamPositions.push_back(localIndices[0]);
			continue;
		}
		if (it_1 == rightSortedSeamPositions.end()) {
			rightCurrentIndex = localIndices[1];
			rightSortedSeamPositions.push_back(localIndices[1]);
			continue;
		}
		break;
	}

	//std::cout << strip.leftIndices.size() << std::endl;

	//std::cout << "Sorted left positions: ";
	//for (uint32_t i : sortedSeamPositions) {
	//	std::cout << i << " ";
	//}
	//std::cout << std::endl;

	//std::cout << "Sorted right positions: ";
	//for (uint32_t i : rightSortedSeamPositions) {
	//	std::cout << i << " ";
	//}
	//std::cout << std::endl;

	std::vector<uint32_t> newLeftIndices{};
	std::vector<uint32_t> newRightIndices{};
	if ((strip.leftClosed && strip.rightClosed) || (!strip.leftClosed && !strip.rightClosed)) {
		for (uint32_t i : sortedSeamPositions) {
			newLeftIndices.push_back(strip.leftIndices[i]);
			newRightIndices.push_back(strip.rightIndices[i]);
		}
	}
	else {
		for (uint32_t i : sortedSeamPositions) {
			newLeftIndices.push_back(strip.leftIndices[i]);
		}
		for (uint32_t i : rightSortedSeamPositions) {
			newRightIndices.push_back(strip.rightIndices[i]);
		}
	}
	if (strip.leftClosed && newLeftIndices.size() < newRightIndices.size()) {
		//std::cout << "Left seam appears to be closed" << std::endl;
		newLeftIndices.push_back(strip.leftIndices[sortedSeamPositions[0]]);
	}
	if (strip.rightClosed && newRightIndices.size() < newLeftIndices.size()) {
		//std::cout << "Right seam appears to be closed" << std::endl;
		newRightIndices.push_back(strip.rightIndices[sortedSeamPositions[0]]);
	}
	strip.leftIndices = newLeftIndices;
	strip.rightIndices = newRightIndices;

	//std::cout << strip.leftIndices.size() << " " << strip.rightIndices.size() << std::endl;
}

void SeamFixer::createSeamMeshes(SeamStrip& strip, float distance = 0.01f) {
	strip.leftMesh.vertices.clear();
	strip.leftMesh.indices.clear();
	strip.leftAlphaMesh.vertices.clear();
	strip.leftAlphaMesh.indices.clear();
	strip.rightMesh.vertices.clear();
	strip.rightMesh.indices.clear();
	strip.rightAlphaMesh.vertices.clear();
	strip.rightAlphaMesh.indices.clear();

	//cv::Mat demoMatDupe = targetTex->texMat.clone();
	uint32_t width = targetTex->texWidth;
	uint32_t height = targetTex->texHeight;

	// First we do the left hand mesh
	// I'll start just by plotting points but later we can make this into an actual mesh

	std::map<uint32_t, std::vector<glm::vec2>> L_out{};
	std::map<uint32_t, std::vector<glm::vec2>> L_in{};

	std::vector<Vertex> vertices = target->vertices;
	std::vector<uint32_t> indices = target->indices;

	for (uint32_t i = 0; i != strip.leftIndices.size() - 1; i++) {
		uint32_t index_1 = strip.leftIndices[i];
		uint32_t index_2 = strip.leftIndices[i + 1];

		glm::vec2 texCoord_0 = vertices[index_1].texCoord;
		glm::vec2 texCoord_1 = vertices[index_2].texCoord;

		std::vector<uint32_t> Occurences_0{};
		std::vector<uint32_t> Occurences_1{};

		auto it_0 = find(indices.begin(), indices.end(), index_1);
		auto it_1 = find(indices.begin(), indices.end(), index_2);
		while (it_0 != indices.end()) {
			Occurences_0.push_back(it_0 - indices.begin());
			it_0 = find(it_0 + 1, indices.end(), index_1);
		}
		while (it_1 != indices.end()) {
			Occurences_1.push_back(it_1 - indices.begin());
			it_1 = find(it_1 + 1, indices.end(), index_2);
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

		//cv::circle(demoMatDupe, cv::Point(thirdCoord.x * width, thirdCoord.y * height), 5, cv::Scalar(0, 0, 255), -1);

		float gradient = static_cast<float>(texCoord_1.y - texCoord_0.y) / static_cast<float>(texCoord_1.x - texCoord_0.x);
		float invGradient = -1.0f / gradient;

		glm::vec2 pointingVec{ 0.0f, 0.0f };
		if (gradient != 0.0f) {
			pointingVec = glm::vec2(1.0f, invGradient);
			pointingVec /= glm::length(pointingVec);
		}
		else {
			pointingVec = glm::vec2(0.0f, 1.0f);
		}

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

		if (L_out.count(index_1) == 0) {
			L_out.insert({ index_1, std::vector<glm::vec2>{outCoord} });
		}
		else {
			L_out.at(index_1).push_back(outCoord);
		}

		if (L_in.count(index_1) == 0) {
			L_in.insert({ index_1, std::vector<glm::vec2>{ inCoord} });
		}
		else {
			L_in.at(index_1).push_back(inCoord);
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

		if (L_out.count(index_2) == 0) {
			L_out.insert({ index_2, std::vector<glm::vec2>{ outCoord } });
		}
		else {
			L_out.at(index_2).push_back(outCoord);
		}

		if (L_in.count(index_2) == 0) {
			L_in.insert({ index_2, std::vector<glm::vec2>{ inCoord } });
		}
		else {
			L_in.at(index_2).push_back(inCoord);
		}
	}

	std::map<uint32_t, std::vector<glm::vec2>> R_out{};
	std::map<uint32_t, std::vector<glm::vec2>> R_in{};

	for (uint32_t i = 0; i != strip.rightIndices.size() - 1; i++) {
		uint32_t index_1 = strip.rightIndices[i];
		uint32_t index_2 = strip.rightIndices[i + 1];

		glm::vec2 texCoord_0 = vertices[index_1].texCoord;
		glm::vec2 texCoord_1 = vertices[index_2].texCoord;

		std::vector<uint32_t> Occurences_0{};
		std::vector<uint32_t> Occurences_1{};

		auto it_0 = find(indices.begin(), indices.end(), index_1);
		auto it_1 = find(indices.begin(), indices.end(), index_2);
		while (it_0 != indices.end()) {
			Occurences_0.push_back(it_0 - indices.begin());
			it_0 = find(it_0 + 1, indices.end(), index_1);
		}
		while (it_1 != indices.end()) {
			Occurences_1.push_back(it_1 - indices.begin());
			it_1 = find(it_1 + 1, indices.end(), index_2);
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

		//cv::circle(demoMatDupe, cv::Point(thirdCoord.x * width, thirdCoord.y * height), 5, cv::Scalar(0, 0, 255), -1);

		float gradient = static_cast<float>(texCoord_1.y - texCoord_0.y) / static_cast<float>(texCoord_1.x - texCoord_0.x);
		float invGradient = -1.0f / gradient;

		glm::vec2 pointingVec{ 0.0f, 0.0f };
		if (gradient != 0.0f) {
			pointingVec = glm::vec2(1.0f, invGradient);
			pointingVec /= glm::length(pointingVec);
		}
		else {
			pointingVec = glm::vec2(0.0f, 1.0f);
		}

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


		if (R_out.count(index_1) == 0) {
			R_out.insert({ index_1, std::vector<glm::vec2>{ outCoord} });
		}
		else {
			R_out.at(index_1).push_back(outCoord);
		}

		if (R_in.count(index_1) == 0) {
			R_in.insert({ index_1, std::vector<glm::vec2>{ inCoord } });
		}
		else {
			R_in.at(index_1).push_back(inCoord);
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

		if (R_out.count(index_2) == 0) {
			R_out.insert({ index_2, std::vector<glm::vec2>{ outCoord } });
		}
		else {
			R_out.at(index_2).push_back(outCoord);
		}

		if (R_in.count(index_2) == 0) {
			R_in.insert({ index_2, std::vector<glm::vec2>{ inCoord} });
		}
		else {
			R_in.at(index_2).push_back(inCoord);
		}
	}

	// Now we need to construct meshes for the seams based on our results
	// Constructing the vertex positions is a simple matter of copying the positions; for each mesh we have two strips of vertices, one for the 'in' region and another for the 'out' region
	// Both strips will share the vertices along the seam but it is easier to just create duplicates of these vertices so that both strips are separate
	// The challenge is in setting the texture coordinates: they need to be the position of the vertices in the opposite mesh on the opposite region
	// For example, the texture coordinates of the 'out' strip of the left mesh need to be the same as the positions of the 'in' strip of the right mesh

	for (uint32_t i = 0; i != strip.leftIndices.size(); i++) {
		std::vector<glm::vec2> second = L_out.at(strip.leftIndices[i]);
		glm::vec2 coord{ 0.0f, 0.0f };
		if (second.size() > 1) {
			glm::vec2 avgPoint{ 0.0f, 0.0f };
			for (glm::vec2 point : second) {
				avgPoint += point;
			}
			avgPoint /= second.size();
			coord.x = avgPoint.x;// *width;
			coord.y = avgPoint.y;// *height;
		}
		else {
			coord.x = second[0].x;// *width;
			coord.y = second[0].y;// *height;
		}
		//cv::circle(demoMatDupe, cv::Point(vertices[strip.leftIndices[elem.first]].texCoord.x * width, vertices[strip.leftIndices[elem.first]].texCoord.y * height), 5, cv::Scalar(255, 0, 0), -1);
		//cv::circle(demoMatDupe, coord, 5, cv::Scalar(255, 0, 0), -1);

		Vertex L_out{};
		L_out.pos = glm::vec3(coord.x, coord.y, 0.0f);
		L_out.normal = glm::vec3(0.0f);
		L_out.texCoord = glm::vec2(0.0f, 0.0f);
		Vertex L_center{};
		L_center.pos = glm::vec3(vertices[strip.leftIndices[i]].texCoord.x, vertices[strip.leftIndices[i]].texCoord.y, 0.0f);
		L_center.normal = glm::vec3(0.0f);
		L_center.texCoord = glm::vec2(0.0f);

		strip.leftMesh.vertices.push_back(L_out);
		strip.leftMesh.vertices.push_back(L_center);

		strip.leftAlphaMesh.vertices.push_back(L_out);
		strip.leftAlphaMesh.vertices.push_back(L_center);


		Vertex R_in{};
		R_in.pos = glm::vec3(0.0f, 0.0f, 0.0f);
		R_in.normal = glm::vec3(0.0f);
		R_in.texCoord = glm::vec2(coord.x, coord.y);

		Vertex R_center{};
		R_center.pos = glm::vec3(0.0f);
		R_center.normal = glm::vec3(0.0f);
		R_center.texCoord = glm::vec2(vertices[strip.leftIndices[i]].texCoord.x, vertices[strip.leftIndices[i]].texCoord.y);

		strip.rightMesh.vertices.push_back(R_in);
		strip.rightMesh.vertices.push_back(R_center);
	}

	for (uint32_t i = 0; i != strip.leftIndices.size(); i++) {
		std::vector<glm::vec2> second = L_in.at(strip.leftIndices[i]);
		glm::vec2 coord{ 0.0f, 0.0f };
		if (second.size() > 1) {
			glm::vec2 avgPoint{ 0.0f, 0.0f };
			for (glm::vec2 point : second) {
				avgPoint += point;
			}
			avgPoint /= second.size();
			coord.x = avgPoint.x;// *width;
			coord.y = avgPoint.y;// *height;
		}
		else {
			coord.x = second[0].x;// *width;
			coord.y = second[0].y;// *height;
		}
		//cv::circle(demoMatDupe, cv::Point(vertices[strip.leftIndices[elem.first]].texCoord.x * width, vertices[strip.leftIndices[elem.first]].texCoord.y * height), 5, cv::Scalar(0, 255, 0), -1);
		//cv::circle(demoMatDupe, coord, 5, cv::Scalar(0, 255, 0), -1);

		Vertex L_in{};
		L_in.pos = glm::vec3(coord.x, coord.y, 0.0f);
		L_in.normal = glm::vec3(0.0f);
		L_in.texCoord = glm::vec2(0.0f, 1.0f);
		Vertex L_center{};
		L_center.pos = glm::vec3(vertices[strip.leftIndices[i]].texCoord.x, vertices[strip.leftIndices[i]].texCoord.y, 0.0f);
		L_center.normal = glm::vec3(0.0f);
		L_center.texCoord = glm::vec2(0.0f);

		strip.leftMesh.vertices.push_back(L_center);
		strip.leftMesh.vertices.push_back(L_in);

		strip.leftAlphaMesh.vertices.push_back(L_center);
		strip.leftAlphaMesh.vertices.push_back(L_in);


		Vertex R_center{};
		R_center.pos = glm::vec3(0.0f);
		R_center.normal = glm::vec3(0.0f);
		R_center.texCoord = glm::vec2(vertices[strip.leftIndices[i]].texCoord.x, vertices[strip.leftIndices[i]].texCoord.y);
		Vertex R_out{};
		R_out.pos = glm::vec3(0.0f, 0.0f, 0.0f);
		R_out.normal = glm::vec3(0.0f);
		R_out.texCoord = glm::vec2(coord.x, coord.y);

		strip.rightMesh.vertices.push_back(R_center);
		strip.rightMesh.vertices.push_back(R_out);
	}

	uint32_t index = 0;

	//for (auto elem : R_in) {
	for (uint32_t i = 0; i != strip.rightIndices.size(); i++) {
		std::vector<glm::vec2> second = R_in.at(strip.rightIndices[i]);
		glm::vec2 coord{ 0.0f, 0.0f };
		if (second.size() > 1) {
			glm::vec2 avgPoint{ 0.0f, 0.0f };
			for (glm::vec2 point : second) {
				avgPoint += point;
			}
			avgPoint /= second.size();
			coord.x = avgPoint.x;// *width;
			coord.y = avgPoint.y;// *height;
		}
		else {
			coord.x = second[0].x;// *width;
			coord.y = second[0].y;// *height;
		}
		//cv::circle(demoMatDupe, cv::Point(vertices[strip.rightIndices[elem.first]].texCoord.x * width, vertices[strip.rightIndices[elem.first]].texCoord.y * height), 5, cv::Scalar(255, 0, 0), -1);
		//cv::circle(demoMatDupe, coord, 5, cv::Scalar(255, 0, 0), -1);

		strip.rightMesh.vertices.at(index).pos = glm::vec3(coord.x, coord.y, 0.0f);
		strip.leftMesh.vertices.at(index).texCoord = glm::vec2(coord.x, coord.y);
		index++;
		strip.rightMesh.vertices.at(index).pos = glm::vec3(vertices[strip.rightIndices[i]].texCoord.x, vertices[strip.rightIndices[i]].texCoord.y, 0.0f);
		strip.leftMesh.vertices.at(index).texCoord = glm::vec2(vertices[strip.rightIndices[i]].texCoord.x, vertices[strip.rightIndices[i]].texCoord.y);
		index++;

		Vertex R_in_Alpha{};
		R_in_Alpha.pos = glm::vec3(coord.x, coord.y, 0.0f);
		R_in_Alpha.normal = glm::vec3(0.0f);
		R_in_Alpha.texCoord = glm::vec2(0.0f, 1.0f);
		Vertex R_center_Alpha{};
		R_center_Alpha.pos = glm::vec3(vertices[strip.rightIndices[i]].texCoord.x, vertices[strip.rightIndices[i]].texCoord.y, 0.0f);
		R_center_Alpha.normal = glm::vec3(0.0f);
		R_center_Alpha.texCoord = glm::vec2(0.0f);

		strip.rightAlphaMesh.vertices.push_back(R_in_Alpha);
		strip.rightAlphaMesh.vertices.push_back(R_center_Alpha);
	}

	//for (auto elem : R_out) {
	for (uint32_t i = 0; i != strip.rightIndices.size(); i++) {
		std::vector<glm::vec2> second = R_out.at(strip.rightIndices[i]);
		glm::vec2 coord{ 0.0f, 0.0f };
		if (second.size() > 1) {
			glm::vec2 avgPoint{ 0.0f, 0.0f };
			for (glm::vec2 point : second) {
				avgPoint += point;
			}
			avgPoint /= second.size();
			coord.x = avgPoint.x;// *width;
			coord.y = avgPoint.y;// *height;
		}
		else {
			coord.x = second[0].x;// *width;
			coord.y = second[0].y;// *height;
		}
		//cv::circle(demoMatDupe, cv::Point(vertices[strip.rightIndices[elem.first]].texCoord.x * width, vertices[strip.rightIndices[elem.first]].texCoord.y * height), 5, cv::Scalar(0, 255, 0), -1);
		//cv::circle(demoMatDupe, coord, 5, cv::Scalar(0, 255, 0), -1);

		strip.rightMesh.vertices.at(index).pos = glm::vec3(vertices[strip.rightIndices[i]].texCoord.x, vertices[strip.rightIndices[i]].texCoord.y, 0.0f);
		strip.leftMesh.vertices.at(index).texCoord = glm::vec2(vertices[strip.rightIndices[i]].texCoord.x, vertices[strip.rightIndices[i]].texCoord.y);
		index++;
		strip.rightMesh.vertices.at(index).pos = glm::vec3(coord.x, coord.y, 0.0f);
		strip.leftMesh.vertices.at(index).texCoord = glm::vec2(coord.x, coord.y);
		index++;

		Vertex R_center_Alpha{};
		R_center_Alpha.pos = glm::vec3(vertices[strip.rightIndices[i]].texCoord.x, vertices[strip.rightIndices[i]].texCoord.y, 0.0f);
		R_center_Alpha.normal = glm::vec3(0.0f);
		R_center_Alpha.texCoord = glm::vec2(0.0f);
		Vertex R_out_Alpha{};
		R_out_Alpha.pos = glm::vec3(coord.x, coord.y, 0.0f);
		R_out_Alpha.normal = glm::vec3(0.0f);
		R_out_Alpha.texCoord = glm::vec2(0.0f);

		strip.rightAlphaMesh.vertices.push_back(R_center_Alpha);
		strip.rightAlphaMesh.vertices.push_back(R_out_Alpha);
	}

	std::vector<uint32_t> stripIndices{}; // All meshes are assembled with essentially the same vertex structure, so the mesh indices will be identical
	uint32_t size = strip.leftMesh.vertices.size();
	uint32_t midpoint = size / 2;

	//Each pair of triangles should be structured like {0, 1, 2, 1, 2, 3}, but 2 and 3 are shared by the next pair of triangles
	uint32_t currentPosition = 0;
	std::vector<uint32_t> quadIndices{ 0, 1, 2, 1, 2, 3 };
	while (currentPosition + 3 < size) {
		for (uint32_t i : quadIndices) {
			stripIndices.push_back(i + currentPosition);
		}
		currentPosition += 2;
		if (currentPosition == midpoint - 2) {
			currentPosition += 2; // We don't want to draw triangles to connect the quad chains
		}
	}

	strip.leftMesh.indices = stripIndices;
	strip.leftAlphaMesh.indices = stripIndices;
	strip.rightMesh.indices = stripIndices;
	strip.rightAlphaMesh.indices = stripIndices;

	strip.leftMesh.setup();
	strip.rightMesh.setup();
	strip.leftAlphaMesh.setup();
	strip.rightAlphaMesh.setup();

	//cv::imshow("Tex coord points", demoMatDupe);
	//cv::waitKey(0);
}

void SeamFixer::updateSeamMeshes(float distance = 0.01f) {
	vkDeviceWaitIdle(Engine::get()->device);

	for (SeamStrip& strip : seamStrips) {
		strip.leftAlphaMesh.cleanup();
		strip.leftMesh.cleanup();
		strip.rightAlphaMesh.cleanup();
		strip.rightMesh.cleanup();

		createSeamMeshes(strip, distance);
	}
}

void SeamFixer::findAdjacentStrips() {
	// This function aims to find the UV seams in a given model and then return two sets of vertex indices - the vertices corresponding to the pair of triangle strips on either side of each seam
	// UV seams are denoted by any unique vertices which share the same position but have different UV coordinates

	//imageTex = new imageTexture(demoMat);

	std::vector<Vertex> vertices = target->vertices;

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

	//cv::Mat demoMat = targetTex->texMat.clone();

	width = targetTex->texWidth;
	height = targetTex->texHeight;

	//cv::Mat cleanDemoMat = demoMat.clone();

	while (seamVertexPairs.size() > 1) {
		SeamStrip newSeamStrip;
		std::vector<uint32_t> chainIndices{};

		getStripChain(newSeamStrip, 0, seamVertexPairs, chainIndices);

		sortSeamIndices(newSeamStrip);

		if (newSeamStrip.leftIndices.size() <= 1) {
			//std::cout << "Seam not found" << std::endl;
			seamVertexPairs[0] = std::array<uint32_t, 2>{ 0,0 };
			seamVertexPairs.erase(remove(seamVertexPairs.begin(), seamVertexPairs.end(), std::array<uint32_t, 2>{0, 0}), seamVertexPairs.end());
			continue;
		}

		//for (uint32_t j = 0; j != newSeamStrip.leftIndices.size(); j++) {
		//	uint32_t i = newSeamStrip.leftIndices[j];
		//	cv::circle(demoMat, cv::Point(vertices[i].texCoord.x * width, vertices[i].texCoord.y * height), 5, cv::Scalar(255 * static_cast<float>(j) / static_cast<float>(newSeamStrip.leftIndices.size()), 0, 0), -1);
		//}

		//for (uint32_t j = 0; j != newSeamStrip.rightIndices.size(); j++) {
		//	uint32_t i = newSeamStrip.rightIndices[j];
		//	cv::circle(demoMat, cv::Point(vertices[i].texCoord.x * width, vertices[i].texCoord.y * height), 5, cv::Scalar(0, 255 * static_cast<float>(j) / static_cast<float>(newSeamStrip.rightIndices.size()), 0), -1);
		//}

		createSeamMeshes(newSeamStrip);

		sort(chainIndices.begin(), chainIndices.end());
		uint32_t sub = 0;
		for (uint32_t i : chainIndices) {
			seamVertexPairs[i] = std::array<uint32_t, 2>{ 0,0 };
		}

		seamVertexPairs.erase(remove(seamVertexPairs.begin(), seamVertexPairs.end(), std::array<uint32_t, 2>{0, 0}), seamVertexPairs.end());

		seamStrips.push_back(newSeamStrip);
	}

	//cv::imshow("Seam checker", demoMat);
	//cv::waitKey(0);
}

void SeamFixer::prepMap(OverlayMap* map) {

	//OverlayMap* map = nullptr;
	//if (alpha) {
	//	if (isRight) {
	//		map = alphaMaps[1];
	//	}
	//	else {
	//		map = alphaMaps[0];
	//	}
	//}
	//else {
	//	if (isRight) {
	//		map = maps[1];
	//	}
	//	else {
	//		map = maps[0];
	//	}
	//}

	map->colour = new Texture;

	map->colour->texWidth = width;
	map->colour->texHeight = height;
	map->colour->texChannels = 4;
	map->colour->mipLevels = 1;
	map->colour->textureFormat = VK_FORMAT_R8G8B8A8_SRGB;
	map->colour->textureLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	map->colour->textureUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	map->colour->createImage(VK_SAMPLE_COUNT_1_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	map->colour->textureImageView = map->colour->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

	std::array<VkAttachmentDescription, 1> attachmentDescriptions = {};
	attachmentDescriptions[0].format = VK_FORMAT_R8G8B8A8_SRGB;
	attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
	renderPassInfo.pAttachments = attachmentDescriptions.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	if (vkCreateRenderPass(Engine::get()->device, &renderPassInfo, nullptr, &map->renderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass");
	}

	VkImageView attachments[1] = {};
	attachments[0] = map->colour->textureImageView;

	VkFramebufferCreateInfo fbufCreateInfo = {};
	fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbufCreateInfo.renderPass = map->renderPass;
	fbufCreateInfo.attachmentCount = 1;
	fbufCreateInfo.pAttachments = attachments;
	fbufCreateInfo.width = map->colour->texWidth;
	fbufCreateInfo.height = map->colour->texHeight;
	fbufCreateInfo.layers = 1;

	if (vkCreateFramebuffer(Engine::get()->device, &fbufCreateInfo, nullptr, &map->frameBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create framebuffer");
	}

	map->cleaned = false;
}

void SeamFixer::prepareColourDescriptor(OverlayMap* map) {

	//OverlayMap* map = (isRight) ? maps[1] : maps[0];

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 0;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &samplerLayoutBinding;

	if (vkCreateDescriptorSetLayout(Engine::get()->device, &layoutInfo, nullptr, &map->descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	VkDescriptorPoolSize poolSize;

	poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize.descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = 1;

	if (vkCreateDescriptorPool(Engine::get()->device, &poolInfo, nullptr, &map->descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = map->descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &map->descriptorSetLayout;

	if (vkAllocateDescriptorSets(Engine::get()->device, &allocInfo, &map->descriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = targetTex->textureImageView;
	imageInfo.sampler = Engine::get()->textureSampler;

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = map->descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(Engine::get()->device, 1, &descriptorWrite, 0, nullptr);

	map->cleaned = false;
}

void SeamFixer::createTexWritePipeline(OverlayMap* map) {
	shaderData* sD = new SEAMFIX_COLOURSHADER;

	//OverlayMap* map = (isRight) ? maps[1] : maps[0];

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_TRUE;
	multisampling.rasterizationSamples = msaaSamples;
	multisampling.minSampleShading = .2f;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &map->descriptorSetLayout;

	if (vkCreatePipelineLayout(Engine::get()->device, &pipelineLayoutInfo, nullptr, &map->pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	auto VertShaderCode = *(sD->vertData);
	auto FragShaderCode = *(sD->fragData);

	VkShaderModule VertShaderModule = Engine::get()->createShaderModule(VertShaderCode);
	VkShaderModule FragShaderModule = Engine::get()->createShaderModule(FragShaderCode);

	VkPipelineShaderStageCreateInfo VertShaderStageInfo{};
	VertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	VertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	VertShaderStageInfo.module = VertShaderModule;
	VertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo FragShaderStageInfo{};
	FragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	FragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	FragShaderStageInfo.module = FragShaderModule;
	FragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo, FragShaderStageInfo };

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = ShaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = map->pipelineLayout;
	pipelineInfo.renderPass = map->renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(Engine::get()->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &map->pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(Engine::get()->device, FragShaderModule, nullptr);
	vkDestroyShaderModule(Engine::get()->device, VertShaderModule, nullptr);

	delete sD;

	map->cleaned = false;
}

void SeamFixer::createAlphaWritePipeline(OverlayMap* map) {
	shaderData* sD = new SEAMFIX_ALPHASHADER;

	//OverlayMap* map = (isRight) ? alphaMaps[1] : alphaMaps[0];

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_TRUE;
	multisampling.rasterizationSamples = msaaSamples;
	multisampling.minSampleShading = .2f;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;

	if (vkCreatePipelineLayout(Engine::get()->device, &pipelineLayoutInfo, nullptr, &map->pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	auto VertShaderCode = *(sD->vertData);
	auto FragShaderCode = *(sD->fragData);

	VkShaderModule VertShaderModule = Engine::get()->createShaderModule(VertShaderCode);
	VkShaderModule FragShaderModule = Engine::get()->createShaderModule(FragShaderCode);

	VkPipelineShaderStageCreateInfo VertShaderStageInfo{};
	VertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	VertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	VertShaderStageInfo.module = VertShaderModule;
	VertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo FragShaderStageInfo{};
	FragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	FragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	FragShaderStageInfo.module = FragShaderModule;
	FragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo, FragShaderStageInfo };

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = ShaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = map->pipelineLayout;
	pipelineInfo.renderPass = map->renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(Engine::get()->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &map->pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(Engine::get()->device, FragShaderModule, nullptr);
	vkDestroyShaderModule(Engine::get()->device, VertShaderModule, nullptr);

	delete sD;

	map->cleaned = false;
}

VkCommandBuffer SeamFixer::drawColourMap(VkCommandBuffer commandbuffer, bool isRight) {

	OverlayMap* map = (isRight) ? maps[1] : maps[0];

	VkClearValue clearValues[1] = {};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = map->renderPass;
	renderPassBeginInfo.framebuffer = map->frameBuffer;
	renderPassBeginInfo.renderArea.extent.width = map->colour->texWidth;
	renderPassBeginInfo.renderArea.extent.height = map->colour->texHeight;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(commandbuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(map->colour->texWidth);
	viewport.height = static_cast<float>(map->colour->texHeight);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandbuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = { map->colour->texWidth, map->colour->texHeight };
	vkCmdSetScissor(commandbuffer, 0, 1, &scissor);

	vkCmdBindPipeline(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, map->pipeline);

	for (SeamStrip seam : seamStrips) {
		if (isRight) {
			VkBuffer vertexBuffers[] = { seam.rightMesh.vertexBuffer };
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindVertexBuffers(commandbuffer, 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(commandbuffer, seam.rightMesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, map->pipelineLayout, 0, 1, &map->descriptorSet, 0, nullptr);

			vkCmdDrawIndexed(commandbuffer, static_cast<uint32_t>(seam.rightMesh.indices.size()), 1, 0, 0, 0);

		}
		else {
			VkBuffer vertexBuffers[] = { seam.leftMesh.vertexBuffer };
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindVertexBuffers(commandbuffer, 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(commandbuffer, seam.leftMesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, map->pipelineLayout, 0, 1, &map->descriptorSet, 0, nullptr);

			vkCmdDrawIndexed(commandbuffer, static_cast<uint32_t>(seam.leftMesh.indices.size()), 1, 0, 0, 0);
		}
	}
	vkCmdEndRenderPass(commandbuffer);

	return commandbuffer;
}

VkCommandBuffer SeamFixer::drawColourMap(VkCommandBuffer commandbuffer, OverlayMap* map, Mesh* mesh) {
	VkClearValue clearValues[1] = {};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = map->renderPass;
	renderPassBeginInfo.framebuffer = map->frameBuffer;
	renderPassBeginInfo.renderArea.extent.width = map->colour->texWidth;
	renderPassBeginInfo.renderArea.extent.height = map->colour->texHeight;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(commandbuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(map->colour->texWidth);
	viewport.height = static_cast<float>(map->colour->texHeight);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandbuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = { map->colour->texWidth, map->colour->texHeight };
	vkCmdSetScissor(commandbuffer, 0, 1, &scissor);

	vkCmdBindPipeline(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, map->pipeline);

	VkBuffer vertexBuffers[] = { mesh->vertexBuffer };
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(commandbuffer, 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(commandbuffer, mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, map->pipelineLayout, 0, 1, &map->descriptorSet, 0, nullptr);

	vkCmdDrawIndexed(commandbuffer, static_cast<uint32_t>(mesh->indices.size()), 1, 0, 0, 0);

	vkCmdEndRenderPass(commandbuffer);

	return commandbuffer;
};

VkCommandBuffer SeamFixer::drawAlphaMap(VkCommandBuffer commandbuffer, bool isRight) {

	OverlayMap* map = (isRight) ? alphaMaps[1] : alphaMaps[0];

	VkClearValue clearValues[1] = {};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = map->renderPass;
	renderPassBeginInfo.framebuffer = map->frameBuffer;
	renderPassBeginInfo.renderArea.extent.width = map->colour->texWidth;
	renderPassBeginInfo.renderArea.extent.height = map->colour->texHeight;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(commandbuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(map->colour->texWidth);
	viewport.height = static_cast<float>(map->colour->texHeight);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandbuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = { map->colour->texWidth, map->colour->texHeight };
	vkCmdSetScissor(commandbuffer, 0, 1, &scissor);

	vkCmdBindPipeline(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, map->pipeline);

	for (SeamStrip seam : seamStrips) {
		if (isRight) {
			VkBuffer vertexBuffers[] = { seam.rightAlphaMesh.vertexBuffer };
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindVertexBuffers(commandbuffer, 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(commandbuffer, seam.rightAlphaMesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(commandbuffer, static_cast<uint32_t>(seam.rightAlphaMesh.indices.size()), 1, 0, 0, 0);

		}
		else {
			VkBuffer vertexBuffers[] = { seam.leftAlphaMesh.vertexBuffer };
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindVertexBuffers(commandbuffer, 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(commandbuffer, seam.leftAlphaMesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(commandbuffer, static_cast<uint32_t>(seam.leftAlphaMesh.indices.size()), 1, 0, 0, 0);
		}
	}
	vkCmdEndRenderPass(commandbuffer);

	return commandbuffer;
}

VkCommandBuffer SeamFixer::drawAlphaMap(VkCommandBuffer commandbuffer, OverlayMap* map, Mesh* mesh) {
	VkClearValue clearValues[1] = {};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = map->renderPass;
	renderPassBeginInfo.framebuffer = map->frameBuffer;
	renderPassBeginInfo.renderArea.extent.width = map->colour->texWidth;
	renderPassBeginInfo.renderArea.extent.height = map->colour->texHeight;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(commandbuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(map->colour->texWidth);
	viewport.height = static_cast<float>(map->colour->texHeight);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandbuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = { map->colour->texWidth, map->colour->texHeight };
	vkCmdSetScissor(commandbuffer, 0, 1, &scissor);

	vkCmdBindPipeline(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, map->pipeline);

	VkBuffer vertexBuffers[] = { mesh->vertexBuffer };
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(commandbuffer, 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(commandbuffer, mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(commandbuffer, static_cast<uint32_t>(mesh->indices.size()), 1, 0, 0, 0);

	vkCmdEndRenderPass(commandbuffer);

	return commandbuffer;
}

void SeamFixer::alphaOverMap(bool isRight) {
	Texture* seamMap = (isRight) ? maps[1]->colour : maps[0]->colour;
	Texture* seamAlpha = (isRight) ? alphaMaps[1]->colour : alphaMaps[0]->colour;
	seamMap = seamMap->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height);
	seamAlpha = seamAlpha->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height);
	Texture* baseTex = targetTex->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height);

	filter alphaOver(std::vector<Texture*>{baseTex, seamMap, seamAlpha}, new ALPHAOVERSHADER, VK_FORMAT_R8G8B8A8_UNORM);
	alphaOver.filterImage();

	Texture* res = alphaOver.filterTarget[0]->copyTexture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height);
	textureLL->replacePtr(res, targetTexName);

	seamMap->cleanup();
	seamAlpha->cleanup();
	baseTex->cleanup();
	alphaOver.cleanup();
}

void SeamFixer::alphaOverMap(OverlayMap* cMap, OverlayMap* aMap, Texture* texInFlight) {
	Texture* seamMap = cMap->colour->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height);
	Texture* seamAlpha = aMap->colour->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height);

	filter alphaOver(std::vector<Texture*>{texInFlight, seamMap, seamAlpha}, new IPALPHAOVERSHADER, VK_FORMAT_R8G8B8A8_UNORM);
	alphaOver.filterImage();

	vkDeviceWaitIdle(Engine::get()->device);

	//texInFlight->getCVMat();
	//cv::imshow("Result", texInFlight->texMat);
	//cv::waitKey(0);

	seamMap->cleanup();
	seamAlpha->cleanup();
	alphaOver.cleanup(false);
};

void SeamFixer::seamFixAll() {
	if (colourMap.cleaned) {
		prepMap(&colourMap);
		prepareColourDescriptor(&colourMap);
		createTexWritePipeline(&colourMap);
	}
	if (alphaMap.cleaned) {
		prepMap(&alphaMap);
		createAlphaWritePipeline(&alphaMap);
	}
	Texture* writeTex = targetTex->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height);
	for (SeamStrip & seam : seamStrips) {
		Mesh* colourMesh = nullptr;
		Mesh* alphaMesh = nullptr;
		if (seam.isRight) {
			colourMesh = &seam.rightMesh;
			alphaMesh = &seam.rightAlphaMesh;
		}
		else {
			colourMesh = &seam.leftMesh;
			alphaMesh = &seam.leftAlphaMesh;
		}
		VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
		commandBuffer = drawColourMap(commandBuffer, &colourMap, colourMesh);
		commandBuffer = drawAlphaMap(commandBuffer, &alphaMap, alphaMesh);
		Engine::get()->endSingleTimeCommands(commandBuffer);

		if (useRemapper) {
			commandBuffer = Engine::get()->beginSingleTimeComputeCommand();
			remapper->performRemap(commandBuffer);
			std::cout << "Here" << std::endl;
		}
		alphaOverMap(&colourMap, &alphaMap, writeTex);
	}
	Texture* res = writeTex->copyTexture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height);
	textureLL->replacePtr(res, targetTexName);

	writeTex->cleanup();
}