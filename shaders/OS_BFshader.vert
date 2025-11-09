#version 450

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
	vec4 UVdistort;
	vec3 backgroundColour;
	vec3 lightPosition;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 vertPos;
layout(location = 2) out mat4 modelMat;
layout(location = 7) out vec3 ambientLighting;
layout(location = 8) out vec3 lightPos;

vec3 normalInterp;

void main(){
	modelMat = ubo.model;
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
	fragTexCoord = inTexCoord;
	vec4 vertPos4 = ubo.view*ubo.model*vec4(inPosition, 1.0);
	vertPos = vec3(vertPos4)/vertPos4.w;
	ambientLighting = ubo.backgroundColour;
	lightPos = ubo.lightPosition;
}