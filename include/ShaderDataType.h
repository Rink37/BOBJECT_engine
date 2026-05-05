#ifndef SHADERDATA
#define SHADERDATA

#include<vector>

#define MESH_SHADER 0
#define COMP_SHADER 1

struct shaderIOValue {
    shaderIOValue(std::string n, int t, bool d) {
        name = n;
        type = t;
        direction = d;
    }

    std::string name = "";
    int type = 0;
    bool direction = true;
};

struct shaderData{
    shaderData(const std::vector<unsigned char>* f, const std::vector<unsigned char>* v, bool w){
        fragData = f;
        vertData = v;
        isWireframe = w;
        type = MESH_SHADER;
    }

    shaderData(const std::vector<unsigned char>* c, bool w) {
        compData = c;
        isWireframe = w;
        type = COMP_SHADER;
    }

    shaderData(const std::vector<unsigned char>* c, bool w, std::vector<shaderIOValue> IOvalues) {
        compData = c;
        isWireframe = w;
        type = COMP_SHADER;
        IO = IOvalues;
    }

    const std::vector<unsigned char>* fragData = nullptr;
    const std::vector<unsigned char>* vertData = nullptr;
    const std::vector<unsigned char>* compData = nullptr;

    std::vector<shaderIOValue> IO;

    bool isWireframe = false;
    uint8_t type = MESH_SHADER;
};

#endif