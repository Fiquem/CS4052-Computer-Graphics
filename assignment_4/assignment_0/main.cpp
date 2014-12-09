#include "maths_funcs.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <obj_parser.h>
#include <obj_parser.cpp>


//
// dimensions of the window drawing surface
int gl_width = 800;
int gl_height = 800;

float cam_speed = 5.0f; // 1 unit per second
float cam_yaw_speed = 100.0f; // 10 degrees per second

float cam_pos[] = {0.0f, 0.0f, 0.0f}; // don't start at zero, or we will be too close
//float cam_yaw = 0.0f; // y-rotation in degrees
//float cam_pitch = 0.0f; // x-rotation in degrees
//float cam_roll = 0.0f; // z-rotation in degrees

// Free camera thing
versor cam_orient = versor (0.0,0.0,1.0,0.0);
vec3 right = vec3 (1,0,0);
vec3 up = vec3 (0,1,0);
vec3 forward = vec3 (0,0,-1);

//
// copy a shader from a plain text file into a character array
bool parse_file_into_str (
	const char* file_name, char* shader_str, int max_len
) {
	FILE* file = fopen (file_name , "r");
	int current_len = 0;
	char line[2048];

	shader_str[0] = '\0'; /* reset string */
	if (!file) {
		fprintf (stderr, "ERROR: opening file for reading: %s\n", file_name);
		return false;
	}
	strcpy (line, ""); /* remember to clean up before using for first time! */
	while (!feof (file)) {
		if (NULL != fgets (line, 2048, file)) {
			current_len += strlen (line); /* +1 for \n at end */
			if (current_len >= max_len) {
				fprintf (stderr, 
					"ERROR: shader length is longer than string buffer length %i\n",
					max_len
				);
			}
			strcat (shader_str, line);
		}
	}
	if (EOF == fclose (file)) { /* probably unnecesssary validation */
		fprintf (stderr, "ERROR: closing file from reading %s\n", file_name);
		return false;
	}
	return true;
}



