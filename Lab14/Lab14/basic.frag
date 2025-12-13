#version 330 core

out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
} fs_in;

struct Material {
    sampler2D diffuse;
    vec3      specular;
    float     shininess;
};

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    vec3 attenuation;
};

struct SpotLight {
    vec3 position;
    vec3 direction;

    float innerCutOff;
    float outerCutOff;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    vec3 attenuation;
};

#define MAX_POINT_LIGHTS 8
#define MAX_SPOT_LIGHTS  8

uniform Material material;
uniform DirLight dirLight;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform SpotLight  spotLights[MAX_SPOT_LIGHTS];
uniform int numPointLights;
uniform int numSpotLights;

uniform vec3 viewPos;

uniform bool u_useToonShading = false;
uniform int u_toonLevels = 4;
uniform float u_toonSpecularSize = 0.1;
uniform float u_toonEdgeThreshold = 0.2;
uniform vec3 u_outlineColor = vec3(0.0, 0.0, 0.0);
uniform float u_outlineWidth = 1.0;

float toonify(float value, int levels) {
    return floor(value * levels) / levels;
}

vec3 toonify(vec3 color, int levels) {
    return floor(color * levels) / levels;
}

float calculateEdge(float threshold, vec3 normal, vec3 viewDir) {
    float edge = dot(normal, viewDir);
    edge = abs(edge);
    return edge < threshold ? 0.0 : 1.0;
}

float getOutlineFactor(vec3 normal, vec3 viewDir) {
    float edge = dot(normal, viewDir);
    edge = abs(edge);
    return smoothstep(u_toonEdgeThreshold - 0.05, u_toonEdgeThreshold + 0.05, edge);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 albedo)
{
    vec3 lightDir = normalize(-light.direction);

    float diff = max(dot(normal, lightDir), 0.0);
    

    if (u_useToonShading) {
        diff = toonify(diff, u_toonLevels);
    }

    vec3 reflectDir = reflect(-lightDir, normal);
    float specAngle = max(dot(viewDir, reflectDir), 0.0);
    float spec = pow(specAngle, material.shininess);
    
    if (u_useToonShading) {
        spec = spec > u_toonSpecularSize ? 1.0 : 0.0;
    }

    vec3 ambient  = light.ambient  * albedo;
    vec3 diffuse  = light.diffuse  * diff * albedo;
    vec3 specular = light.specular * spec * material.specular;

    return ambient + diffuse + specular;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 viewDir, vec3 fragPos, vec3 albedo)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    
    if (u_useToonShading) {
        diff = toonify(diff, u_toonLevels);
    }

    vec3 reflectDir = reflect(-lightDir, normal);
    float specAngle = max(dot(viewDir, reflectDir), 0.0);
    float spec = pow(specAngle, material.shininess);
    

    if (u_useToonShading) {
        spec = spec > u_toonSpecularSize ? 1.0 : 0.0;
    }

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.attenuation.x + light.attenuation.y * distance + light.attenuation.z * distance * distance);

    vec3 ambient = light.ambient  * albedo;
    vec3 diffuse = light.diffuse  * diff * albedo;
    vec3 specular = light.specular * spec * material.specular;

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 viewDir, vec3 fragPos, vec3 albedo)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    

    if (u_useToonShading) {
        diff = toonify(diff, u_toonLevels);
    }

    vec3 reflectDir = reflect(-lightDir, normal);
    float specAngle = max(dot(viewDir, reflectDir), 0.0);
    float spec = pow(specAngle, material.shininess);
    

    if (u_useToonShading) {
        spec = spec > u_toonSpecularSize ? 1.0 : 0.0;
    }

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.attenuation.x + light.attenuation.y * distance + light.attenuation.z * distance * distance);

    float theta   = dot(normalize(-lightDir), normalize(light.direction));
    float epsilon = light.innerCutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    vec3 ambient  = light.ambient  * albedo;
    vec3 diffuse  = light.diffuse  * diff * albedo;
    vec3 specular = light.specular * spec * material.specular;

    ambient  *= attenuation * intensity;
    diffuse  *= attenuation * intensity;
    specular *= attenuation * intensity;

    return ambient + diffuse + specular;
}

void main()
{
    vec3 albedo = texture(material.diffuse, fs_in.TexCoord).rgb;
    vec3 normal = normalize(fs_in.Normal);
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);

    vec3 result = vec3(0.0);

    result += CalcDirLight(dirLight, normal, viewDir, albedo);

    for (int i = 0; i < numPointLights; ++i) {
        result += CalcPointLight(pointLights[i], normal, viewDir, fs_in.FragPos, albedo);
    }

    for (int i = 0; i < numSpotLights; ++i) {
        result += CalcSpotLight(spotLights[i], normal, viewDir, fs_in.FragPos, albedo);
    }
    
    if (u_useToonShading) {
        float edgeFactor = getOutlineFactor(normal, viewDir);
        
        if (edgeFactor < 0.5) {
            result = u_outlineColor;
        } else {
            result = toonify(result, u_toonLevels);
            result = pow(result, vec3(1.2));
        }
    }

    FragColor = vec4(result, 1.0);
}
