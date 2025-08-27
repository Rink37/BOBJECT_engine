#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 normalInterp;

void main(){
	vec3 translatedNormals = normalize(normalInterp*0.5+0.5);
	outColor = vec4(translatedNormals, 1.0);
}