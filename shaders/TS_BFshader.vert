#version 450

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
	vec4 UVdistort;
	vec3 backgroundColour;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inTangent;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 vertPos;
layout(location = 2) out mat3 tanMat;
layout(location = 7) out vec3 ambientLighting;

mat3 TBN = mat3(
	normalize(vec3(ubo.model*vec4(inTangent.xyz * inTangent.w, 0.0))),
	normalize(vec3(ubo.model*vec4(cross(inTangent.xyz * inTangent.w, inNormal), 0.0))),
	normalize(vec3(ubo.model*vec4(inNormal, 0.0)))
);

void main(){
	tanMat = TBN;
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
	fragTexCoord = inTexCoord;
	vec4 vertPos4 = ubo.view*ubo.model*vec4(inPosition, 1.0);
	vertPos = vec3(vertPos4)/vertPos4.w;
	ambientLighting = ubo.backgroundColour;
}