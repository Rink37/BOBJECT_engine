<<<<<<< HEAD
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

=======
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

>>>>>>> 65e49fd884fc33b59605b3036ff7b8ff8393947b
#endif