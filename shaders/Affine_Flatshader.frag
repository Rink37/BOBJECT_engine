#version 450

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 normalInterp;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in float w;
layout(location = 4) in vec3 projectorPos;

layout(location = 0) out vec4 outColor;

layout(binding = 2) uniform sampler2D texSampler;

const mat4 ditherMatrix = mat4(0.0, 0.5, 0.125, 0.625,
0.75, 0.25, 0.875, 0.375,
0.1875, 0.6875, 0.0625, 0.5625,
0.9375, 0.4375, 0.8125, 0.3125);


void main(){
	float dotprod = dot(normalize(projectorPos - vertPos), normalInterp);
	float angle = acos(dotprod);
	float transparency = cos(4*angle);
	if (-1.571 <= angle && angle <= 1.571){
		transparency = 1.0f;
	}
	if (angle <= -1.9634 || angle >= 1.9634){
		transparency = 0.0f;
	}
	
	ivec2 pixelCoord = ivec2(gl_FragCoord.xy);
	float ditherThresh = ditherMatrix[pixelCoord.x%4][pixelCoord.y%4];
	if (transparency <= ditherThresh){
		discard;
	}
	outColor = texture(texSampler, fragTexCoord / w);
}