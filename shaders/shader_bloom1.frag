#version 430 core

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform vec3 objectColor;
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
	float coef = pow(max(0,dot(normal,V)),2);
	vec4 textureColor = texture2D(colorTexture, -vTexCoord);
	vec3 texture = vec3(textureColor.x, textureColor.y, textureColor.z) * colorTex;
	FragColor = vec4(texture + texture * coef, 1.0);

	float brightness = dot(FragColor.rgb, vec3(0.2, 0.7, 0.07));
	if(brightness > 1.0)
        BrightColor = vec4(FragColor.rgb, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
