layout (location = 0) in vec3 position;
layout (location = 2) in vec2 texCoord;

out vec3 color;
out vec2 TexCoord;

uniform vec3 offset;
uniform mat4 rotation;

void main()
{
    gl_Position = rotation * vec4(position + offset, 1.0);
    TexCoord = texCoord;
    color = position + vec3(0.5);
}