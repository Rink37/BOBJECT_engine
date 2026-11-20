#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 primaryColour;
layout(location = 2) in vec3 secondaryColour;
layout(location = 3) in vec2 winSize;
layout(location = 4) in vec3 meshSizes;

layout(location = 0) out vec4 outColor;

void main(){
	float secondaryBlend = 1.0f;
	float lineThickness = 3;
	float innerRadius = 15; // Inner radius
	float scaleFac = 1.0f / (meshSizes.x * 2.0f * winSize.x); // Converts a pixel width to a UV-coordinate area width
	float innerDistance = innerRadius * scaleFac;
	float outerDistance = (lineThickness + innerRadius) * scaleFac;
	float aspectRatio = winSize.x / winSize.y;
	aspectRatio *= meshSizes.z;
	ivec2 pixelCoord = ivec2(gl_FragCoord.xy);
	vec2 edgeDistances = vec2(min(fragTexCoord.x, 1-fragTexCoord.x), min(fragTexCoord.y, 1-fragTexCoord.y) / aspectRatio);
	edgeDistances.x = (outerDistance - edgeDistances.x < 0.0f) ? 0.0f : outerDistance - edgeDistances.x;
	edgeDistances.y = (outerDistance - edgeDistances.y < 0.0f) ? 0.0f : outerDistance - edgeDistances.y; 
	innerDistance *= innerDistance;
	outerDistance *= outerDistance;
	float rad = edgeDistances.x * edgeDistances.x + edgeDistances.y * edgeDistances.y;
	if (rad > outerDistance){
		discard;
	} 
	if (rad > innerDistance * 0.8f){
		secondaryBlend = 0.0f;
	}
	vec3 mixColour = primaryColour*(1.0-secondaryBlend) + secondaryBlend*secondaryColour;
	outColor = vec4(mixColour, 1.0);
}