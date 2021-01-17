#version 430 core

uniform vec3 objectColor;
//uniform vec3 lightDir;
uniform vec3 lightPos;
uniform vec3 cameraPos;
uniform sampler2D colorTexture;
uniform vec3 colorTex;
in vec3 interpNormal;
in vec3 fragPos;
in vec2 vTexCoord;

void main()
{
	vec3 normal = normalize(interpNormal);
	vec3 V = normalize(cameraPos-fragPos);
	float coef = pow(max(0,dot(normal,V)),3);
	vec4 textureColor = texture2D(colorTexture, -vTexCoord);
	vec3 texture = vec3(textureColor.x, textureColor.y, textureColor.z) * colorTex;
	gl_FragColor = vec4(texture + texture * coef, 1.0);
}
