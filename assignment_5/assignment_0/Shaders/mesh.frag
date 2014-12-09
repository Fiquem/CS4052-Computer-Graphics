#version 400

in vec3 p; // positions
in vec2 t; // texures from vertex shader
in vec3 n; // normals

uniform sampler2D basic_texture;
uniform mat4 view;

out vec4 frag_colour;

void main () {
	// use the normals as an interesting colouration
	// whenever you get normals in the fragment shader you should
	// re-normalise them, as scaling and interpolation can bang them about a bit
	vec3 l_a = vec3 (0.2, 0.2, 0.2);
	vec3 k_a = vec3 (0.2, 0.2, 0.2);
	vec4 i_a = vec4 (l_a * k_a, 1.0);

	vec3 renorm = normalize (n);

	vec3 l_d = vec3 (0.3, 0.3, 0.3);
	vec3 k_d = vec3 (0.3, 0.3, 0.7);
	vec3 light_dir = (view * normalize (vec4 (1.0,1.0,1.0,0.0))).xyz;
	vec4 i_d = vec4 (l_d * k_d * max (0.0, dot (light_dir, renorm)), 1.0);

	vec3 l_s = vec3 (1.0, 1.0, 1.0);
	vec3 k_s = vec3 (1.0, 1.0, 1.0);
	vec3 r = normalize (reflect (-light_dir, renorm));
	vec3 v = normalize (-p);
	float spec_exp = 100;
	vec4 i_s = vec4 (l_s * k_s * max (0.0, pow (dot (v, r), spec_exp)), 1.0);

	vec4 texel = texture (basic_texture, t);
	frag_colour = vec4(texel+i_a+i_d+i_s);
}
