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
uniform sampler2D depthTexture;

uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int LightsCount;

in vec3 fragPos;
in vec2 vTexCoord;
in vec3 LightPosTS[MAX_POINT_LIGHTS];
in vec3 CameraPosTS;
in vec3 FragPosTS;

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
    // number of depth layers
    const float minLayers = 8;
    const float maxLayers = 32;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));  
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * 0.00001; 
    vec2 deltaTexCoords = P / numLayers;
  
    // get initial values
    vec2  currentTexCoords     = texCoords;
    float currentdepthTextureValue = texture(depthTexture, currentTexCoords).r;
      
    while(currentLayerDepth < currentdepthTextureValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthTexture value at current texture coordinates
        currentdepthTextureValue = texture(depthTexture, currentTexCoords).r;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentdepthTextureValue - currentLayerDepth;
    float beforeDepth = texture(depthTexture, prevTexCoords).r - currentLayerDepth + layerDepth;
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}

void main()
{
	vec3 fragColor = vec3(0,0,0);
	vec3 V = normalize(CameraPosTS-FragPosTS);
	vec2 texCoords = ParallaxMapping(vTexCoord, V);
	if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
        discard;

	vec3 texture = texture2D(diffuseTexture, texCoords).rgb;
	//vec3 texture = vec3(textureColor.x, textureColor.y, textureColor.z);
	vec3 ambient = vec3(0.1, 0.1, 0.1) * texture;

    vec3 normal = texture2D(normalTexture, vTexCoord).rgb;
    normal = normalize(normal * 2.0 - 1.0);
	for(int i = 0; i < LightsCount; i++)
	{
		vec3 lightDir = normalize(LightPosTS[i] - FragPosTS);
	

		vec3 R = reflect(-lightDir,normal);

		float dist = distance(fragPos, pointLights[i].position);
		float distance = (1/dist) * (1/dist);
	
		float spec = pow(max(0,dot(R,V)),2);
		float diff = max(0,dot(normal,normalize(lightDir)));

		vec3 diffuse = pointLights[i].color * diff * distance * pointLights[i].intensity;
		vec3 specular = spec * pointLights[i].color * (pointLights[i].intensity/dist);


		fragColor += texture*(diffuse+specular);
	}

    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
	FragColor = vec4(fragColor+ambient,1.0);
}
