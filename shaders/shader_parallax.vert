#version 430 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;
layout(location = 2) in vec3 vertexNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

struct PointLight {
	vec3 position;
	vec3 color;
	float intensity;
};

#define MAX_POINT_LIGHTS 16

uniform mat4 transformation;
uniform mat4 modelMatrix;
uniform vec3 cameraPos;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int LightsCount;
uniform mat3 normalMatrix;


out vec3 fragPos;
out vec2 vTexCoord;
out vec3 LightPosTS[MAX_POINT_LIGHTS];
out vec3 CameraPosTS;
out vec3 FragPosTS;

void main()
{
	gl_Position = transformation * vec4(vertexPosition, 1.0);

	vec3 T = normalize(normalMatrix * aTangent);
	vec3 N = normalize(normalMatrix * vertexNormal);
	// re-orthogonalize T with respect to N
	T = normalize(T - dot(T, N) * N);
	// then retrieve perpendicular vector B with the cross product of T and N
	vec3 B = cross(N, T);

	mat3 TBN = transpose(mat3(T, B, N));

	for(int i=0; i<LightsCount; i++)
		LightPosTS[i] = TBN * pointLights[i].position;

	fragPos = (modelMatrix*vec4(vertexPosition,1)).xyz;
    CameraPosTS  = TBN * cameraPos;
    FragPosTS  = TBN * fragPos;
	vTexCoord = vertexTexCoord;
}
