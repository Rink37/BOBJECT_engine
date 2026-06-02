#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 primaryColour;
layout(location = 2) in vec3 secondaryColour;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

float median(float a, float b, float c){
	return max(min(a, b), min(max(a, b), c));
}

void main(){
	vec3 pix = texture(texSampler, fragTexCoord).rgb;
	
	float d = median(pix.r, pix.g, pix.b) - 0.5;

	float w = clamp(d/fwidth(d) + 0.5, 0.0, 1.0); // The anti-aliased alpha value

	if (d <= 0.0){
		discard;
	}
	outColor = vec4(primaryColour, w);
}