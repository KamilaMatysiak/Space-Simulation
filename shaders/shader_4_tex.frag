#version 430 core

uniform vec3 objectColor;
//uniform vec3 lightDir;
uniform vec3 lightPos;
uniform vec3 cameraPos;
uniform sampler2D colorTexture;

in vec3 interpNormal;
in vec3 fragPos;
in vec2 vTexCoord;

void main()
{
	vec3 lightDir = normalize(lightPos-fragPos);
	vec3 V = normalize(cameraPos-fragPos);
	vec3 normal = normalize(interpNormal);
	vec3 R = reflect(-normalize(lightDir),normal);
	
	float specular = pow(max(0,dot(R,V)),10);
	float diffuse = max(0,dot(normal,normalize(lightDir)));
	vec4 textureColor = texture2D(colorTexture, -vTexCoord);
	vec3 texture = vec3(textureColor.x, textureColor.y, textureColor.z);
	vec4 ambient = vec4(0.1, 0.1, 0.1, 1.0) * textureColor;
	gl_FragColor = vec4(mix(texture,texture*diffuse+vec3(1)*specular,0.9), 1.0) + ambient;
}
