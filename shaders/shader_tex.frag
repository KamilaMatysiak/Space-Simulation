#version 430 core

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

struct PointLight {
	vec3 position;
	vec3 color;
	float intensity;
};

#define MAX_POINT_LIGHTS 16

uniform vec3 cameraPos;
uniform sampler2D diffuseTexture;
uniform sampler2D normalTexture;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int LightsCount;

in vec3 interpNormal;
in vec3 fragPos;
in vec2 vTexCoord;



void main()
{
	vec3 fragColor = vec3(0,0,0);
	vec3 texture = texture2D(diffuseTexture, vTexCoord).rgb;
	vec3 ambient = vec3(0.2) * texture;
	vec3 normal = normalize(interpNormal);
	vec3 V = normalize(cameraPos-fragPos);

	for(int i = 0; i < LightsCount; i++)
	{
		vec3 lightDir = normalize(pointLights[i].position - fragPos);
		vec3 R = reflect(-lightDir,normal);

		float dist = distance(fragPos, pointLights[i].position);
		float distance = (1/dist) * (1/dist);
	
		float spec = pow(max(0,dot(R,V)),2);
		float diff = max(0,dot(normal,normalize(lightDir)));

		vec3 diffuse = pointLights[i].color * diff * distance * pointLights[i].intensity;
		vec3 specular = spec * pointLights[i].color * (pointLights[i].intensity/(dist*5));

		fragColor += texture*(diffuse+specular);
	}

    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
	FragColor = vec4(fragColor+ambient,1.0);
}
