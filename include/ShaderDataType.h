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
        type = MESH_SHADER;
    }

    shaderData(std::vector<unsigned char> c, bool w) {
        compData = c;
        isWireframe = w;
        type = COMP_SHADER;
    }

    shaderData(std::vector<unsigned char> c, bool w, std::map<std::string, int> bindings, std::vector<bool> directions) {
        compData = c;
        isWireframe = w;
        type = COMP_SHADER;
        bindingMap = bindings;
        bindingDirections = directions;
    }

    std::vector<unsigned char> fragData;
    std::vector<unsigned char> vertData;
    std::vector<unsigned char> compData;

    bool isWireframe = false;
    uint8_t type = MESH_SHADER;

    std::map<std::string, int> bindingMap;
    std::vector<bool> bindingDirections;
};

#endif