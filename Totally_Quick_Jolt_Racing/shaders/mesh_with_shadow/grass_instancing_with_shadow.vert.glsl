#version 330 core

// Vertex shader - this code is executed for every vertex of the shape

// Inputs coming from VBOs
layout (location = 0) in vec3 vertex_position; // vertex position in local space (x,y,z)
layout (location = 1) in vec3 vertex_normal;   // vertex normal in local space   (nx,ny,nz)
layout (location = 2) in vec3 vertex_color;    // vertex color      (r,g,b)
layout (location = 3) in vec2 vertex_uv;       // vertex uv-texture (u,v)
layout(location = 4) in vec3 instance_color; // instancing color
layout(location = 5) in vec3 instance_offset; // instancing position
layout(location = 6) in vec3 instance_angle; // x,y = unused, z = angle


// Output variables sent to the fragment shader
out struct fragment_data
{
    vec3 position; // vertex position in world space
    vec3 normal;   // normal position in world space
    vec3 color;    // vertex color
    vec2 uv;       // vertex uv
    vec4 position_lightspace; // the vertex position in the light space
} fragment;
// Uniform variables expected to receive from the C++ program
uniform mat4 model; // Model affine transform matrix associated to the current shape
uniform mat4 view;  // View matrix (rigid transform) of the camera
uniform mat4 projection; // Projection (perspective or orthogonal) matrix of the camera
uniform mat4 view_light; // View matrix from the light
uniform mat4 projection_light; // Projection matrix of the light
uniform float time;

// Generate a 3x3 rotation matrix around the z-axis
mat3 rotation_z(in float angle) {
	return mat3(vec3(cos(angle), -sin(angle), 0.0),
		vec3(sin(angle), cos(angle), 0.0),
		vec3(0.0, 0.0, 1.0));
}


// Random noise - https://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
float PHI = 1.61803398874989484820459;
float gold_noise(in vec2 xy, in float seed) {return fract(tan(distance(xy * PHI, xy) * seed) * xy.x);}
float rand(in float n) { return gold_noise(vec2(n,n+0.5), 1.0); }

// Procedural wind-like deformation
vec3 wind(in vec3 p,in float t) {
	return vec3(0.2*cos(t-p.x), 0.1*sin(0.5*t - 1.2*p.y), 0.0);
}

void main()
{
    mat3 rotation_instance = rotation_z(instance_angle.z);

	// The position of the vertex in the world space
	vec4 position = model * vec4(rotation_instance*vertex_position, 1.0);

	position.xyz = position.xyz + instance_offset; // offset of the instance
	position.xyz = position.xyz + position.z * wind(position.xyz, time); // procedural deformation modeling the wind effect (we scale the deformation along z such that only the tips of the blades are moving while the root remains fixed).
    
    // The normal of the vertex in the world space
    mat4 modelNormal = transpose(inverse(model));
    vec4 normal = modelNormal * vec4(vertex_normal, 0.0);
    
    // The projected position of the vertex in the normalized device coordinates:
    vec4 position_projected = projection * view * position;
    
    // Fill the parameters sent to the fragment shader
    fragment.position = position.xyz;
    fragment.normal = normal.xyz;
    
    fragment.color = vertex_color * instance_color;
    
    fragment.uv = vertex_uv;
    
    // compute the coordinates viewed from the light
    fragment.position_lightspace = projection_light * view_light * position;
    
    // gl_Position is a built-in variable which is the expected output of the vertex shader
    gl_Position = position_projected;
}