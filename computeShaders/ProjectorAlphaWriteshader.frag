#version 450

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 normalInterp;
layout(location = 3) in vec3 projectorPos;

layout(location = 0) out vec4 outColor;

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
	outColor = vec4(transparency, transparency, transparency, 1.0);
}