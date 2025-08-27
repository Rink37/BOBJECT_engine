#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 normalInterps;

void main(){
	gl_Position = vec4(inTexCoord[0]*2 - 1, inTexCoord[1]*2 - 1, 0.0, 1.0);
	normalInterps = inNormal;
}