#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 ourColor;

uniform vec3 offset;
uniform mat4 rotation;
uniform vec3 scale;

void main() {
    vec3 scaledPos = aPos * scale;
    gl_Position = rotation * vec4(scaledPos + offset, 1.0);
    ourColor = aColor;
}