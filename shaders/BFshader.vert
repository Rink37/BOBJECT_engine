#version 450

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
	vec4 UVdistort;
	vec3 backgroundColour;
	vec3 lightPosition;
	vec3 cameraPosition;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 normalInterps;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 vertPos;
layout(location = 3) out vec3 ambientLighting;
layout(location = 4) out vec3 lightPos;
layout(location = 5) out vec3 cameraPos;

vec3 normalInterp;

void main(){
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
	normalInterps = vec4(ubo.model * vec4(inNormal, 1.0)).xyz;
	fragTexCoord = inTexCoord;
	vec4 vertPos4 = ubo.model*vec4(inPosition, 1.0);
	vertPos = vec3(vertPos4)/vertPos4.w;
	ambientLighting = ubo.backgroundColour;
	lightPos = ubo.lightPosition;
	cameraPos = ubo.cameraPosition;
}