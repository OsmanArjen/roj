#version 400
layout (location = 0) out vec4 gLightmap;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform samplerCube shadowMap;
uniform float far_plane;
struct Light {
    vec3 Position;
    vec3 Color;
    
    float radius;
};

uniform Light light;
uniform vec3 viewPos;

vec3 gridSamplingDisk[20] = vec3[]
(
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1), 
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);

float ShadowCalculation(vec3 fragPos, vec3 lightPos, vec3 normal, vec3 lightDir)
{
 
    vec3 fragToLight = fragPos - lightPos;

    float currentDepth = length(fragToLight);

    float shadow = 0.0;
    float bias = 20;
    int samples = 20;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 25.0;
    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(shadowMap, fragToLight + gridSamplingDisk[i] * diskRadius).r;
        closestDepth *= far_plane;   // undo mapping [0;1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);
        
        
    return shadow;
}

#define PI 3.14
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;

    float num = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

float DistributionGGX(vec3 N, vec3 H, float a)
{
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float CalculateSpecular(vec3 worldPos, vec3 normal, vec3 lightDir, float roughness)
{
    vec3 vDir = normalize(viewPos - worldPos);
    lightDir = normalize(lightDir);
    vec3 halfwayDir = normalize(vDir + lightDir);

    float NdotH = max(dot(normal, halfwayDir), 0.0);
    float NdotV = max(dot(normal, vDir), 0.0);
    float NdotL = max(dot(normal, lightDir), 0.0);

    // GGX Normal Distribution Function (NDF)
    float roughnessSq = roughness * roughness;
    float D = DistributionGGX(normal, halfwayDir, roughnessSq);

    // Geometry function using Smith's method
    float G = GeometrySmith(normal, vDir, lightDir, roughnessSq);

    // Specular BRDF
    float specular = (D * G) / (4.0 * NdotV * NdotL + 0.001);

    return specular * 0.2;
}

void main()
{             

    vec3 FragPos = texture(gPosition, TexCoords).rgb + viewPos;
    vec3 Normal = (texture(gNormal, TexCoords).rgb * 2) - 1;
    
    vec3 viewDir  = normalize(viewPos - FragPos);
    vec3 lightDir = normalize(light.Position - FragPos);
    vec3 lightDiffuse = max(dot(Normal, lightDir), 0.0) * light.Color;

    float spec = CalculateSpecular(FragPos, Normal, lightDir, 0.5);
    vec3 specular = light.Color * spec;

    float distance = length(light.Position - FragPos);
    float attenuation = distance / (light.radius);
    attenuation *= attenuation;
    attenuation = 1-attenuation;
    lightDiffuse *= attenuation;
    specular *= attenuation;     
    
    float shadow = ShadowCalculation(FragPos, light.Position, Normal, lightDir);
    gLightmap.rgb = max((1-shadow) * (lightDiffuse + specular), 0);
    gLightmap.a = 1.0;
}