#version 330 core
out vec4 FragColor;

in vec2 vTexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform vec2 screenSize;

float FXAA_SPAN_MAX = 100.0f;
//sila dzialania
float FXAA_REDUCE_MUL = 1.0f/8.0f;

//minimalna sila dzialania
float FXAA_REDUCE_MIN = 1.0f/128.0f;

vec3 fastAA()
{
    vec2 screenTextureOffset = screenSize;
    vec3 luma = vec3(0.299f, 0.587f, 0.114f);

    vec3 offsetNW = texture(scene, vTexCoords.xy + (vec2(-1.0f, -1.0f) * screenTextureOffset)).xyz;
    vec3 offsetNE = texture(scene, vTexCoords.xy + (vec2(1.0f, -1.0f) * screenTextureOffset)).xyz;
    vec3 offsetSW = texture(scene, vTexCoords.xy + (vec2(-1.0f, 1.0f) * screenTextureOffset)).xyz;
    vec3 offsetSE = texture(scene, vTexCoords.xy + (vec2(1.0f, 1.0f) * screenTextureOffset)).xyz;
    vec3 offsetM  = texture(scene, vTexCoords.xy).xyz;

    float lumaNW = dot(luma, offsetNW);
    float lumaNE = dot(luma, offsetNE);
    float lumaSW = dot(luma, offsetSW);
    float lumaSE = dot(luma, offsetSE);
    float lumaM  = dot(luma, offsetNW);

    vec2 dir = vec2(-((lumaNW + lumaNE) - (lumaSW + lumaSE)),
                     ((lumaNW + lumaSW) - (lumaNE + lumaSE)));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (FXAA_REDUCE_MUL * 0.25f), FXAA_REDUCE_MIN);
    float dirCorrection = 1.0f / (min(abs(dir.x), abs(dir.y)) + dirReduce);

    dir = min(vec2(FXAA_SPAN_MAX), max(vec2(-FXAA_SPAN_MAX), dir * dirCorrection)) * screenTextureOffset;

    vec3 resultA = 0.5f * (texture(scene, vTexCoords.xy + (dir * vec2(1.0f / 3.0f - 0.5f))).xyz +
                                    texture(scene, vTexCoords.xy + (dir * vec2(2.0f / 3.0f - 0.5f))).xyz);

    vec3 resultB = resultA * 0.5f + 0.25f * (texture(scene, vTexCoords.xy + (dir * vec2(0.0f / 3.0f - 0.5f))).xyz +
                                             texture(scene, vTexCoords.xy + (dir * vec2(3.0f / 3.0f - 0.5f))).xyz);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    float lumaResultB = dot(luma, resultB);

    if(lumaResultB < lumaMin || lumaResultB > lumaMax)
        return vec3(resultA);
    else
        return vec3(resultB);
}

void main()
{             
    const float gamma = 0.9f;
    vec3 hdrColor = fastAA(); //texture(scene, vTexCoords).rgb;      
    vec3 bloomColor = texture(bloomBlur, vTexCoords).rgb;
    hdrColor += bloomColor;
    vec3 result = vec3(1.0) - exp(-hdrColor * 1.0f);    
    result = pow(result, vec3(1.0 / gamma));
    FragColor = vec4(result, 1.0);
}