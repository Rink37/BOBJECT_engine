#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 primaryColour;
layout(location = 2) in vec3 secondaryColour;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main(){
	float secondaryBlend = texture(texSampler, fragTexCoord).r;
	outColor = vec4(primaryColour*(1.0-secondaryBlend) + secondaryBlend*secondaryColour, 1.0);
}