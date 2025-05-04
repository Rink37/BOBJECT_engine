#ifndef PIPELINES
#define PIPELINES

#include<string>
#include<iostream>

struct Pipeline {

	// Contains information which is used to simplify the definition of graphics pipelines

	std::string vertPath;
	std::string fragPath;

	bool isWireframe = false;

	Pipeline(std::string vP, std::string fP) {
		vertPath = vP;
		fragPath = fP;
	}
	void setIsPipelineWireframe(bool iW) {
		isWireframe = iW;
	}
};


#endif