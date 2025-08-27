#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inTangent;

layout(location = 0) out vec3 normalInterps;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out mat3 objectToTangent;
layout(location = 5) out float w;

mat3 TBN = transpose(mat3(
	normalize(inTangent.xyz * inTangent.w),
	normalize(cross(inTangent.xyz * inTangent.w, inNormal)),
	normalize(inNormal)
));

void main(){
	gl_Position = vec4(inTexCoord[0]*2 - 1, inTexCoord[1]*2 - 1, 0.0, 1.0);
	normalInterps = inNormal;
	fragTexCoord = inTexCoord;
	objectToTangent = TBN;
	w = inTangent.w;
}