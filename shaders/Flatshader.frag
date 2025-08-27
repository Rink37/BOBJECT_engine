<<<<<<< HEAD
#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main(){
	outColor = texture(texSampler, fragTexCoord);
=======
#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main(){
	outColor = texture(texSampler, fragTexCoord);
>>>>>>> 65e49fd884fc33b59605b3036ff7b8ff8393947b
}