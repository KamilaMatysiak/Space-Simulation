#version 330 core
layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec2 vertexTexCoords;

out vec3 fragPos;
out vec3 interpNormal;
out vec2 vTexCoords;

uniform mat4 transformation;
uniform mat4 matrixModel;

void main()
{
    fragPos = vec3(matrixModel * vec4(vertexPosition, 1.0));   
    vTexCoords = vertexTexCoords;
        
    mat3 normalMatrix = transpose(inverse(mat3(matrixModel)));
    interpNormal = normalize(normalMatrix * vertexNormal);
    
    gl_Position = transformation * matrixModel * vec4(vertexPosition, 1.0);
}