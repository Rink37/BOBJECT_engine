#ifndef SHADERDATA
#define SHADERDATA

#include<vector>
#include<string>

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

    shaderData(const std::vector<unsigned char>* f, const std::vector<unsigned char>* v, bool w, std::string name) {
        fragData = f;
        vertData = v;
        isWireframe = w;
        type = MESH_SHADER;
        shaderName = name;
    }

    shaderData(const std::vector<unsigned char>* f, const std::vector<unsigned char>* v, bool w, std::vector<shaderIOValue> IOvalues) {
        fragData = f;
        vertData = v;
        isWireframe = w;
        type = MESH_SHADER;
        IO = IOvalues;
    }

    shaderData(const std::vector<unsigned char>* f, const std::vector<unsigned char>* v, bool w, std::vector<shaderIOValue> IOvalues, std::string name) {
        fragData = f;
        vertData = v;
        isWireframe = w;
        type = MESH_SHADER;
        IO = IOvalues;
        shaderName = name;
    }

    shaderData(const std::vector<unsigned char>* c, bool w) {
        compData = c;
        isWireframe = w;
        type = COMP_SHADER;
    }

    shaderData(const std::vector<unsigned char>* c, bool w, std::string name) {
        compData = c;
        isWireframe = w;
        type = COMP_SHADER;
        shaderName = name;
    }

    shaderData(const std::vector<unsigned char>* c, bool w, std::vector<shaderIOValue> IOvalues) {
        compData = c;
        isWireframe = w;
        type = COMP_SHADER;
        IO = IOvalues;
    }

    shaderData(const std::vector<unsigned char>* c, bool w, std::vector<shaderIOValue> IOvalues, std::string name) {
        compData = c;
        isWireframe = w;
        type = COMP_SHADER;
        IO = IOvalues;
        shaderName = name;
    }

    const std::vector<unsigned char>* fragData = nullptr;
    const std::vector<unsigned char>* vertData = nullptr;
    const std::vector<unsigned char>* compData = nullptr;

    std::vector<shaderIOValue> IO;

    bool isWireframe = false;
    uint8_t type = MESH_SHADER;
    
    std::string shaderName = "";
};

#endif