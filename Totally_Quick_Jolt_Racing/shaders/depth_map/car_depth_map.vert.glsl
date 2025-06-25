#version 330 core

layout (location = 0) in vec3 vertex_position;

// Uniforms
uniform mat4 view_light;
uniform mat4 projection_light;

void main()
{
    // La position est déjà en world space dans vbo_position
    vec4 world_pos = vec4(vertex_position, 1.0);
    gl_Position = projection_light * view_light * world_pos;
}