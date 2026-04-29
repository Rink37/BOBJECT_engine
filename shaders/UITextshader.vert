#version 450

layout(binding = 0) uniform ColourSchemeObject {
	vec3 Primary;
	vec3 Secondary;
	vec3 Tertiary;
} cso;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 primaryColour;
layout(location = 2) out vec3 secondaryColour;

void main(){
	gl_Position = vec4(inPosition, 1.0);
	fragTexCoord = inTexCoord;
	primaryColour = cso.Primary;
	secondaryColour = cso.Secondary;
}