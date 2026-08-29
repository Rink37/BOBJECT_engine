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
		uint32_t index = indexOccurrences[i];
		switch (index % 3) {
		case (0):
			localIndices.push_back(indices[index + 1]);
			localIndices.push_back(indices[index + 2]);
			break;
		case (1):
			localIndices.push_back(indices[index - 1]);
			localIndices.push_back(indices[index + 1]);
			break;
		case (2):
			localIndices.push_back(indices[index - 2]);
			localIndices.push_back(indices[index - 1]);
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

	std::array<uint32_t, 2> seamVert = vertexPairs[nodeIndex];

	strip.leftIndices.push_back(seamVert[0]);
	strip.rightIndices.push_back(seamVert[1]);

	deleteIndices.push_back(nodeIndex);

	std::vector<uint32_t> indices = target->indices;

	std::vector<uint32_t> L_adjacentIndices{};
	std::vector<uint32_t> R_adjacentIndices{};

	findLocalIndices(L_adjacentIndices, indices, seamVert[0]);
	findLocalIndices(R_adjacentIndices, indices, seamVert[1]);

	std::vector<uint32_t> chainIndices{};

	for (uint32_t i = 0; i != vertexPairs.size(); i++) {
		//if (find(deleteIndices.begin(), deleteIndices.end(), i) != deleteIndices.end()) {
		//	continue;
		//}
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
			std::cout << i << " ";
		}
	}
}