// methods for making cleans yes
void initialise_texture(GLuint* id, int x, int y, unsigned char* data) {
	glGenTextures(1, id);
	glActiveTexture(*id);
	glBindTexture(GL_TEXTURE_2D,*id);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,x,y,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void initialise_vao(GLuint* vao, int* point_count, const char* file) {
		GLuint points_vbo, texture_vbo, normals_vbo;
		GLfloat* vp = NULL; // array of vertex points
		GLfloat* vn = NULL; // array of vertex normals (not needed for assignment)
		GLfloat* vt = NULL; // array of texture coordinates

		if (!load_obj_file (file, vp, vt, vn, *point_count)) {
			fprintf (stderr, "ERROR: could not find mesh file...\n");
			// do something
		}

		// create a vertex points buffer
		glGenBuffers (1, &points_vbo);
		glBindBuffer (GL_ARRAY_BUFFER, points_vbo);
		// note: this is dynamically allocated memory, so we can't say "sizeof (vp)"
		glBufferData (GL_ARRAY_BUFFER, sizeof (GLfloat) * 3 * (*point_count), vp,
		GL_STATIC_DRAW);

		//for(int i = 0; i < 3*(*point_count); i+=3)
		//	printf("%fl, %fl, %fl\n", vp[i],vp[i+1],vp[i+1]);

		// -----==create a texture coordinates vertex buffer object here==-----
		glGenBuffers (1, &texture_vbo);
		glBindBuffer (GL_ARRAY_BUFFER, texture_vbo);
		glBufferData (GL_ARRAY_BUFFER, sizeof (GLfloat) * 2 * (*point_count),
			vt, GL_STATIC_DRAW);

		glGenBuffers (1, &normals_vbo);
		glBindBuffer (GL_ARRAY_BUFFER, normals_vbo);
		glBufferData (GL_ARRAY_BUFFER, sizeof (GLfloat) * 3 * (*point_count),
			vn, GL_STATIC_DRAW);

		// free allocated memory
		free (vp);
		free (vn);
		free (vt);
	
		glGenVertexArrays (1, vao);
		glBindVertexArray (*vao);
		glEnableVertexAttribArray (0);
		glBindBuffer (GL_ARRAY_BUFFER, points_vbo);
		glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray (1);
		glBindBuffer (GL_ARRAY_BUFFER, texture_vbo);
		glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray (2);
		glBindBuffer (GL_ARRAY_BUFFER, normals_vbo);
		glVertexAttribPointer (2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}



int main () {
	GLFWwindow* window = NULL;
	const GLubyte* renderer;
	const GLubyte* version;

	GLuint shader_programme;
	GLuint vao, vao1, vao2, vao3, vao4;
	GLuint texture0, texture1, texture2, texture3, texture4;
	int point_count, point_count1, point_count2, point_count3 = 0;

	//
	// Start OpenGL using helper libraries
	// --------------------------------------------------------------------------
	if (!glfwInit ()) {
		fprintf (stderr, "ERROR: could not start GLFW3\n");
		return 1;
	} 

	/* change to 3.2 if on Apple OS X
	glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); */

	window = glfwCreateWindow (gl_width, gl_height, "Hello Texture", NULL, NULL);
	if (!window) {
		fprintf (stderr, "ERROR: opening OS window\n");
		return 1;
	}
	glfwMakeContextCurrent (window);

	glewExperimental = GL_TRUE;
	glewInit ();

	/* get version info */
	renderer = glGetString (GL_RENDERER); /* get renderer string */
	version = glGetString (GL_VERSION); /* version as a string */
	printf ("Renderer: %s\n", renderer);
	printf ("OpenGL version supported %s\n", version);

	// TEXTURES
    int x = 1024;
	int y = 1024;
	int n = 4;
    //unsigned char *environ = stbi_load("environ.PNG", &x, &y, &n, 0);
    unsigned char *world = stbi_load("world2.PNG", &x, &y, &n, 0);
    unsigned char *star = stbi_load("star.PNG", &x, &y, &n, 0);
    //unsigned char *cow = stbi_load("COWTEXTURE.PNG", &x, &y, &n, 0);
    unsigned char *space = stbi_load("space.PNG", &x, &y, &n, 0);
    unsigned char *ship = stbi_load("ship.PNG", &x, &y, &n, 0);
    unsigned char *man = stbi_load("spaceman.PNG", &x, &y, &n, 0);
	
	//initialise_texture(&texture0, x, y, environ);
	initialise_texture(&texture0, x, y, space);
	initialise_texture(&texture1, x, y, world);
	initialise_texture(&texture2, x, y, star);
	//initialise_texture(&texture3, x, y, cow);
	initialise_texture(&texture3, x, y, ship);
	initialise_texture(&texture4, x, y, man);

	//
	// Set up vertex buffers and vertex array object
	// --------------------------------------------------------------------------
	{
		initialise_vao(&vao, &point_count, "environ.obj");
		initialise_vao(&vao1, &point_count1, "world2.obj");
		initialise_vao(&vao2, &point_count2, "star.obj");
		initialise_vao(&vao3, &point_count3, "ship.obj");
		initialise_vao(&vao4, &point_count3, "spaceman.obj");
		//initialise_vao(&vao3, &point_count3, "SPACECOW.obj");
	}
	//
	// Load shaders from files
	// --------------------------------------------------------------------------
	{
		char* vertex_shader_str;
		char* fragment_shader_str;
		
		// allocate some memory to store shader strings
		vertex_shader_str = (char*)malloc (81920);
		fragment_shader_str = (char*)malloc (81920);
		// load shader strings from text files
		assert (parse_file_into_str ("Shaders/mesh.vert", vertex_shader_str, 81920));
		assert (parse_file_into_str ("Shaders/mesh.frag", fragment_shader_str, 81920));
		GLuint vs, fs;
		vs = glCreateShader (GL_VERTEX_SHADER);
		fs = glCreateShader (GL_FRAGMENT_SHADER);
		glShaderSource (vs, 1, (const char**)&vertex_shader_str, NULL);
		glShaderSource (fs, 1, (const char**)&fragment_shader_str, NULL);
		// free memory
		free (vertex_shader_str);
		free (fragment_shader_str);
		glCompileShader (vs);
		glCompileShader (fs);
		shader_programme = glCreateProgram ();
		glAttachShader (shader_programme, fs);
		glAttachShader (shader_programme, vs);
		glLinkProgram (shader_programme);
		/* TODO NOTE: you should check for errors and print logs after compiling and also linking shaders */
	}
	
	//
	// Create some matrices
	// --------------------------------------------------------------------------
	// a model matrix
	// can you see what affine transformation this does? y Anton bb I get dis
	
	// Setting up transformation matrices
	mat4 t = identity_mat4();
	mat4 r = identity_mat4();
	mat4 s = identity_mat4();
	mat4 axis = inverse(quat_to_mat4(cam_orient));

	mat4 view_mat = quat_to_mat4 (cam_orient) * translate (identity_mat4(), vec3 (-cam_pos[0], -cam_pos[1], -cam_pos[2])); // QUAT CODE
	mat4 pers_mat = perspective(90/(gl_width/gl_height),gl_width/gl_height,0.1,10000);

	// world
	float env_theta = 0.0;
	mat4 env_mat = pers_mat*view_mat;

	// star
	float star_theta[] = { 0.0, 0.0, 0.0, 0.0};
	
	// ship
	float ship_theta = 0.0;
	float ship_y_rot = 0.0;

	// location of "M" in vertex shader
	int PVM_loc = glGetUniformLocation (shader_programme, "PVM");
	assert (PVM_loc > -1);
	//int cam_loc = glGetUniformLocation (shader_programme, "cam_pos");
	int model_loc = glGetUniformLocation (shader_programme, "model");
	int view_loc = glGetUniformLocation (shader_programme, "view");

	// send matrix values to shader immediately
	glUseProgram (shader_programme);
	glUniformMatrix4fv (PVM_loc, 1, GL_FALSE, identity_mat4().m);
	//glUniformMatrix4fv (cam_loc, 1, GL_FALSE, cam_pos);
	glUniformMatrix4fv (model_loc, 1, GL_FALSE, identity_mat4().m);
	glUniformMatrix4fv (view_loc, 1, GL_FALSE, view_mat.m);
	
	//
	// Start rendering
	// --------------------------------------------------------------------------
	// tell GL to only draw onto a pixel if the fragment is closer to the viewer
	glEnable (GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc (GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor (0.5, 0.5, 0.5, 1.0);
	//glDisable(GL_CULL_FACE);

	while (!glfwWindowShouldClose (window)) {

		// add a timer for doing animation
		static double previous_seconds = glfwGetTime ();
		double current_seconds = glfwGetTime ();
		double elapsed_seconds = current_seconds - previous_seconds;
		previous_seconds = current_seconds;

		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// just the default viewport, covering the whole render area
		glViewport (0, 0, gl_width, gl_height);

		glUseProgram (shader_programme);


		// spinny spinny worldy worldy
		env_theta += 0.1;
		r = rotate_y_deg (identity_mat4 (), env_theta);
		s = scale (identity_mat4().m, vec3 (4,4,4));
		glBindVertexArray (vao);
		glBindTexture(GL_TEXTURE_2D,texture0);
		glUniformMatrix4fv (PVM_loc, 1, GL_FALSE, (env_mat*r*s).m);
		glUniformMatrix4fv (model_loc, 1, GL_FALSE, (r*s).m);
		glDrawArrays (GL_TRIANGLES, 0, point_count);

		// ground
		glBindVertexArray (vao1);
		glBindTexture(GL_TEXTURE_2D,texture1);
		glUniformMatrix4fv (PVM_loc, 1, GL_FALSE, env_mat.m);
		glUniformMatrix4fv (model_loc, 1, GL_FALSE, identity_mat4().m);
		glDrawArrays (GL_TRIANGLES, 0, point_count1);
		
		// ship
		ship_theta += 0.001;
		ship_y_rot += 0.001*57.2957795;
		t = translate (identity_mat4 (), vec3 (2000*sin(ship_theta), 0.0, -2000*cos(ship_theta)));
		s = scale (identity_mat4().m, vec3 (20,20,20));
		r = rotate_z_deg (identity_mat4(), 90);
		r = rotate_y_deg (r, 90);
		r = rotate_y_deg (r, -ship_y_rot);
		glBindVertexArray (vao3);
		glBindTexture(GL_TEXTURE_2D,texture3);
		glUniformMatrix4fv (PVM_loc, 1, GL_FALSE, (env_mat*t*r*s).m);
		glUniformMatrix4fv (model_loc, 1, GL_FALSE, (t*r*s).m);
		glDrawArrays (GL_TRIANGLES, 0, point_count3);
		// man
		glBindVertexArray (vao4);
		glBindTexture(GL_TEXTURE_2D,texture4);
		glUniformMatrix4fv (PVM_loc, 1, GL_FALSE, (env_mat*t*r*s*(translate (identity_mat4 (), vec3 (10*sin(5*ship_theta), -10*cos(5*ship_theta), 0.0)))*(scale (identity_mat4 (), vec3 (0.5,0.5,0.5)))).m);
		glUniformMatrix4fv (model_loc, 1, GL_FALSE, (t*r*s*(translate (identity_mat4 (), vec3 (10*sin(5*ship_theta), -10*cos(5*ship_theta), 0.0)))*(scale (identity_mat4 (), vec3 (0.5,0.5,0.5)))).m);
		glDrawArrays (GL_TRIANGLES, 0, point_count3);


		// spinny spinny starry starry
		star_theta[0] += 0.005;
		star_theta[1] += 0.02;
		star_theta[2] += 0.001;
		star_theta[3] += 0.05;
		glBindVertexArray (vao2);
		glBindTexture(GL_TEXTURE_2D,texture2);
		t = translate (identity_mat4 (), vec3 (20+1380*sin(star_theta[0]), 450.0, -1380*cos(star_theta[0])));
		glUniformMatrix4fv (PVM_loc, 1, GL_FALSE, (pers_mat*view_mat*t).m);
		glUniformMatrix4fv (model_loc, 1, GL_FALSE, t.m);
		glDrawArrays (GL_TRIANGLES, 0, point_count2);
		t = translate (identity_mat4 (), vec3 (100+1500*sin(star_theta[1]), -1350.0, -1500*cos(star_theta[1])));
		glUniformMatrix4fv (PVM_loc, 1, GL_FALSE, (pers_mat*view_mat*t).m);
		glUniformMatrix4fv (model_loc, 1, GL_FALSE, t.m);
		glDrawArrays (GL_TRIANGLES, 0, point_count2);
		t = translate (identity_mat4 (), vec3 (-100+1500*sin(star_theta[2]), 370.0, -1500*cos(star_theta[2])));
		glUniformMatrix4fv (PVM_loc, 1, GL_FALSE, (pers_mat*view_mat*t).m);
		glUniformMatrix4fv (model_loc, 1, GL_FALSE, t.m);
		glDrawArrays (GL_TRIANGLES, 0, point_count2);
		t = translate (identity_mat4 (), vec3 (200+1500*sin(star_theta[3]), 1500.0, 50-1500*cos(star_theta[3])));
		glUniformMatrix4fv (PVM_loc, 1, GL_FALSE, (pers_mat*view_mat*t).m);
		glUniformMatrix4fv (model_loc, 1, GL_FALSE, t.m);
		glDrawArrays (GL_TRIANGLES, 0, point_count2);

		/* this just updates window events and keyboard input events (not used yet) */
		glfwPollEvents ();
		glfwSwapBuffers (window);

		// close window
        if (glfwGetKey (window, GLFW_KEY_ESCAPE))
            glfwSetWindowShouldClose(window, GL_TRUE);

		// control keys
		bool cam_moved = false;
		if (glfwGetKey (window, GLFW_KEY_A)) {
			cam_pos[0] -= right.v[0] * cam_speed;
			cam_pos[1] -= right.v[1] * cam_speed;
			cam_pos[2] -= right.v[2] * cam_speed;
			cam_moved = true;
		}
		if (glfwGetKey (window, GLFW_KEY_D)) {
			cam_pos[0] += right.v[0] * cam_speed;
			cam_pos[1] += right.v[1] * cam_speed;
			cam_pos[2] += right.v[2] * cam_speed;
			cam_moved = true;
		}
		if (glfwGetKey (window, GLFW_KEY_UP)) {
			cam_pos[0] += up.v[0] * cam_speed;
			cam_pos[1] += up.v[1] * cam_speed;
			cam_pos[2] += up.v[2] * cam_speed;
			cam_moved = true;
		}
		if (glfwGetKey (window, GLFW_KEY_DOWN)) {
			cam_pos[0] -= up.v[0] * cam_speed;
			cam_pos[1] -= up.v[1] * cam_speed;
			cam_pos[2] -= up.v[2] * cam_speed;
			cam_moved = true;
		}
		if (glfwGetKey (window, GLFW_KEY_W)) {
			cam_pos[0] += forward.v[0] * cam_speed;
			cam_pos[1] += forward.v[1] * cam_speed;
			cam_pos[2] += forward.v[2] * cam_speed;
			cam_moved = true;
		}
		if (glfwGetKey (window, GLFW_KEY_S)) {
			cam_pos[0] -= forward.v[0] * cam_speed;
			cam_pos[1] -= forward.v[1] * cam_speed;
			cam_pos[2] -= forward.v[2] * cam_speed;
			cam_moved = true;
		}
		if (glfwGetKey (window, GLFW_KEY_KP_4)) {
			cam_orient = quat_from_axis_deg (-cam_yaw_speed * elapsed_seconds, 0,1,0) * (cam_orient);
			cam_moved = true;
		}
		if (glfwGetKey (window, GLFW_KEY_KP_6)) {
			cam_orient = quat_from_axis_deg (cam_yaw_speed * elapsed_seconds, 0,1,0) * (cam_orient);
			cam_moved = true;
		}
		if (glfwGetKey (window, GLFW_KEY_KP_8)) {
			cam_orient = quat_from_axis_deg (-cam_yaw_speed * elapsed_seconds, 1,0,0) * (cam_orient);
			cam_moved = true;
		}
		if (glfwGetKey (window, GLFW_KEY_KP_5)) {
			cam_orient = quat_from_axis_deg (cam_yaw_speed * elapsed_seconds, 1,0,0) * (cam_orient);
			cam_moved = true;
		}
		if (glfwGetKey (window, GLFW_KEY_KP_7)) {
			cam_orient = quat_from_axis_deg (-cam_yaw_speed * elapsed_seconds, 0,0,1) * (cam_orient);
			cam_moved = true;
		}
		if (glfwGetKey (window, GLFW_KEY_KP_9)) {
			cam_orient = quat_from_axis_deg (cam_yaw_speed * elapsed_seconds, 0,0,1) * (cam_orient);
			cam_moved = true;
		}

		// update view matrix
		if (cam_moved) {
			//glUniformMatrix4fv (cam_loc, 1, GL_FALSE, cam_pos);
			glUniformMatrix4fv (model_loc, 1, GL_FALSE, identity_mat4().m);

			// QUAT CODE
			axis = inverse(quat_to_mat4(cam_orient));
			right = axis * vec4(1.0, 0.0, 0.0, 0.0);
			up = axis * vec4(0.0, 1.0, 0.0, 0.0);
			forward = axis * vec4(0.0, 0.0, -1.0, 0.0);
			view_mat = quat_to_mat4 (cam_orient) * translate (identity_mat4(), vec3 (-cam_pos[0], -cam_pos[1], -cam_pos[2]));
			glUniformMatrix4fv (view_loc, 1, GL_FALSE, view_mat.m);

			env_mat = pers_mat*view_mat;

			// environment
			glBindVertexArray (vao);
			glUniformMatrix4fv (PVM_loc, 1, GL_FALSE, env_mat.m);
			glBindVertexArray (vao1);
			glUniformMatrix4fv (PVM_loc, 1, GL_FALSE, env_mat.m);
		}
	
	}


	return 0;
}