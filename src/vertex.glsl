#version 330 core
layout(location = 0) in vec2 uPos;
layout(location = 1) in int color;

out float glColor;

void main() {
    glColor = color;
    gl_Position = vec4(uPos, 0.0, 1.0);
}
