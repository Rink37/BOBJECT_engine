#version 450

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 normalInterp;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in float w;
layout(location = 4) in vec3 backgroundColour;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main(){
	float transparency = 0.5f;
	outColor = texture(texSampler, fragTexCoord / w) * transparency + (1.0f - transparency) * vec4(backgroundColour, 1.0f);
}