#version 450

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

const vec3 primary = vec3(0, 0, 0);
const vec3 secondary = vec3(1, 0, 0);

void main(){
	outColor = vec4(primary + texture(texSampler, fragTexCoord).r*secondary, 1.0);
}