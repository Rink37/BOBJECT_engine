#ifndef SHADERDATA
#define SHADERDATA

#include<vector>

struct shaderData{
    shaderData(std::vector<unsigned char> f, std::vector<unsigned char> v, bool w){
        fragData = f;
        vertData = v;
        isWireframe = w;
    }

    std::vector<unsigned char> fragData;
    std::vector<unsigned char> vertData;

    bool isWireframe;
};

#endif