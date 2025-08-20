#version 330 core
in float glColor;
out vec4 glFragColor;

const mat4 palette = mat4(
    42./255., 69./255., 59./255., 1.,
    54./255., 93./255., 72./255., 1.,
    87./255., 124./255., 68./255., 1.,
    127./255., 134./255., 15./255., 1.
);

void main() {
    glFragColor = palette[int(glColor)];
}
