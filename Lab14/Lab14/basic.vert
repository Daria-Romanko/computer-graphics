#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);

    vs_out.FragPos = worldPos.xyz;
    vs_out.Normal  = normalize(normalMatrix * aNormal);
    vs_out.TexCoord = aTexCoord;

    gl_Position = projection * view * worldPos;
}
