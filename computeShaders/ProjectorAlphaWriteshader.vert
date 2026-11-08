#version 450

layout(binding = 0, std140) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
	vec4 UVdistort;
	vec3 backgroundColour;
	vec3 lightPosition;
	vec3 cameraPosition;
} ubo;
layout(binding = 1) uniform ProjectorPositionObject{
	vec4 projectorPosition;
} ppo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 vertPos;
layout(location = 1) out vec3 normalInterp;
layout(location = 3) out vec3 projectorPos;

void main(){
	gl_Position = vec4(inTexCoord*2.0 - 1.0, 0.0, 1.0);
	vec4 vertPos4 = ubo.model*vec4(inPosition, 1.0);
	vertPos = vec3(vertPos4)/vertPos4.w;
	normalInterp = vec4(ubo.model * vec4(inNormal, 1.0)).xyz;
	projectorPos = ppo.projectorPosition.xyz;
}