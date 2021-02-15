#version 430 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;
layout(location = 2) in vec3 vertexNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in mat4 aInstanceMatrix;
layout (location = 9) in mat3 normalMatrix;

struct PointLight {
	vec3 position;
	vec3 color;
	float intensity;
};

#define MAX_POINT_LIGHTS 16

uniform mat4 projection;
uniform mat4 view;
uniform vec3 cameraPos;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int LightsCount;

out vec3 fragPos;
out vec2 vTexCoord;
out vec3 LightPosTS[MAX_POINT_LIGHTS];
out vec3 CameraPosTS;
out vec3 FragPosTS;

void main()
{
	gl_Position = projection * view * aInstanceMatrix * vec4(vertexPosition, 1.0);

	//mat3 normalMatrix = transpose(inverse(mat3(aInstanceMatrix)));

	vec3 T = normalize(normalMatrix * aTangent);
	vec3 N = normalize(normalMatrix * vertexNormal);
	// re-orthogonalize T with respect to N
	T = normalize(T - dot(T, N) * N);
	// then retrieve perpendicular vector B with the cross product of T and N
	vec3 B = cross(N, T);

	mat3 TBN = mat3(T, B, N);

	for(int i=0; i<LightsCount; i++)
		LightPosTS[i] = TBN * pointLights[i].position;

    CameraPosTS  = TBN * cameraPos;
    FragPosTS  = TBN * fragPos;


	fragPos = (aInstanceMatrix*vec4(vertexPosition,1)).xyz;
	vTexCoord = vertexTexCoord;
}
