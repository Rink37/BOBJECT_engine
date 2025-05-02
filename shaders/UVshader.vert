#version 450

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
	vec4 UVdistort;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

void main(){
	gl_Position = vec4(inTexCoord[0]*ubo.UVdistort[0]+ubo.UVdistort[1], inTexCoord[1]*ubo.UVdistort[2]+ubo.UVdistort[3], 0.0, 1.0);
}