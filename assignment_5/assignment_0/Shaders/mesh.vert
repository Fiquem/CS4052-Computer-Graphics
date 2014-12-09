#version 400

layout (location=0) in vec3 vp; // points
layout (location=1) in vec2 vt; // textures
layout (location=2) in vec3 vn; // normals

uniform mat4 PVM; // PVM matrix
uniform mat4 model;
uniform mat4 view;

out vec3 p;
out vec2 t;
out vec3 n;

void main () {
	// send normals to fragment shader
	t = vt;
	n = (view * model * vec4 (vn, 0.0)).xyz;
	p = (view * model * vec4 (vp, 1.0)).xyz;
	gl_Position = PVM * vec4 (vp, 1.0);
}
