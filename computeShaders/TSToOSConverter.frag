#version 450

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D tangentSpaceNormalSampler;

layout(location = 0) in vec3 normalInterp;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in mat3 tangentToObject;
layout(location = 5) in float w;

void main(){
	vec3 tangentNormal = normalize(texture(tangentSpaceNormalSampler, fragTexCoord).rgb * 2.0 - 1.0);
	vec3 objectNormal = tangentToObject * tangentNormal;
	objectNormal = normalize(objectNormal) * 0.5 + 0.5;
	outColor = vec4(objectNormal, 1.0);
}