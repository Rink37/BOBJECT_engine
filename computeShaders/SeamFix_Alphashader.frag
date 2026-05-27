#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 fragTexCoord;

void main(){
	vec3 alpha = vec3(1.0 - fragTexCoord.y);
	outColor = vec4(alpha, 1.0);
}