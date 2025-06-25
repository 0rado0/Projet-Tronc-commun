#version 330 core
layout (location = 0) in vec3 vertex_position; // vertex position in local space (x,y,z)
layout (location = 4) in vec3 instance_color;  // instance color (not used for depth map, but keep for consistency)
layout (location = 5) in vec3 instance_offset; // instance offset (x,y,z)
layout(location = 6) in vec3 instance_angle; // x,y = unused, z = angle

// Uniform variables expected to receive from the C++ program
uniform mat4 model; // Model affine transform matrix associated to the current shape
uniform mat4 view_light;  // View matrix (rigid transform) of the camera
uniform mat4 projection_light; // Projection (perspective or orthogonal) matrix of the camera
uniform vec3 light_position;
uniform float time; // Add time uniform for wind calculation if needed

// Generate a 3x3 rotation matrix around the z-axis (same as in your instancing shader)
mat3 rotation_z(in float angle) {
    return mat3(vec3(cos(angle), -sin(angle), 0.0),
        vec3(sin(angle), cos(angle), 0.0),
        vec3(0.0, 0.0, 1.0));
}

// Random noise function (same as in your instancing shader)
float PHI = 1.61803398874989484820459;
float gold_noise(in vec2 xy, in float seed) {return fract(tan(distance(xy * PHI, xy) * seed) * xy.x);}
float rand(in float n) { return gold_noise(vec2(n,n+0.5), 1.0); }

// Wind function (same as in your instancing shader)
vec3 wind(in vec3 p, in float t) {
    float influence = 0.00;
    float limit_wind = 0.01;
    if (p.z/40 > limit_wind) {
        influence = smoothstep(0.00, (1.00-limit_wind), p.y-limit_wind);
    }
    float radius = sqrt((p.x-0.5)*(p.x-0.5)+(2.0*p.y/3 - 0.5)*(2.0*p.y/3 - 0.5));
    float sway_x = 0.04 * cos(t - p.x * 0.5) * influence*radius; 
    float sway_y = 0.04 * sin(0.7 * t - 1.5 * p.y) * influence*radius;
    
    return vec3(sway_x, sway_y, 0.0);
}

void main()
{
    mat3 rotation_instance = rotation_z(instance_angle.z);
    
    // Initialize the offset (same as in your instancing shader)
    vec3 offset = instance_offset;
    
    // The position of the vertex in the world space
    vec4 position = model * vec4(rotation_instance*vertex_position, 1.0);
    position.xyz = position.xyz + offset; // Add instance offset
    position.xyz = position.xyz + wind(vertex_position, time); // Add wind effect
    
    // Calculate final position for depth map
    gl_Position = projection_light * view_light * position;
}