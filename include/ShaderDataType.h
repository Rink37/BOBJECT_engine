#ifndef SHADERDATA
#define SHADERDATA

#include<vector>

#define MESH_SHADER 0
#define COMP_SHADER 1

struct shaderData{
    shaderData(std::vector<unsigned char> f, std::vector<unsigned char> v, bool w){
        fragData = f;
        vertData = v;
        isWireframe = w;
        type = 0;
    }

    shaderData(std::vector<unsigned char> c, bool w) {
        compData = c;
        isWireframe = w;
        type = 1;
    }

    shaderData(std::vector<unsigned char> c, bool w, std::map<std::string, int> bindings, std::vector<bool> directions) {
        compData = c;
        isWireframe = w;
        type = 1;
        bindingMap = bindings;
        bindingDirections = directions;
    }

    std::vector<unsigned char> fragData;
    std::vector<unsigned char> vertData;
    std::vector<unsigned char> compData;

    bool isWireframe = false;
    uint8_t type = 0;

    std::map<std::string, int> bindingMap;
    std::vector<bool> bindingDirections;
};

#endif