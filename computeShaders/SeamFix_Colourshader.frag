#version 450

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D imageSampler;

layout(location = 0) in vec2 fragTexCoord;

void main(){
	outColor = vec4(texture(imageSampler, fragTexCoord).rgb, 1.0);
}