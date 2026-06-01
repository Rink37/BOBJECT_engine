#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 primaryColour;
layout(location = 2) in vec3 secondaryColour;

layout(location = 0) out vec4 outColor;

void main(){
	float secondaryBlend = 1.0f;
	if (fragTexCoord.x < 0.01f || fragTexCoord.x > 0.99f){
		secondaryBlend = 0.0f;
	} 
	if (fragTexCoord.y < 0.01f || fragTexCoord.y > 0.99f){
		secondaryBlend = 0.0f;
	} 
	vec3 mixColour = primaryColour*(1.0-secondaryBlend) + secondaryBlend*secondaryColour;
	outColor = vec4(mixColour, 1.0);
}