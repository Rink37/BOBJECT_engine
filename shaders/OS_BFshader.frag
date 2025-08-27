#version 450

precision mediump float;

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D normalSampler;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 vertPos;
layout(location = 2) in mat4 modelMat;

layout(location = 0) out vec4 outColor;

vec3 lightPos = vec3(5.0, 0.0, 0.0);
const vec3 lightColor = vec3(1.0, 1.0, 1.0);
const float lightPower = 40.0;
const vec3 ambientLighting = vec3(0.812, 0.537, 0.514);
float ambientScale = 0.2;
const vec3 specColor = vec3(1.0, 1.0, 1.0);
const float shininess = 16.0;

void main(){
	vec4 tex = texture(texSampler, fragTexCoord);

	vec4 ambient = vec4(tex.rgb*ambientLighting*ambientScale, tex.a); 

	vec4 convertedNormal = modelMat * vec4(texture(normalSampler, fragTexCoord).rgb * 2.0 - 1.0, 1.0);
	vec3 normal = normalize(convertedNormal.rgb);

	vec3 lightDir = lightPos-vertPos;
	float distance = dot(lightDir, lightDir);
	lightDir = normalize(lightDir);

	float lambertian = max(dot(lightDir, normal), 0.0);
	float specular = 0.0f;

	vec4 diffuse = vec4(tex.rgb*lambertian*lightColor.rgb*lightPower/distance, tex.a); 

	if (lambertian > 0.0){
		vec3 viewDir = normalize(-vertPos);
		vec3 halfDir = normalize(lightDir + viewDir);

		float specAngle = max(dot(halfDir, normal), 0.0);

		specular =  pow(specAngle, shininess);
	}

	vec4 specularOut = vec4(tex.rgb*specColor.rgb*specular*lightColor.rgb*lightPower/distance, tex.a);

	outColor = ambient + diffuse + specularOut;
}