#version 450

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D objectSpaceNormalSampler;

layout(location = 0) in vec3 normalInterp;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in mat3 objectToTangent;
layout(location = 5) in float w;

void main(){
	vec3 objectNormal = normalize(texture(objectSpaceNormalSampler, fragTexCoord).rgb * 2.0 - 1.0);
	vec3 tangentNormal = objectToTangent * objectNormal;
	//tangentNormal  = vec3(tangentNormal.x, tangentNormal.y * -1.0, tangentNormal.z);
	tangentNormal = normalize(tangentNormal) * 0.5 + 0.5;
	outColor = vec4(tangentNormal, 1.0);
}