void SeamFixer::sortSeamIndices(SeamStrip& strip) {
	// Sorts both sets of indices based on the layout of the left indices
	// We need to modify this so that it's based on the layout of both the left and right indices
	std::map<uint32_t, std::vector<uint32_t>> localIndicesMap{};
	std::map<uint32_t, std::vector<uint32_t>> rightLocalIndicesMap{};
	std::cout << strip.rightIndices.size() << " " << strip.leftIndices.size() << std::endl;
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
	//bool closed = true;
	uint32_t currentIndex = 0;
	for (auto elem : localIndicesMap) {
		if (elem.second.size() == 1 || rightLocalIndicesMap.at(elem.first).size() == 1) {
	//		closed = false;
	//		strip.leftClosed = !(elem.second.size() == 1);
	//		strip.rightClosed = !(rightLocalIndicesMap.at(elem.first).size() == 1);
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
	//if ((strip.leftClosed && strip.rightClosed) || (!strip.leftClosed && !strip.rightClosed)) {
	for (uint32_t i : sortedSeamPositions) {
		newLeftIndices.push_back(strip.leftIndices[i]);
		newRightIndices.push_back(strip.rightIndices[i]);
	}
	//}
	//else {
	//	for (uint32_t i : sortedSeamPositions) {
	//		newLeftIndices.push_back(strip.leftIndices[i]);
	//	}
	//	for (uint32_t i : rightSortedSeamPositions) {
	//		newRightIndices.push_back(strip.rightIndices[i]);
	//	}
	//}
	//if (strip.leftClosed){ // && newLeftIndices.size() < newRightIndices.size()) {
	//	//std::cout << "Left seam appears to be closed" << std::endl;
	//	newLeftIndices.push_back(strip.leftIndices[sortedSeamPositions[0]]);
	//}
	//if (strip.rightClosed){ //&& newRightIndices.size() < newLeftIndices.size()) {
	//	//std::cout << "Right seam appears to be closed" << std::endl;
	//	newRightIndices.push_back(strip.rightIndices[sortedSeamPositions[0]]);
	//}

	if (newLeftIndices.size() != newRightIndices.size()) {
		if (newLeftIndices.size() < newRightIndices.size()) {
			newLeftIndices.push_back(strip.leftIndices[sortedSeamPositions[0]]);
		}
		if (newRightIndices.size() < newLeftIndices.size()) {
			newRightIndices.push_back(strip.rightIndices[sortedSeamPositions[0]]);
		}
	}

	strip.leftIndices = newLeftIndices;
	strip.rightIndices = newRightIndices;

	std::cout << "No issue sorting seams" << std::endl;
	//std::cout << strip.leftIndices.size() << " " << strip.rightIndices.size() << std::endl;
}

void findSeamMeshLocalIndices(std::set<std::pair<uint32_t, uint32_t>>& localIndices, std::vector<uint32_t> indices, uint32_t index) {
	std::vector<uint32_t> Occurrences{};
	
	auto it = find(indices.begin(), indices.end(), index);
	while (it != indices.end()) {
		Occurrences.push_back(it - indices.begin());
		it = find(it + 1, indices.end(), index);
	}

	for (uint32_t j : Occurrences) {
		switch (j % 3) {
		case (0):
			localIndices.insert({ j, j + 1 });
			localIndices.insert({ j, j + 2 });
			break;
		case (1):
			localIndices.insert({ j, j - 1 });
			localIndices.insert({ j, j + 1 });
			break;
		case (2):
			localIndices.insert({ j, j - 2 });
			localIndices.insert({ j, j - 1 });
			break;
		default:
			break;
		}
	}
}

void getSeamIO(std::vector<Vertex> vertices, std::vector<uint32_t> indices, std::vector<uint32_t> stripIndices, float distance, std::map<uint32_t, std::vector<glm::vec2>>& in, std::map<uint32_t, std::vector<glm::vec2>>& out, bool closed, cv::Mat& demoMatDupe) {
	
	size_t size = stripIndices.size() - 1;
	if (!closed && stripIndices[0] == stripIndices[size]) {
		size -= 1;
	}

	uint32_t width = demoMatDupe.size().width;
	uint32_t height = demoMatDupe.size().height;

	//std::cout << "Size: " << size+1 << std::endl;
	
	for (uint32_t i = 0; i != size; i++) {
		uint32_t index_1 = stripIndices[i];
		uint32_t index_2 = stripIndices[i + 1];	

		glm::vec2 texCoord_0 = vertices[index_1].texCoord;
		glm::vec2 texCoord_1 = vertices[index_2].texCoord;

		std::set<std::pair<uint32_t, uint32_t>> connectedIndices_0{}; // all indices that touch index_1
		std::set<std::pair<uint32_t, uint32_t>> connectedIndices_1{}; // all indices that touch index_2

		findSeamMeshLocalIndices(connectedIndices_0, indices, index_1);
		findSeamMeshLocalIndices(connectedIndices_1, indices, index_2);

		std::vector<uint32_t> connectedIndices{}; // This should represent the four indices in the square

		// We now want to find the triangle containing both index_1 and index_2

		for (std::pair<uint32_t, uint32_t> pair_0 : connectedIndices_0) {
			for (std::pair<uint32_t, uint32_t> pair_1 : connectedIndices_1) {
				if (pair_0.second == pair_1.second) {
					connectedIndices.push_back(pair_0.first);
					connectedIndices.push_back(pair_1.first);
					connectedIndices.push_back(pair_0.second);
					break;
				}
			}
		}

		if (connectedIndices.size() == 0) {
			glm::vec2 coord{ 0, 0 };
			if (out.count(index_1) == 0) {
				out.insert({ index_1, std::vector<glm::vec2>{coord} });
			}
			else {
				out.at(index_1).push_back(coord);
			}

			if (in.count(index_1) == 0) {
				in.insert({ index_1, std::vector<glm::vec2>{coord} });
			}
			else {
				in.at(index_1).push_back(coord);
			}

			if (out.count(index_2) == 0) {
				out.insert({ index_2, std::vector<glm::vec2>{ coord } });
			}
			else {
				out.at(index_2).push_back(coord);
			}

			if (in.count(index_2) == 0) {
				in.insert({ index_2, std::vector<glm::vec2>{ coord } });
			}
			else {
				in.at(index_2).push_back(coord);
			}
			continue;
		}

		// Now we want to find the final index i.e. the index which is connected to both one of index_1 or index_2 and the triangle index we just found

		uint32_t index_3 = indices[connectedIndices[2]];

		std::set<std::pair<uint32_t, uint32_t>> connectedIndices_2{}; // all indices that touch index_2
		findSeamMeshLocalIndices(connectedIndices_2, indices, index_3);

		std::vector<std::pair<uint32_t, uint32_t>> candidates{};

		for (std::pair<uint32_t, uint32_t> pair_0 : connectedIndices_0) {
			for (std::pair<uint32_t, uint32_t> pair_2 : connectedIndices_2) {
				if (pair_0.second == pair_2.second && pair_0.second != index_2) {
					candidates.push_back({ index_1, pair_0.second });
				}
			}
		}

		for (std::pair<uint32_t, uint32_t> pair_1 : connectedIndices_1) {
			for (std::pair<uint32_t, uint32_t> pair_2 : connectedIndices_2) {
				if (pair_1.second == pair_2.second && pair_1.second != index_1) {
					candidates.push_back({ index_2, pair_1.second });
				}
			}
		}

		float minDistance = 10000.0f;
		uint32_t bestCandidate = 0;
		for (std::pair<uint32_t, uint32_t> pair : candidates) {
			float dist = 0.0f;
			if (pair.first == index_1) {
				dist = glm::length(texCoord_1 - vertices[indices[pair.second]].texCoord);
			}
			else {
				dist = glm::length(texCoord_0 - vertices[indices[pair.second]].texCoord);
			}
			if (dist < minDistance && dist != 0.0f) {
				minDistance = dist;
				bestCandidate = pair.second;
			}
		}

		if (bestCandidate != 0) {
			connectedIndices.push_back(bestCandidate);
		}

		glm::vec2 thirdCoord{ 0, 0 };

		for (uint32_t j : connectedIndices) {
			thirdCoord += vertices[indices[j]].texCoord;
			//cv::circle(demoMatDupe, cv::Point(vertices[indices[j]].texCoord.x * width, vertices[indices[j]].texCoord.y * height), 5, cv::Scalar(colour, 0, 255), -1);
		}

		thirdCoord /= connectedIndices.size();

		cv::circle(demoMatDupe, cv::Point(thirdCoord.x * width, thirdCoord.y * height), 3, cv::Scalar(0, 0, 255), -1);

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

		if (out.count(index_1) == 0) {
			out.insert({ index_1, std::vector<glm::vec2>{outCoord} });
		}
		else {
			out.at(index_1).push_back(outCoord);
		}

		if (in.count(index_1) == 0) {
			in.insert({ index_1, std::vector<glm::vec2>{ inCoord} });
		}
		else {
			in.at(index_1).push_back(inCoord);
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

		if (out.count(index_2) == 0) {
			out.insert({ index_2, std::vector<glm::vec2>{ outCoord } });
		}
		else {
			out.at(index_2).push_back(outCoord);
		}

		if (in.count(index_2) == 0) {
			in.insert({ index_2, std::vector<glm::vec2>{ inCoord } });
		}
		else {
			in.at(index_2).push_back(inCoord);
		}
	}
}

void SeamFixer::createSeamMeshes(SeamStrip& strip, float distance = 0.025f, bool updateColour = true, bool smoothOutside = false) {
	
	//std::cout << "Creating seam meshes!" << std::endl;

	stripSize = distance;

	if (updateColour) {
		strip.leftMesh.vertices.clear();
		strip.leftMesh.indices.clear();
		strip.rightMesh.vertices.clear();
		strip.rightMesh.indices.clear();
	}
	strip.leftAlphaMesh.vertices.clear();
	strip.leftAlphaMesh.indices.clear();
	strip.rightAlphaMesh.vertices.clear();
	strip.rightAlphaMesh.indices.clear();

	cv::Mat demoMatDupe = targetTex->texMat.clone();
	resize(demoMatDupe, demoMatDupe, cv::Size(), 0.2f, 0.2f);
	uint32_t width = demoMatDupe.size().width;
	uint32_t height = demoMatDupe.size().height;

	// First we do the left hand mesh

	std::vector<Vertex> vertices = target->vertices;
	std::vector<uint32_t> indices = target->indices;

	std::map<uint32_t, std::vector<glm::vec2>> L_out{};
	std::map<uint32_t, std::vector<glm::vec2>> L_in{};

	getSeamIO(vertices, indices, strip.leftIndices, distance, L_in, L_out, strip.leftClosed, demoMatDupe);

	//std::cout << "Got left seam IO" << std::endl;

	std::map<uint32_t, std::vector<glm::vec2>> R_out{};
	std::map<uint32_t, std::vector<glm::vec2>> R_in{};

	getSeamIO(vertices, indices, strip.rightIndices, distance, R_in, R_out, strip.rightClosed, demoMatDupe);

	//std::cout << "Got right seam IO" << std::endl;

	//std::cout << L_in.size() << " " << L_out.size() << " " << R_in.size() << " " << R_out.size() << std::endl;
	//if (strip.leftClosed) {
	//	std::cout << "Left closed" << std::endl;
	//}
	//if (strip.rightClosed) {
	//	std::cout << "Right closed" << std::endl;
	//}

	// Now we need to construct meshes for the seams based on our results
	// Constructing the vertex positions is a simple matter of copying the positions; for each mesh we have two strips of vertices, one for the 'in' region and another for the 'out' region
	// Both strips will share the vertices along the seam but it is easier to just create duplicates of these vertices so that both strips are separate
	// The challenge is in setting the texture coordinates: they need to be the position of the vertices in the opposite mesh on the opposite region
	// For example, the texture coordinates of the 'out' strip of the left mesh need to be the same as the positions of the 'in' strip of the right mesh

	float outsideTexCoord = (smoothOutside) ? 1.0f : 0.0f;

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
		cv::circle(demoMatDupe, cv::Point(vertices[strip.leftIndices[i]].texCoord.x * width, vertices[strip.leftIndices[i]].texCoord.y * height), 5, cv::Scalar(255, 0, 0), -1);
		cv::circle(demoMatDupe, cv::Point(coord.x * width, coord.y * height), 5, cv::Scalar(255, 0, 0), -1);

		Vertex L_out{};
		L_out.pos = glm::vec3(coord.x, coord.y, 0.0f);
		L_out.normal = glm::vec3(0.0f);
		L_out.texCoord = glm::vec2(0.0f, outsideTexCoord);
		Vertex L_center{};
		L_center.pos = glm::vec3(vertices[strip.leftIndices[i]].texCoord.x, vertices[strip.leftIndices[i]].texCoord.y, 0.0f);
		L_center.normal = glm::vec3(0.0f);
		L_center.texCoord = glm::vec2(0.0f);

		if (updateColour) {
			strip.leftMesh.vertices.push_back(L_out);
			strip.leftMesh.vertices.push_back(L_center);
		}

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

		if (updateColour) {
			strip.rightMesh.vertices.push_back(R_in);
			strip.rightMesh.vertices.push_back(R_center);
		}
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
		cv::circle(demoMatDupe, cv::Point(vertices[strip.leftIndices[i]].texCoord.x * width, vertices[strip.leftIndices[i]].texCoord.y * height), 5, cv::Scalar(0, 255, 0), -1);
		cv::circle(demoMatDupe, cv::Point(coord.x * width, coord.y * height), 5, cv::Scalar(0, 255, 0), -1);

		Vertex L_in{};
		L_in.pos = glm::vec3(coord.x, coord.y, 0.0f);
		L_in.normal = glm::vec3(0.0f);
		L_in.texCoord = glm::vec2(0.0f, 1.0f);
		Vertex L_center{};
		L_center.pos = glm::vec3(vertices[strip.leftIndices[i]].texCoord.x, vertices[strip.leftIndices[i]].texCoord.y, 0.0f);
		L_center.normal = glm::vec3(0.0f);
		L_center.texCoord = glm::vec2(0.0f);

		if (updateColour) {
			strip.leftMesh.vertices.push_back(L_center);
			strip.leftMesh.vertices.push_back(L_in);
		}

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

		if (updateColour) {
			strip.rightMesh.vertices.push_back(R_center);
			strip.rightMesh.vertices.push_back(R_out);
		}
	}

	uint32_t index = 0;

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
		cv::circle(demoMatDupe, cv::Point(vertices[strip.rightIndices[i]].texCoord.x * width, vertices[strip.rightIndices[i]].texCoord.y * height), 5, cv::Scalar(255, 0, 0), -1);
		cv::circle(demoMatDupe, cv::Point(coord.x * width, coord.y * height), 5, cv::Scalar(255, 0, 0), -1);

		if (updateColour) {
			strip.rightMesh.vertices.at(index).pos = glm::vec3(coord.x, coord.y, 0.0f);
			strip.leftMesh.vertices.at(index).texCoord = glm::vec2(coord.x, coord.y);
			index++;
			strip.rightMesh.vertices.at(index).pos = glm::vec3(vertices[strip.rightIndices[i]].texCoord.x, vertices[strip.rightIndices[i]].texCoord.y, 0.0f);
			strip.leftMesh.vertices.at(index).texCoord = glm::vec2(vertices[strip.rightIndices[i]].texCoord.x, vertices[strip.rightIndices[i]].texCoord.y);
			index++;
		}
		

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
		cv::circle(demoMatDupe, cv::Point(vertices[strip.rightIndices[i]].texCoord.x * width, vertices[strip.rightIndices[i]].texCoord.y * height), 5, cv::Scalar(0, 255, 0), -1);
		cv::circle(demoMatDupe, cv::Point(coord.x * width, coord.y * height), 5, cv::Scalar(0, 255, 0), -1);

		if (updateColour) {
			strip.rightMesh.vertices.at(index).pos = glm::vec3(vertices[strip.rightIndices[i]].texCoord.x, vertices[strip.rightIndices[i]].texCoord.y, 0.0f);
			strip.leftMesh.vertices.at(index).texCoord = glm::vec2(vertices[strip.rightIndices[i]].texCoord.x, vertices[strip.rightIndices[i]].texCoord.y);
			index++;
			strip.rightMesh.vertices.at(index).pos = glm::vec3(coord.x, coord.y, 0.0f);
			strip.leftMesh.vertices.at(index).texCoord = glm::vec2(coord.x, coord.y);
			index++;
		}

		Vertex R_center_Alpha{};
		R_center_Alpha.pos = glm::vec3(vertices[strip.rightIndices[i]].texCoord.x, vertices[strip.rightIndices[i]].texCoord.y, 0.0f);
		R_center_Alpha.normal = glm::vec3(0.0f);
		R_center_Alpha.texCoord = glm::vec2(0.0f);
		Vertex R_out_Alpha{};
		R_out_Alpha.pos = glm::vec3(coord.x, coord.y, 0.0f);
		R_out_Alpha.normal = glm::vec3(0.0f);
		R_out_Alpha.texCoord = glm::vec2(0.0f, outsideTexCoord);

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

	if (strip.directionLocked) {
		//std::cout << "Locking direction!" << std::endl;
		if (updateColour) {
			strip.rightMesh.indices = stripIndices;
			strip.rightMesh.setup();

			strip.leftMesh = strip.rightMesh;
		}
		strip.rightAlphaMesh.indices = stripIndices;
		strip.rightAlphaMesh.setup();

		strip.leftAlphaMesh = strip.rightAlphaMesh;
	}
	else {
		if (updateColour) {
			strip.leftMesh.indices = stripIndices;
			strip.rightMesh.indices = stripIndices;

			strip.leftMesh.setup();
			strip.rightMesh.setup();
		}
		strip.leftAlphaMesh.indices = stripIndices;
		strip.rightAlphaMesh.indices = stripIndices;

		strip.leftAlphaMesh.setup();
		strip.rightAlphaMesh.setup();
	}

	//cv::imshow("Tex coord points", demoMatDupe);
	//cv::waitKey(0);

	//std::cout << "Finished" << std::endl;
}

void SeamFixer::updateSeamMeshes(float distance, bool updateColour, bool smoothOutside) {
	vkDeviceWaitIdle(Engine::get()->device);

	stripSize = distance;

	for (SeamStrip& strip : seamStrips) {
		if (updateColour) {
			if (!strip.directionLocked) {
				strip.leftMesh.cleanup();
			}
			strip.rightMesh.cleanup();
		}
		if (!strip.directionLocked) {
			strip.leftAlphaMesh.cleanup();
		}
		strip.rightAlphaMesh.cleanup();

		createSeamMeshes(strip, distance, updateColour, smoothOutside);
	}

	//packSeamStrips();
}

void flipSeamIndices(SeamStrip& strip) {
	std::vector<uint32_t> savedRightIndices = strip.rightIndices;
	strip.rightIndices = strip.leftIndices;
	strip.leftIndices = savedRightIndices;
}

void compareStrips(SeamStrip& a, SeamStrip& b, std::vector<Vertex> vertices) {
	// Checks if two strips have sides with matching UV coordinates, and updates them accordingly
	// Pairs of strips which have a strip that shares UV coordinates should always place the matching strip on the left and then set the strips to locked

	if (a.leftIndices.size() != b.leftIndices.size() || a.leftIndices.size() == 0 || a.leftIndices.size() != a.rightIndices.size() || b.leftIndices.size() != b.rightIndices.size()) {
		return; // Assume that if two seams share UV coordinates on one side they will be the same non-zero length
	}

	uint8_t matchDirections = 0; // 0 means no match, 1 means LL match, 2 means RR match, 3 means LR match, 4 means RL match

	for (uint32_t i = 0; i != a.leftIndices.size(); i++) {
		uint8_t lastMatchDirection = matchDirections;
		glm::vec2 a_L_tC = vertices[a.leftIndices[i]].texCoord;
		glm::vec2 a_R_tC = vertices[a.rightIndices[i]].texCoord;
		for (uint32_t j = 0; j != b.leftIndices.size(); j++) {
			glm::vec2 b_L_tC = vertices[b.leftIndices[j]].texCoord;
			glm::vec2 b_R_tC = vertices[b.rightIndices[j]].texCoord;

			if (a_L_tC == b_L_tC) {
				if (a_R_tC == b_R_tC) {
					return; // The seams are duplicates - repeated geometry that doesn't connect to non-repeated geometry
				}
				matchDirections = 1;
				break;
			}
			else if (a_R_tC == b_R_tC) {
				matchDirections = 2;
				break;
			}
			else if (a_L_tC == b_R_tC) {
				if (a_R_tC == b_L_tC) {
					flipSeamIndices(b);
					return;
				}
				matchDirections = 3;
				break;
			}
			else if (a_R_tC == b_L_tC) {
				matchDirections = 4;
				break;
			}
		}
		if (matchDirections == 0) {
			return; // No match found
		}
		else if (matchDirections == lastMatchDirection){
			break; // Assume that if the first two vertices match then the rest will too
		}
		else if (i > 0) {
			return; // If we've checked more than one seam vertex and the directions don't match we assume there was some error and do nothing
		}
	}

	if (matchDirections == 1) {
		a.directionLocked = true;
		b.directionLocked = true;
	}
	else if (matchDirections == 2) {
		flipSeamIndices(a);
		flipSeamIndices(b);
		a.directionLocked = true;
		b.directionLocked = true;
	}
	else if (matchDirections == 3) {
		flipSeamIndices(b);
		a.directionLocked = true;
		b.directionLocked = true;
	}
	else if (matchDirections == 4) {
		flipSeamIndices(a);
		a.directionLocked = true;
		b.directionLocked = true;
	}
}

void SeamFixer::findAdjacentStrips() {
	// This function aims to find the UV seams in a given model and then return two sets of vertex indices - the vertices corresponding to the pair of triangle strips on either side of each seam
	// UV seams are denoted by any unique vertices which share the same position but have different UV coordinates

	//imageTex = new imageTexture(demoMat);

	std::vector<Vertex> vertices = target->vertices;

	std::vector<std::array<uint32_t, 2>> seamVertexPairs{};
	std::unordered_map<glm::vec3, std::vector<uint32_t>> uniquePositions{};
	
	for (uint32_t i = 0; i != vertices.size(); i++) {
		if (uniquePositions.count(vertices[i].pos) == 0) {
			uniquePositions.insert({ vertices[i].pos, std::vector<uint32_t>{i} });
		}
		else {
			uniquePositions.at(vertices[i].pos).push_back(i);
		}
	}

	for (auto it = uniquePositions.begin(); it != uniquePositions.end();) {
		if (it->second.size() == 1) {
			it = uniquePositions.erase(it);
		}
		else {
			it++;
		}
	}

	// Now uniquePositions contains the set of all positions which share multiple vertices
	for (auto elem : uniquePositions) {
		for (uint32_t i = 0; i != elem.second.size(); i++) {
			for (uint32_t j = 0; j != elem.second.size(); j++) {
				if (i != j) {
					seamVertexPairs.push_back(std::array<uint32_t, 2>{elem.second[i], elem.second[j]});	
				}
			}
		}
	}

	// We should now have a vector containing the indices of all pairs of vertices that mark the seams of the UV map
	// The aim is now to separate individual seams and determine which side of the seam each vertex falls on i.e. we construct the chain of vertices on either side of each
	// In the indices of the mesh vertices which correspond to triangles are arranged in triples; therefore if multiple seam vertices appear in the same set of three indices we know that these vertices are on the same side of a seam

	if (targetTex->texMat.empty()) {
		targetTex->getCVMat();
	}
	cv::Mat demoMat = targetTex->texMat.clone();
	cv::resize(demoMat, demoMat, cv::Size(), 0.25f, 0.25f);

	width = targetTex->texWidth;
	height = targetTex->texHeight;

	cv::Mat cleanDemoMat = demoMat.clone();

	demoMat = cleanDemoMat.clone();

	while (seamVertexPairs.size() > 1) {
		SeamStrip newSeamStrip;
		std::vector<uint32_t> chainIndices{};

		getStripChain(newSeamStrip, 0, seamVertexPairs, chainIndices);
		sortSeamIndices(newSeamStrip);

		if (newSeamStrip.leftIndices.size() <= 1) {
			seamVertexPairs[0] = std::array<uint32_t, 2>{ 0,0 };
			seamVertexPairs.erase(remove(seamVertexPairs.begin(), seamVertexPairs.end(), std::array<uint32_t, 2>{0, 0}), seamVertexPairs.end());
			continue;
		}

		seamStrips.push_back(newSeamStrip);

		sort(chainIndices.begin(), chainIndices.end());
		uint32_t sub = 0;
		for (uint32_t i : chainIndices) {
			if (i < seamVertexPairs.size()) {
				seamVertexPairs[i] = std::array<uint32_t, 2>{ 0,0 };
			}
		}

		seamVertexPairs.erase(remove(seamVertexPairs.begin(), seamVertexPairs.end(), std::array<uint32_t, 2>{0, 0}), seamVertexPairs.end());
	}

	for (uint32_t i = 0; i != seamStrips.size(); i++) {
		for (uint32_t j = 0; j != seamStrips.size(); j++) {
			if (i == j) {
				continue;
			}
			compareStrips(seamStrips[i], seamStrips[j], target->vertices);
		}
	}

	for (uint32_t i = 0; i != seamStrips.size(); i++){
		for (uint32_t j = 0; j != seamStrips[i].leftIndices.size(); j++) {
			uint32_t k = seamStrips[i].leftIndices[j];
			cv::Point L = cv::Point(vertices[k].texCoord.x * width, vertices[k].texCoord.y * height);
			k = seamStrips[i].rightIndices[j];
			cv::Point R = cv::Point(vertices[k].texCoord.x * width, vertices[k].texCoord.y * height);
			L *= 0.25f;
			R *= 0.25f;
			cv::circle(demoMat, L, 5, cv::Scalar(255 * static_cast<float>(j) / static_cast<float>(seamStrips[i].leftIndices.size()), 0, 0), -1);
			cv::circle(demoMat, R, 5, cv::Scalar(0, 255 * static_cast<float>(j) / static_cast<float>(seamStrips[i].rightIndices.size()), 0), -1);
			cv::line(demoMat, L, R, cv::Scalar(0, 0, 0), 1);
		}

		//std::cout << "Creating seam meshes" << std::endl;
		createSeamMeshes(seamStrips[i]);
		//std::cout << "Finished creating seam meshes" << std::endl;

		//seamStrips.push_back(newSeamStrip);

		//cv::imshow("Seam checker", demoMat);
		//cv::waitKey(0);
		//demoMat = cleanDemoMat.clone();

		//std::cout << "Seam done" << std::endl;
	}

	//std::cout << "Created all seam meshes" << std::endl;

	//std::cout << "Packing seam indices" << std::endl;
	packSeamStrips();
	//std::cout << "Done" << std::endl;
}

bool doLinesIntersect(glm::vec4 a, glm::vec4 b) {
	if (a == b) {
		//std::cout << "Intersect : Lines are equal" << std::endl;
		return true;
	}

	if (std::max(a[0], a[2]) < std::min(b[0], b[2]) || std::max(b[0], b[2]) < std::min(a[0], a[2])) {
	//	std::cout << "No intersect: x intervals don't overlap" << std::endl;
		return false;
	}

	if (std::max(a[1], a[3]) < std::min(b[1], b[3]) || std::max(b[1], b[3]) < std::min(a[1], a[3])) {
	//	std::cout << "No intersect: y intervals don't overlap" << std::endl;
		return false;
	}

	// We find the equations of the lines
	float a_m = (a[0] - a[2] == 0.0f) ? 1000.0f : (a[1] - a[3]) / (a[0] - a[2]);
	float b_m = (b[0] - b[2] == 0.0f) ? 1000.0f : (b[1] - b[3]) / (b[0] - b[2]);
	if (a_m == b_m) {
	//	std::cout << "No intersect: lines are parallel" << std::endl;
		return false; // The lines are parallel
	}
	float a_c = a[1] - a_m * a[0];
	float b_c = b[1] - b_m * b[0];

	// We find the x coordinate at which both lines will intersect
	float x_intersect = (b_c - a_c) / (a_m - b_m);

	// Finally we check if this x coordinate is within the bounds of both lines
	if (x_intersect < std::max(std::min(a[0], a[1]), std::min(b[0], b[1])) || x_intersect > std::min(std::max(a[0], a[1]), std::max(b[0], b[1]))) {
	//	std::cout << "No intersect: intersection point outside of line bounds" << std::endl;
		return false; // the intersection is outside the bounds of the lines
	}

	//std::cout << a[0] << " " << a[1] << " " << a[2] << " " << a[3] << std::endl;
	//std::cout << b[0] << " " << b[1] << " " << b[2] << " " << b[3] << std::endl;
	//std::cout << "Intersect" << std::endl;
	return true;
}

bool doMeshesOverlap(Mesh* a, Mesh* b) {
	float a_min_x = a->vertices[0].pos[0];
	float a_max_x = a->vertices[0].pos[1];
	float a_min_y = a->vertices[0].pos[0];
	float a_max_y = a->vertices[0].pos[1];

	for (uint32_t i = 0; i != a->vertices.size(); i++) {
		float x = a->vertices[i].pos[0];
		float y = a->vertices[i].pos[1];
		a_min_x = (x < a_min_x) ? x : a_min_x;
		a_max_x = (x > a_max_x) ? x : a_max_x;
		a_min_y = (y < a_min_y) ? y : a_min_y;
		a_max_y = (y > a_max_y) ? y : a_max_y;
	}

	float b_min_x = b->vertices[0].pos[0];
	float b_max_x = b->vertices[0].pos[1];
	float b_min_y = b->vertices[0].pos[0];
	float b_max_y = b->vertices[0].pos[1];

	for (uint32_t i = 0; i != b->vertices.size(); i++) {
		float x = b->vertices[i].pos[0];
		float y = b->vertices[i].pos[1];
		b_min_x = (x < b_min_x) ? x : b_min_x;
		b_max_x = (x > b_max_x) ? x : b_max_x;
		b_min_y = (y < b_min_y) ? y : b_min_y;
		b_max_y = (y > b_max_y) ? y : b_max_y;
	}

	bool overlap = false;
	bool overlap_x = a_max_x >= b_min_x and b_max_x >= a_min_x;
	bool overlap_y = a_max_y >= b_min_y and b_max_y >= a_min_y;
	overlap = (overlap_x and overlap_y);

	if (!overlap) {
		//std::cout << "Meshes overlap:" << std::endl;
		//std::cout << a_min_x << " " << a_max_x << " " << a_min_y << " " << a_max_y << std::endl;
		//std::cout << b_min_x << " " << b_max_x << " " << b_min_y << " " << b_max_y << std::endl;
		return false;
	}

	//if (b_TL[0] > a_BR[0] || b_BR[0] < a_TL[0] || b_TL[1] < a_BR[1] || b_BR[1] > a_TL[1]) {
	//	// If the bounding boxes don't overlap then the meshes won't overlap
	//	std::cout << "Meshes don't overlap:" << std::endl;
	//	std::cout << a_TL[0] << " " << a_TL[1] << " " << a_BR[0] << " " << a_BR[1] << std::endl;
	//	std::cout << b_TL[0] << " " << b_TL[1] << " " << b_BR[0] << " " << b_BR[1] << std::endl;
	//	return false;
	//}

	//std::cout << "Comparing triangles" << std::endl;

	size_t aSize = std::floor(a->indices.size() / 3) * 3;
	size_t bSize = std::floor(b->indices.size() / 3) * 3;
	for (uint32_t i = 0; i != aSize; i += 3) {
		uint32_t a_i = a->indices[i];
		glm::vec4 a_1{ a->vertices[a_i].pos[0], a->vertices[a_i].pos[1], a->vertices[a_i + 1].pos[0], a->vertices[a_i + 1].pos[1] };
		glm::vec4 a_2{ a->vertices[a_i+1].pos[0], a->vertices[a_i+1].pos[1], a->vertices[a_i + 2].pos[0], a->vertices[a_i + 2].pos[1] };
		glm::vec4 a_3{ a->vertices[a_i+2].pos[0], a->vertices[a_i+2].pos[1], a->vertices[a_i].pos[0], a->vertices[a_i].pos[1] };
		for (uint32_t j = 0; j != bSize; j += 3) {

			uint32_t b_i = b->indices[j];
			glm::vec4 b_1{ b->vertices[b_i].pos[0], b->vertices[b_i].pos[1], b->vertices[b_i + 1].pos[0], b->vertices[b_i + 1].pos[1] };
			glm::vec4 b_2{ b->vertices[b_i + 1].pos[0], b->vertices[b_i + 1].pos[1], b->vertices[b_i + 2].pos[0], b->vertices[b_i + 2].pos[1] };
			glm::vec4 b_3{ b->vertices[b_i + 2].pos[0], b->vertices[b_i + 2].pos[1], b->vertices[b_i].pos[0], b->vertices[b_i].pos[1] };

			if (doLinesIntersect(a_1, b_1) || doLinesIntersect(a_1, b_2) || doLinesIntersect(a_1, b_3)) {
				return true;
			}
			if (doLinesIntersect(a_2, b_1) || doLinesIntersect(a_2, b_2) || doLinesIntersect(a_2, b_3)) {
				return true;
			}
			if (doLinesIntersect(a_3, b_1) || doLinesIntersect(a_3, b_2) || doLinesIntersect(a_3, b_3)) {
				return true;
			}
		}
	}

	return false;
}

bool doStripsOverlap(SeamStrip* a, SeamStrip* b) {
	if (doMeshesOverlap(&a->leftMesh, &b->leftMesh)) {
		std::cout << "Left meshes overlap" << std::endl;
		return true;
	}
	if (doMeshesOverlap(&a->rightMesh, &b->rightMesh)) {
		std::cout << "Right meshes overlap" << std::endl;
		return true;
	}
	return false;
}

void SeamFixer::packSeamStrips() {
	// Attempts to condense non-overlapping seam strips into a smaller subset of seam strips
	
	sortedSeamStrips.clear();
	sortedLeftMeshes.clear();
	sortedLeftAlphaMeshes.clear();
	sortedRightMeshes.clear();
	sortedRightAlphaMeshes.clear();

	std::map<uint32_t, std::vector<uint32_t>> intersecting_strips{};

	for (uint32_t i = 0; i != seamStrips.size(); i++) {
		for (uint32_t j = 0; j != seamStrips.size(); j++) {
			if (intersecting_strips.count(j) != 0){
				if (find(intersecting_strips.at(j).begin(), intersecting_strips.at(j).end(), i) != intersecting_strips.at(j).end()) {
					continue;
				}
			}
			if (j != i && doStripsOverlap(&seamStrips[i], &seamStrips[j])){
				//std::cout << "Strips " << i << " and " << j << " overlap" << std::endl;
				if (intersecting_strips.count(i) == 0) {
					intersecting_strips.insert({ i, std::vector<uint32_t>{j} });
				}
				else {
					intersecting_strips.at(i).push_back(j);
				}
				if (intersecting_strips.count(j) == 0) {
					intersecting_strips.insert({ j, std::vector<uint32_t>{i} });
				}
				else {
					intersecting_strips.at(j).push_back(i);
				}
			}
		}
	}

	for (auto elem: intersecting_strips) {
		std::cout << elem.first << ": ";
		for (uint32_t j = 0; j != elem.second.size(); j++) {
			std::cout << elem.second[j] << " ";
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;

	std::vector<bool> consumed(seamStrips.size(), false);

	while (find(consumed.begin(), consumed.end(), false) != consumed.end()) {
		std::vector<uint32_t> stripSetIndices{};
		for (uint32_t i = 0; i != seamStrips.size(); i++) {
			if (consumed[i]) {
				continue;
			}
			if (stripSetIndices.size() == 0) {
				stripSetIndices.push_back(i);
				consumed[i] = true;
			}
			else {
				if (intersecting_strips.count(i) == 0) {
					stripSetIndices.push_back(i);
					consumed[i] = true;
				}
				else {
					bool canBeInserted = true;
					for (uint32_t j : stripSetIndices) {
						if (intersecting_strips.count(j) > 0) {
							for (uint32_t elem : intersecting_strips.at(j)) {
								if (elem == i) {
									canBeInserted = false;
									break;
								}
							}
						}
					}
					if (canBeInserted) {
						stripSetIndices.push_back(i);
						consumed[i] = true;
					}
				}
			}
		}
		sortedSeamStrips.push_back(stripSetIndices);
	}
	for (uint32_t i = 0; i != sortedSeamStrips.size(); i++) {
		std::vector<Mesh*> rightMeshes{};
		std::vector<Mesh*> leftMeshes{};
		std::vector<Mesh*> rightAlphaMeshes{};
		std::vector<Mesh*> leftAlphaMeshes{};
		for (uint32_t j = 0; j != sortedSeamStrips[i].size(); j++) {
			std::cout << sortedSeamStrips[i][j] << " ";
			leftMeshes.push_back(&seamStrips[sortedSeamStrips[i][j]].leftMesh);
			rightMeshes.push_back(&seamStrips[sortedSeamStrips[i][j]].rightMesh);
			leftAlphaMeshes.push_back(&seamStrips[sortedSeamStrips[i][j]].leftAlphaMesh);
			rightAlphaMeshes.push_back(&seamStrips[sortedSeamStrips[i][j]].rightAlphaMesh);
		}
		std::cout << std::endl;
		sortedLeftMeshes.push_back(leftMeshes);
		sortedLeftAlphaMeshes.push_back(leftAlphaMeshes);
		sortedRightMeshes.push_back(rightMeshes);
		sortedRightAlphaMeshes.push_back(rightAlphaMeshes);
	}
	std::cout << std::endl;
}

void SeamFixer::prepMap(OverlayMap* map) {

	map->colour = new Texture;

	map->colour->texWidth = width;
	map->colour->texHeight = height;
	map->colour->texChannels = 4;
	map->colour->mipLevels = 1;
	map->colour->textureFormat = VK_FORMAT_R8G8B8A8_SRGB;
	map->colour->textureLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	map->colour->textureUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

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
	return drawColourMap(commandbuffer, map, std::vector<Mesh*>{mesh});
};

VkCommandBuffer SeamFixer::drawColourMap(VkCommandBuffer commandbuffer, OverlayMap* map, std::vector<Mesh*> meshes) {
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

	for (Mesh* mesh:meshes) {
		VkBuffer vertexBuffers[] = { mesh->vertexBuffer };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(commandbuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandbuffer, mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, map->pipelineLayout, 0, 1, &map->descriptorSet, 0, nullptr);

		vkCmdDrawIndexed(commandbuffer, static_cast<uint32_t>(mesh->indices.size()), 1, 0, 0, 0);
	}
	
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
	return drawAlphaMap(commandbuffer, map, std::vector<Mesh*>{mesh});
}

VkCommandBuffer SeamFixer::drawAlphaMap(VkCommandBuffer commandbuffer, OverlayMap* map, std::vector<Mesh*> meshes) {
	VkClearValue clearValues[1] = {};
	clearValues[0].color = { {0.0f, 0.0f, 1.0f / 255.0f, 1.0f} };

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

	for (Mesh* mesh : meshes) {
		VkBuffer vertexBuffers[] = { mesh->vertexBuffer };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(commandbuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandbuffer, mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(commandbuffer, static_cast<uint32_t>(mesh->indices.size()), 1, 0, 0, 0);
	}

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

void SeamFixer::alphaOverMap(Texture* cMap, Texture* aMap, Texture* texInFlight) {
	Texture* seamMap = cMap->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height);
	Texture* seamAlpha = aMap->copyTexture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height);

	filter alphaOver(std::vector<Texture*>{texInFlight, seamMap, seamAlpha}, new IPALPHAOVERSHADER, VK_FORMAT_R8G8B8A8_UNORM);
	alphaOver.filterImage();

	vkDeviceWaitIdle(Engine::get()->device);

	seamMap->cleanup();
	seamAlpha->cleanup();
	alphaOver.cleanup(false);
};

void SeamFixer::createDemoImages(std::string baseName) {
	// Image with strips in a separate image

	std::vector<Mesh*> rightColourMeshes{};
	std::vector<Mesh*> rightAlphaMeshes{};

	std::vector<Mesh*> leftColourMeshes{};
	std::vector<Mesh*> leftAlphaMeshes{};

	std::vector<Mesh*> outsideMeshes{};
	std::vector<Mesh*> insideMeshes{};

	updateSeamMeshes(0.025f, false, true);
	
	for (uint32_t i = 0; i != seamStrips.size(); i++) {
		if (seamStrips[i].leftIndices.size() == 0) {
			continue;
		}

		Mesh* rightOutsideMesh = new Mesh();
		Mesh* leftOutsideMesh = new Mesh();

		Mesh* rightInsideMesh = new Mesh();
		Mesh* leftInsideMesh = new Mesh();
		
		rightColourMeshes.push_back(&seamStrips[i].rightMesh);
		rightAlphaMeshes.push_back(&seamStrips[i].rightAlphaMesh);

		size_t rSize = seamStrips[i].rightMesh.indices.size() / 2;
		size_t lSize = seamStrips[i].leftMesh.indices.size() / 2;

		rightOutsideMesh->vertices = seamStrips[i].rightMesh.vertices;
		rightInsideMesh->vertices = seamStrips[i].rightMesh.vertices;

		leftOutsideMesh->vertices = seamStrips[i].leftMesh.vertices;
		leftInsideMesh->vertices = seamStrips[i].leftMesh.vertices;
		
		for (uint32_t j = 0; j != rSize; j++) {
			rightOutsideMesh->indices.push_back(seamStrips[i].rightMesh.indices[j]);
			rightInsideMesh->indices.push_back(seamStrips[i].rightMesh.indices[j + rSize]);
		}
			
		leftColourMeshes.push_back(&seamStrips[i].leftMesh);
		leftAlphaMeshes.push_back(&seamStrips[i].leftAlphaMesh);

		for (uint32_t j = 0; j != lSize; j++) {
			leftOutsideMesh->indices.push_back(seamStrips[i].leftMesh.indices[j]);
			leftInsideMesh->indices.push_back(seamStrips[i].leftMesh.indices[j + lSize]);
		}

		rightOutsideMesh->setup();
		rightInsideMesh->setup();

		leftOutsideMesh->setup();
		leftInsideMesh->setup();

		insideMeshes.push_back(leftInsideMesh);
		insideMeshes.push_back(rightInsideMesh);

		outsideMeshes.push_back(leftOutsideMesh);
		outsideMeshes.push_back(rightOutsideMesh);
	}

	VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
	commandBuffer = drawColourMap(commandBuffer, &colourMap, rightColourMeshes);
	commandBuffer = drawAlphaMap(commandBuffer, &alphaMap, rightAlphaMeshes);
	Engine::get()->endSingleTimeCommands(commandBuffer);

	Texture* rightCol = colourMap.colour->copyTexture();
	Texture* rightAlpha = alphaMap.colour->copyTexture();

	rightCol->getCVMat();
	rightAlpha->getCVMat();

	cv::Mat rC = rightCol->texMat.clone();
	cv::Mat rA = rightAlpha->texMat.clone();

	cv::Mat rThresh;
	cv::cvtColor(rC, rThresh, cv::COLOR_BGR2GRAY);
	cv::threshold(rThresh, rThresh, 0, 255, cv::THRESH_BINARY);

	cv::imwrite(baseName + std::string("_R_Strips.jpg"), rC);
	cv::imwrite(baseName + std::string("_R_SolidAlpha.jpg"), rThresh);
	cv::imwrite(baseName + std::string("_R_BlendAlpha.jpg"), rA);

	if (remapper == nullptr) {
		createRemapper();
		seamFixAll();
	}

	remapper->cleanup();

	remapper->setup();
	remapper->toggleNormalization();
	remapper->createReferenceMaps(colourMap.colour, alphaMap.colour);

	remapper->filteredTarget->getCVMat();
	cv::imwrite(baseName + std::string("_R_RemapAlpha.jpg"), remapper->filteredTarget->texMat);

	remapper->filteredTarget->destroyCVMat();

	commandBuffer = Engine::get()->beginSingleTimeCommands();
	commandBuffer = drawColourMap(commandBuffer, &colourMap, leftColourMeshes);
	commandBuffer = drawAlphaMap(commandBuffer, &alphaMap, leftAlphaMeshes);
	Engine::get()->endSingleTimeCommands(commandBuffer);

	Texture* leftCol = colourMap.colour->copyTexture();
	Texture* leftAlpha = alphaMap.colour->copyTexture();

	leftCol->getCVMat();
	leftAlpha->getCVMat();

	cv::Mat lC = leftCol->texMat.clone();
	cv::Mat lA = leftAlpha->texMat.clone();

	cv::Mat lThresh;
	cv::cvtColor(lC, lThresh, cv::COLOR_BGR2GRAY);
	cv::threshold(lThresh, lThresh, 0, 255, cv::THRESH_BINARY);

	cv::imwrite(baseName + std::string("_L_Strips.jpg"), lC);
	cv::imwrite(baseName + std::string("_L_SolidAlpha.jpg"), lThresh);
	cv::imwrite(baseName + std::string("_L_BlendAlpha.jpg"), lA);

	remapper->cleanup();

	remapper->setup();
	remapper->toggleNormalization();
	remapper->createReferenceMaps(colourMap.colour, alphaMap.colour);

	remapper->filteredTarget->getCVMat();
	cv::imwrite(baseName + std::string("_L_RemapAlpha.jpg"), remapper->filteredTarget->texMat);

	commandBuffer = Engine::get()->beginSingleTimeCommands();
	commandBuffer = drawAlphaMap(commandBuffer, &alphaMap, outsideMeshes);
	Engine::get()->endSingleTimeCommands(commandBuffer);

	Texture* outsideAlpha = alphaMap.colour->copyTexture();
	outsideAlpha->getCVMat();

	cv::Mat outsideThresh;
	cv::cvtColor(outsideAlpha->texMat, outsideThresh, cv::COLOR_BGR2GRAY);
	cv::threshold(outsideThresh, outsideThresh, 1, 255, cv::THRESH_BINARY);

	cv::imwrite(baseName + std::string("_OutsideAlpha.jpg"), outsideThresh);

	commandBuffer = Engine::get()->beginSingleTimeCommands();
	commandBuffer = drawAlphaMap(commandBuffer, &alphaMap, insideMeshes);
	Engine::get()->endSingleTimeCommands(commandBuffer);

	Texture* insideAlpha = alphaMap.colour->copyTexture();
	insideAlpha->getCVMat();

	cv::Mat insideThresh;
	cv::cvtColor(insideAlpha->texMat, insideThresh, cv::COLOR_BGR2GRAY);
	cv::threshold(insideThresh, insideThresh, 1, 255, cv::THRESH_BINARY);

	cv::imwrite(baseName + std::string("_InsideAlpha.jpg"), insideThresh);

	updateSeamMeshes(stripSize, false, false);
}

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
	for (uint32_t i = 0; i != sortedLeftMeshes.size(); i++) {
		std::cout << i << " " << sortedSeamStrips.size() << std::endl;
		std::vector<Mesh*> colourMeshes{};
		std::vector<Mesh*> alphaMeshes{};
		if (isRight) {
			colourMeshes = sortedRightMeshes[i];
			alphaMeshes = sortedRightAlphaMeshes[i];
		}
		else {
			colourMeshes = sortedLeftMeshes[i];
			alphaMeshes = sortedLeftAlphaMeshes[i];
		}
		VkCommandBuffer commandBuffer = Engine::get()->beginSingleTimeCommands();
		commandBuffer = drawColourMap(commandBuffer, &colourMap, colourMeshes);
		commandBuffer = drawAlphaMap(commandBuffer, &alphaMap, alphaMeshes);
		

		if (useRemapper && remapper != nullptr) {
			Engine::get()->endSingleTimeCommands(commandBuffer);

			remapper->cleanup();

			remapper->setup();
			remapper->toggleNormalization();
			remapper->createReferenceMaps(colourMap.colour, alphaMap.colour);
			
			//colourMap.colour->getCVMat();
			//cv::imshow("remapped alpha", colourMap.colour->texMat);
			//cv::waitKey(0);
			//colourMap.colour->destroyCVMat();

			//remapper->filteredTarget->getCVMat();
			//cv::imshow("remapped alpha", remapper->filteredTarget->texMat);
			//cv::waitKey(0);
			//remapper->filteredTarget->destroyCVMat();

			alphaOverMap(colourMap.colour, remapper->filteredTarget, writeTex);
		}
		else {
			Engine::get()->endSingleTimeCommands(commandBuffer);
			alphaOverMap(colourMap.colour, alphaMap.colour, writeTex);
		}
	}
	Texture* res = writeTex->copyTexture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_TILING_OPTIMAL, 1, width, height);
	textureLL->replacePtr(res, targetTexName);

	writeTex->cleanup();
}