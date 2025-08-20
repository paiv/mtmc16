static const GLchar* _fragment_glsl =
    "#version 330 core\n"
    "in float glColor;\n"
    "out vec4 glFragColor;\n"
    "const mat4 palette = mat4(\n"
    "    42./255., 69./255., 59./255., 1.,\n"
    "    54./255., 93./255., 72./255., 1.,\n"
    "    87./255., 124./255., 68./255., 1.,\n"
    "    127./255., 134./255., 15./255., 1.\n"
    ");\n"
    "void main() {\n"
    "    glFragColor = palette[int(glColor)];\n"
    "}\n"
    ;

static const GLchar* _vertex_glsl =
    "#version 330 core\n"
    "layout(location = 0) in vec2 uPos;\n"
    "layout(location = 1) in int color;\n"
    "out float glColor;\n"
    "void main() {\n"
    "    glColor = color;\n"
    "    gl_Position = vec4(uPos, 0.0, 1.0);\n"
    "}\n"
    ;

