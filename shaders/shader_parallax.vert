#version 330 core
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;
layout(location = 2) in vec3 vertexNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 modelMatrix;
uniform mat4 transformation;


uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{
    gl_Position = transformation * vec4(vertexPosition, 1.0);
    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));

    gl_Position      = projection * view * modelMatrix * vec4(vertexPosition, 1.0);
    vs_out.FragPos = vec3(modelMatrix * vec4(vertexPosition, 1.0));   
    vs_out.TexCoords = vertexTexCoord;   
    
    vec3 T = normalize(normalMatrix * aTangent);
	vec3 N = normalize(normalMatrix * vertexNormal);
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);

	mat3 TBN = mat3(T, B, N);

    vs_out.TangentLightPos = TBN * lightPos;
    vs_out.TangentViewPos  = TBN * viewPos;
    vs_out.TangentFragPos  = (modelMatrix*vec4(vertexPosition,1)).xyz;
}