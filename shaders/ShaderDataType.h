#ifndef SHADERDATA
#define SHADERDATA

#include<vector>

struct shaderData{
    shaderData(std::vector<char> f, std::vector<char> v){
        fragData = f;
        vertData = v;
    }

    std::vector<char> fragData;
    std::vector<char> vertData;
};

#endif