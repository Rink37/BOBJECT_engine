#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

const mat4 ditherMatrix = mat4(0.0, 0.5, 0.125, 0.625,
0.75, 0.25, 0.875, 0.375,
0.1875, 0.6875, 0.0625, 0.5625,
0.9375, 0.4375, 0.8125, 0.3125);

void main(){
	vec4 tex = texture(texSampler, fragTexCoord);
	ivec2 pixelCoord = ivec2(gl_FragCoord.xy);
	float ditherThresh = ditherMatrix[pixelCoord.x%4][pixelCoord.y%4];
	if (tex.w <= ditherThresh){
		discard;
	}
	outColor = texture(texSampler, fragTexCoord);
}