#include "maths_funcs.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "obj_parser.h"
#include "text.h"
#include <SFML/Audio.hpp>

//
// dimensions of the window drawing surface
int gl_width = 800;
int gl_height = 800;

struct Node {
	float x;
	float y;
	float z;
	Node *prev;
	Node *next;
	float radius;
	bool visible;
	float speed;
	float scale;
};

class LinkedList{

// public member
public:
	Node *head;
	Node *tail;
	int length;

	LinkedList(){
		head = new Node();
		tail = head;
		head->next = tail;
		tail->prev = head;
		length = 0;
	}

	void add_node(float xpos, float ypos, float zpos, float r, float speed, float scale = 0.005){
		Node *n = new Node();
		n->x = xpos;
		n->y = ypos;
		n->z = zpos;
		n->radius = r;
		n->visible = true;
		n->speed = speed;
		n->scale = scale;
		tail->next = n;
		n->prev = tail;
		tail = n;
		length++;
	}

	void move_bullets(float max){
		Node *n = head->next;
		for(int i = 0; i < length; i++) {
			n->z += n->speed;
			n = n->next;
		}
		if(head->next->z > max) {
			n = head;
			head = head->next;
			delete n;
			length--;
		}
	}

	void move_meteors(float min){
		Node *n = head->next;
		for(int i = 0; i < length; i++) {
			n->z -= 2;
			n = n->next;
		}
		if(head->next->z < min) {
			n = head;
			head = head->next;
			delete n;
			length--;
		}
	}

	bool is_empty(){
		if(head == tail)
			return true;
		else return false;
	}

	bool* get_vis() {
		bool * vis = (bool*)malloc(length*sizeof(bool));
		Node *n = head->next;
		for(int i = 0; i < length; i++) {
			vis[i] = n->visible;
			n = n->next;
		}
		return vis;
	}

	float ** get_coords() {
		float ** coords = (float**)malloc(length*sizeof(float*));
		for  (int i = 0; i < length; i++)
			coords[i] = (float*)malloc(3*sizeof(float));
		Node *n = head->next;
		for(int i = 0; i < length; i++) {
			coords[i][0] = n->x;
			coords[i][1] = n->y;
			coords[i][2] = n->z;
			n = n->next;
		}
		return coords;
	}

	float * get_scale() {
		float * scale = (float*)malloc(length*sizeof(float));
		Node *n = head->next;
		for(int i = 0; i < length; i++) {
			scale[i] = n->scale;
			n = n->next;
		}
		return scale;
	}

	int get_length() {
		return length;
	}
};

bool in(Node *n, Node *m) {
	if (pow (n->x-m->x,2) + pow (n->y-m->y,2) + pow (n->z-m->z,2) < pow (n->radius+m->radius,2))
			return true;
	return false;
}

int collision(LinkedList *a, LinkedList *b, int score) {
	Node *n = a->head->next;
	Node *m = b->head->next;
	for (int i = 0; i < a->get_length(); i++) {
		m = b->head->next;
		for(int j = 0; j < b->get_length(); j++) {
			if(n->visible && m->visible && in(n,m)) {
				score++;
				n->visible = false;
				m->visible = false;
			}
			m = m->next;
		}
		n = n->next;
	}
	return score;
}

int collision(float *ship, float w, float h, float d, LinkedList *a, int health) {
	Node *n = a->head->next;
	for (int i = 0; i < a->get_length(); i++) {
		if(n->visible && 
			n->x - n->radius < ship[0] + w && n->x + n->radius > ship[0] - w &&
			n->y - n->radius < ship[1] + h && n->y + n->radius > ship[1] - h &&
			n->z - n->radius < ship[2] + d && n->z + n->radius > ship[2] - d) {
			health--;
			n->visible = false;
		}
		n = n->next;
	}
	return health;
}


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
void initialise_texture(GLuint* id, GLuint a, int x, int y, unsigned char* data) {
	glGenTextures(1, id);
	glActiveTexture(a);
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
			// no
		}

		// create a vertex points buffer
		glGenBuffers (1, &points_vbo);
		glBindBuffer (GL_ARRAY_BUFFER, points_vbo);
		// note: this is dynamically allocated memory, so we can't say "sizeof (vp)"
		glBufferData (GL_ARRAY_BUFFER, sizeof (GLfloat) * 3 * (*point_count), vp,
		GL_STATIC_DRAW);

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

void initialise_shader(const char* file, GLuint* id, GLuint shader_type) {
		// allocate some memory to store shader strings
		char * str = (char*)malloc (81920);
		// load shader strings from text files
		assert (parse_file_into_str (file, str, 81920));
		*id = glCreateShader (shader_type);
		glShaderSource (*id, 1, (const char**)&str, NULL);
		// free memory
		free (str);
		glCompileShader (*id);
}

void move (float * obj, vec3 axis, int dir, float speed) {
	obj[0] += dir * axis.v[0] * speed;
	obj[1] += dir * axis.v[1] * speed;
	obj[2] += dir * axis.v[2] * speed;
}




int main () {
	GLFWwindow* window = NULL;
	const GLubyte* renderer;
	const GLubyte* version;

	GLuint shader_programme, ship_shader_programme;
	GLuint vao, vao1, vao2, vao3, vao4;
	GLuint texture0, texture1, texture2, texture3, texture3_dmg, texture4;
	int point_count, point_count1, point_count2, point_count3, point_count4 = 0;

	//
	// Start OpenGL using helper libraries
	// --------------------------------------------------------------------------
	if (!glfwInit ()) {
		fprintf (stderr, "ERROR: could not start GLFW3\n");
		return 1;
	} 

	// change to 3.2 if on Apple OS X
	glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow (gl_width, gl_height, "Hello Space Game", NULL, NULL);
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
    unsigned char *man = stbi_load("spaceman.PNG", &x, &y, &n, 0);
    unsigned char *star = stbi_load("star.PNG", &x, &y, &n, 0);
    unsigned char *space = stbi_load("space2.PNG", &x, &y, &n, 0);
    unsigned char *ship = stbi_load("ship.PNG", &x, &y, &n, 0);
    unsigned char *ship_dmg = stbi_load("ship_dmg.PNG", &x, &y, &n, 0);
    unsigned char *meteor = stbi_load("meteor2.PNG", &x, &y, &n, 0);
	
	initialise_texture(&texture0, GL_TEXTURE0, x, y, space);
	initialise_texture(&texture1, GL_TEXTURE0, x, y, man);
	initialise_texture(&texture2, GL_TEXTURE0, x, y, star);
	initialise_texture(&texture3, GL_TEXTURE0, x, y, ship);
	initialise_texture(&texture3_dmg, GL_TEXTURE1, x, y, ship_dmg);
	initialise_texture(&texture4, GL_TEXTURE0, x, y, meteor);

	//
	// Set up vertex buffers and vertex array object
	// --------------------------------------------------------------------------
	{
		initialise_vao(&vao, &point_count, "space.obj");
		initialise_vao(&vao1, &point_count1, "spaceman.obj");
		initialise_vao(&vao2, &point_count2, "star.obj");
		initialise_vao(&vao3, &point_count3, "ship.obj");
		initialise_vao(&vao4, &point_count4, "spaceman.obj");
	}
	//
	// Load shaders from files
	// --------------------------------------------------------------------------
	{
		GLuint vs, fs, fss;

		initialise_shader("Shaders/mesh.vert", &vs, GL_VERTEX_SHADER);
		initialise_shader("Shaders/mesh.frag", &fs, GL_FRAGMENT_SHADER);
		initialise_shader("Shaders/ship.frag", &fss, GL_FRAGMENT_SHADER);

		shader_programme = glCreateProgram ();
		glAttachShader (shader_programme, fs);
		glAttachShader (shader_programme, vs);
		glLinkProgram (shader_programme);
		
		ship_shader_programme = glCreateProgram ();
		glAttachShader (ship_shader_programme, fss);
		glAttachShader (ship_shader_programme, vs);
		glLinkProgram (ship_shader_programme);
		/* TODO NOTE: you should check for errors and print logs after compiling and also linking shaders */
	}


	//
	// Text
	init_text_rendering ("freemono.png", "freemono.meta", gl_width, gl_height);
	// x and y are -1 to 1
	// size_px is the maximum glyph size in pixels (try 100.0f)
	// r,g,b,a are red,blue,green,opacity values between 0.0 and 1.0
	// if you want to change the text later you will use the returned integer as a parameter
	float text_x = -0.95f;
	float text_y = -0.85f;
	float size_px = 100.0f;
	float text_r = 0.0f;
	float text_g = 0.75f;
	float text_b = 0.0f;
	float text_a = 1.0f;
	int score_id = add_text ("Score: 0", text_x, text_y, size_px, text_r, text_g, text_b, text_a);
	int health_id = add_text ("Health: 10", text_x+1.3, text_y, size_px, text_r, text_g, text_b, text_a);
	int cosmo_id = add_text (":)", 0, text_y, size_px, text_r, text_g, text_b, text_a);
	int lose_id = add_text ("", -(size_px*5)/gl_width, size_px/gl_height, size_px*2, text_r, text_g, text_b, text_a);
	int win_id = add_text ("", -(size_px*5)/gl_width, size_px/gl_height, size_px*2, text_r, text_g, text_b, text_a);
	int restart_id = add_text ("", -(size_px*5)/gl_width, (4*size_px)/gl_height, size_px, text_r, text_g, text_b, text_a);
	
	//
	// Create some matrices
	// --------------------------------------------------------------------------
	// a model matrix
	// can you see what affine transformation this does? y Anton bb I get dis
	
	// All the variables
	float cam_speed = 1.0f; // 1 unit per second
	float cam_yaw_speed = 100.0f; // 10 degrees per second

	float cam_pos[] = {0.0f, 0.0f, -2000.0f}; // don't start at zero, or we will be too close
	float ship_pos[] = {0.0f, -5.0f, -1970.0f}; // don't start at zero, or we will be too close

	// Free camera thing
	versor cam_orient = versor (0.0,0.0,1.0,0.0);
	vec3 right = vec3 (1,0,0);
	vec3 up = vec3 (0,1,0);
	vec3 forward = vec3 (0,0,-1);

	// Setting up matrices
	mat4 t,r,s;
	mat4 axis = inverse(quat_to_mat4(cam_orient));
	mat4 ship_mat = translate (rotate_y_deg(identity_mat4(), 180), vec3 (ship_pos[0], ship_pos[1], ship_pos[2])) * quat_to_mat4 (versor(cam_orient.q[0],cam_orient.q[1],-cam_orient.q[2],cam_orient.q[3]));

	mat4 view_mat = quat_to_mat4 (cam_orient) * translate (identity_mat4(), vec3 (-cam_pos[0], -cam_pos[1], -cam_pos[2])); // QUAT CODE
	mat4 pers_mat = perspective(90/(gl_width/gl_height),gl_width/gl_height,0.1,10000);
	float space_pers = 145;
	mat4 pers_mat2 = perspective(space_pers/(gl_width/gl_height),gl_width/gl_height,0.1,10000);

	// world
	float space_theta = 0.0;
	
	// bullets and meteors
	LinkedList *meteors = new LinkedList();
	LinkedList *bullets = new LinkedList();
	float ** coords;
	float * scales;
	bool * draw;
	int wait = 0;
	float meteor_rad = 6;
	float bullet_rad = 1;

	//ship
	float ship_w = 6;
	float ship_h = 3;
	float ship_d = 6;
	float ship_x_rot = 0;
	float ship_z_rot = 0;
	int ship_dir = 1;
	int barrel_roll_dir = 1;
	bool barrel_roll = false;
	bool turn_around = false;
	bool turned = false;

	// man
	float man_theta = 0;
	float man_w = 2;
	float man_h = 4;
	float man_d = 2;
	float man_pos[] = {0,0,0};
	int alive = 1;

	// Game
	int score = 0;
	int max_health = 10;
	int health = max_health;
	bool lose = false;
	bool win = false;

	// Music
	sf::Music music, shoot, boost, hit, die;
	if (!music.openFromFile("Audio/brody.wav"))
		return -1; // error
	if (!shoot.openFromFile("Audio/shoot.wav"))
		return -1; // error
	shoot.setPitch(3);
	shoot.setVolume(75);
	if (!hit.openFromFile("Audio/ow.wav"))
		return -1; // error
	if (!die.openFromFile("Audio/die.wav"))
		return -1; // error
	music.play();

	// location of "M" in vertex shader
	int PVM_loc = glGetUniformLocation (shader_programme, "PVM");
	assert (PVM_loc > -1);
	int model_loc = glGetUniformLocation (shader_programme, "model");
	int view_loc = glGetUniformLocation (shader_programme, "view");
	
	
	int ship_PVM_loc = glGetUniformLocation (ship_shader_programme, "PVM");
	assert (ship_PVM_loc > -1);
	int ship_model_loc = glGetUniformLocation (ship_shader_programme, "model");
	int ship_view_loc = glGetUniformLocation (ship_shader_programme, "view");
	int ship_loc = glGetUniformLocation (ship_shader_programme, "ship");
	int ship_dmg_loc = glGetUniformLocation (ship_shader_programme, "ship_dmg");
	int dmg_loc = glGetUniformLocation (ship_shader_programme, "dmg_fac");

	// send matrix values to shader immediately
	glUseProgram (shader_programme);
	glUniformMatrix4fv (PVM_loc, 1, GL_FALSE, identity_mat4().m);
	glUniformMatrix4fv (model_loc, 1, GL_FALSE, identity_mat4().m);
	glUniformMatrix4fv (view_loc, 1, GL_FALSE, view_mat.m);
	
	glUseProgram (ship_shader_programme);
	glUniformMatrix4fv (ship_PVM_loc, 1, GL_FALSE, identity_mat4().m);
	glUniformMatrix4fv (ship_model_loc, 1, GL_FALSE, identity_mat4().m);
	glUniformMatrix4fv (ship_view_loc, 1, GL_FALSE, view_mat.m);
	glUniform1f (dmg_loc, ((float)health/(float)max_health));
	
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


		// spinny spinny worldy worldy
		space_theta += 0.05;
		r = rotate_z_deg (identity_mat4 (), space_theta);
		s = scale (identity_mat4(), vec3 (4,4,4));
		glBindVertexArray (vao);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D,texture0);
		glUniformMatrix4fv (PVM_loc, 1, GL_FALSE, (pers_mat2*view_mat*r*s).m);
		glUniformMatrix4fv (model_loc, 1, GL_FALSE, (r*s).m);
		glDrawArrays (GL_TRIANGLES, 0, point_count);
		
		// ship
		// trying to get multiple textures here
		// only part with a diff shader programme so I just wrapped it in UseProgs so I wouldn't forget
		// except nothing's working D:
		glUseProgram (ship_shader_programme);
		glUniform1f (dmg_loc, ((float)health/(float)max_health));
		glBindVertexArray (vao3);
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, texture3);
		glUniform1i (ship_loc, 0);
		glActiveTexture (GL_TEXTURE1);
		glBindTexture (GL_TEXTURE_2D, texture3_dmg);
		glUniform1i (ship_dmg_loc, 1);
		r = rotate_x_deg (identity_mat4 (), 57.2957795*ship_x_rot);
		r = rotate_z_deg (r, 57.2957795*ship_z_rot);
		glUniformMatrix4fv (PVM_loc, 1, GL_FALSE, (pers_mat*view_mat*ship_mat*r).m);
		glUniformMatrix4fv (model_loc, 1, GL_FALSE, (ship_mat*r).m);
		glDrawArrays (GL_TRIANGLES, 0, point_count3);
		glUseProgram (shader_programme);

		// cosmonaut
		if (alive == 1) {
			glBindVertexArray (vao1);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D,texture1);
			s = scale (identity_mat4 (), vec3 (0.5, 0.5, 0.5));
			r = rotate_y_deg(identity_mat4 (), 90);
			t = translate(identity_mat4 (), vec3 (5*sin(man_theta), -5*cos(man_theta), -5*cos(man_theta)));
			man_theta += 0.01;
			man_pos[0] = ship_pos[0]+5*sin(man_theta);
			man_pos[1] = ship_pos[1]-5*cos(man_theta);
			man_pos[2] = ship_pos[2]-5*cos(man_theta);
			glUniformMatrix4fv (PVM_loc, 1, GL_FALSE, (pers_mat*view_mat*ship_mat*t*r*s).m);
			glUniformMatrix4fv (model_loc, 1, GL_FALSE, (ship_mat*t*r*s).m);
			glDrawArrays (GL_TRIANGLES, 0, point_count1);
		}

		// bullet
		if(!bullets->is_empty()) {
			bullets->move_bullets(ship_pos[2]+200);
			coords = bullets->get_coords();
			draw = bullets->get_vis();
			glBindVertexArray (vao);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D,texture2);
			int len = bullets->get_length();
			for(int i = 0; i < len; i++) {
				if(draw[i]) {
					s = scale (identity_mat4(), vec3 (0.0005,0.0005,0.0005));
					t = translate (identity_mat4(), vec3 (coords[i][0], coords[i][1], coords[i][2]));
					glUniformMatrix4fv (PVM_loc, 1, GL_FALSE, (pers_mat*view_mat*t*s).m);
					glUniformMatrix4fv (model_loc, 1, GL_FALSE, (t*s).m);
					glDrawArrays (GL_TRIANGLES, 0, point_count);
				}
			}
		}

		// meteor
		if (ship_dir == 1) {
			if (!win && !lose && glfwGetKey (window, GLFW_KEY_F)) {
				if (rand()%3 == 0) {
					float xcoord = ((rand()%1000)-500)/10;
					float ycoord = ((rand()%1000)-500)/10;
					float meteor_scale = 5*(rand()%1000)/1000;
					meteors->add_node(xcoord, ycoord, ship_pos[2]+200, meteor_rad*meteor_scale, 0, meteor_scale);
				}
			} else {
				if (rand()%30 == 0) {
					float xcoord = ((rand()%1000)-500)/10;
					float ycoord = ((rand()%1000)-500)/10;
					float meteor_scale = 5*(rand()%1000)/1000;
					meteors->add_node(xcoord, ycoord, ship_pos[2]+200, meteor_rad*meteor_scale, 0, meteor_scale);
				}
			}
		}
		if(!meteors->is_empty()) {
			meteors->move_meteors(cam_pos[2]-5);
			coords = meteors->get_coords();
			draw = meteors->get_vis();
			scales = meteors->get_scale();
			glBindVertexArray (vao2);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D,texture4);
			int len = meteors->get_length();
			for(int i = 0; i < len; i++) {
				if(draw[i]) {
					s = scale (identity_mat4(), vec3 (scales[i],scales[i],scales[i]));
					t = translate (identity_mat4(), vec3 (coords[i][0], coords[i][1], coords[i][2]));
					glUniformMatrix4fv (PVM_loc, 1, GL_FALSE, (pers_mat*view_mat*t*s).m);
					glUniformMatrix4fv (model_loc, 1, GL_FALSE, (t*s).m);
					glDrawArrays (GL_TRIANGLES, 0, point_count2);
				}
			}
		}

		// text
		char tmp[256];
		sprintf (tmp, "Score: %d\n", score);
		update_text (score_id, tmp);
		sprintf (tmp, "Health: %d\n", health);
		update_text (health_id, tmp);
		if (!alive) {
			music.stop();
			sprintf (tmp, ":(");
			update_text (cosmo_id, tmp);
		}
		if (lose) {
			sprintf (tmp, "GAME OVER");
			update_text (lose_id, tmp);
		}
		if (win)  {
			sprintf (tmp, "YOU WIN :D");
			update_text (win_id, tmp);
		}
		if (win || lose) {
			sprintf (tmp, "Press Space to restart\n(or T for special mode)");
			update_text (restart_id, tmp);
		}
		draw_texts ();
		glUseProgram (shader_programme);

		/* this just updates window events and keyboard input events (not used yet) */
		glfwPollEvents ();
		glfwSwapBuffers (window);

		// close window
		if (glfwGetKey (window, GLFW_KEY_ESCAPE)) {
			music.stop();
			glfwSetWindowShouldClose(window, GL_TRUE);
		}

		if ((win || lose) && !wait && (glfwGetKey (window, GLFW_KEY_SPACE) || glfwGetKey (window, GLFW_KEY_T))) {
			// reset evrything
			cam_pos[0] = 0.0f;
			cam_pos[1] = 0.0f;
			cam_pos[2] = -100.0f;
			ship_pos[0] = 0.0f;
			ship_pos[1] = -5.0f;
			ship_pos[2] = -70.0f;
			ship_x_rot = 0;
			ship_z_rot = 0;
			space_pers = 145;
			pers_mat2 = perspective(space_pers/(gl_width/gl_height),gl_width/gl_height,0.1,10000);
			space_theta = 0.0;
			meteors = new LinkedList();
			bullets = new LinkedList();
			wait = 0;
			score = 0;
			health = max_health;
			alive = 1;
			lose = false;
			win = false;
			sprintf (tmp, "");
			update_text (lose_id, tmp);
			update_text (win_id, tmp);
			update_text (restart_id, tmp);
			sprintf (tmp, "Score: %d\n", score);
			update_text (score_id, tmp);
			sprintf (tmp, "Health: %d\n", health);
			update_text (health_id, tmp);
			sprintf (tmp, ":)");
			update_text (cosmo_id, tmp);
			draw_texts ();
			glUseProgram (shader_programme);
			music.stop();
			if (glfwGetKey (window, GLFW_KEY_T)) music.openFromFile("Audio/sio.wav");
			else music.openFromFile("Audio/brody.wav");
			music.play();
		} else if (wait) wait--;

		score = collision(bullets, meteors, score);
		if (alive == 1) alive = collision(man_pos, man_w, man_h, man_d, meteors, alive);
		int prev_health = health;
		health = collision(ship_pos, ship_w, ship_h, ship_d, meteors, health);
		if (prev_health != health) hit.play();

		if (!win && !lose ) {
			if(!health) {
				die.play();
				lose = true;
				wait = 30;
			}
			if(score == 50) {
				win = true;
				wait = 30;
			}

			// control keys
			if(ship_pos[0]-cam_pos[0] < 20)
				if (glfwGetKey (window, GLFW_KEY_A))
					move(ship_pos, right, -1, cam_speed);
			if(ship_pos[0]-cam_pos[0] > -20)
				if (glfwGetKey (window, GLFW_KEY_D))
					move(ship_pos, right, 1, cam_speed);
			if(ship_pos[1]-cam_pos[1] < 20)
				if (glfwGetKey (window, GLFW_KEY_W))
					move(ship_pos, up, 1, cam_speed);
			if(ship_pos[1]-cam_pos[1] > -20)
				if (glfwGetKey (window, GLFW_KEY_S))
					move(ship_pos, up, -1, cam_speed);
			if (glfwGetKey (window, GLFW_KEY_SPACE) && !wait) {
				if (glfwGetKey (window, GLFW_KEY_F)) bullets->add_node(ship_pos[0], ship_pos[1], ship_pos[2], bullet_rad, ship_dir*(99*(cam_speed/10)+2));
				else bullets->add_node(ship_pos[0], ship_pos[1], ship_pos[2], bullet_rad, ship_dir*2);
				wait = 10;
				shoot.play();
			}
			else if (wait) wait--; // shoot in intervals, not just constant stream if you hold space

			if (glfwGetKey (window, GLFW_KEY_E)) {
				barrel_roll = true;
				barrel_roll_dir = 1;
			}
			if (glfwGetKey (window, GLFW_KEY_Q)) {
				barrel_roll = true;
				barrel_roll_dir = -1;
			}

			// :D
			if (barrel_roll) {
				if (barrel_roll_dir == 1 && ship_z_rot < 2*M_PI) {
					ship_z_rot += 0.2;
					if(ship_pos[0]-cam_pos[0] > -20) ship_pos[0] -= 1.5;
				} else if (barrel_roll_dir == -1 && ship_z_rot > -2*M_PI) {
					ship_z_rot -= 0.2;
					if(ship_pos[0]-cam_pos[0] < 20) ship_pos[0] += 1.5;
				} else {
					ship_z_rot = 0;
					barrel_roll = false;
				}
			}

			// FLIP THE SHIP
			if (turn_around) {
				if (ship_x_rot > -M_PI || ship_x_rot < 0) ship_x_rot += ship_dir*0.1;
				if (ship_z_rot > -M_PI || ship_z_rot < 0) ship_z_rot += ship_dir*0.1;
				if ((ship_x_rot >= 0 && ship_z_rot >= 0) || (ship_x_rot <= -M_PI && ship_z_rot <= -M_PI)) {
					turn_around = false;
					if (turned) turned = false;
					else turned = true;
					if (ship_x_rot >= 0 && ship_z_rot >= 0) {
						ship_x_rot = 0;
						ship_z_rot = 0;
					}
				}
			}

			// Constantly moving forward
			if (glfwGetKey (window, GLFW_KEY_F) || ship_dir == -1) {
				move(ship_pos, forward, ship_dir, 10*cam_speed);
				move(cam_pos, forward, ship_dir, 10*cam_speed);
				music.setPitch(1.2);
				if (space_pers < 160) {
					space_pers++;
					pers_mat2 = perspective(space_pers/(gl_width/gl_height),gl_width/gl_height,0.1,10000);
				}
			} else {
				move(ship_pos, forward, ship_dir, cam_speed / 10);
				move(cam_pos, forward, ship_dir, cam_speed / 10);
				music.setPitch(1);
				if (space_pers > 145) {
					space_pers--;
					pers_mat2 = perspective(space_pers / (gl_width / gl_height), gl_width / gl_height, 0.1, 10000);
				}
			}
		}

		// So at some point the ship is gonna reach the edge of space so instead of just stopping the game I'm gonna flip the ship
		// FLIP THE SHIP
		if (ship_pos[2] > 2000) {
			ship_dir = -1;
			turn_around = true;
		} else if (ship_pos[2] < -2000) {
			ship_dir = 1;
			turn_around = true;
		}

		// QUAT CODE
		axis = inverse(quat_to_mat4(cam_orient));
		right = axis * vec4(1.0, 0.0, 0.0, 0.0);
		up = axis * vec4(0.0, 1.0, 0.0, 0.0);
		forward = axis * vec4(0.0, 0.0, -1.0, 0.0);
		view_mat = quat_to_mat4 (cam_orient) * translate (identity_mat4(), vec3 (-cam_pos[0], -cam_pos[1], -(ship_pos[2]-30)));
		ship_mat = translate (rotate_y_deg(identity_mat4(), 180), vec3 (ship_pos[0], ship_pos[1], ship_pos[2])) * quat_to_mat4 (versor(cam_orient.q[0],cam_orient.q[1],-cam_orient.q[2],cam_orient.q[3]));
		glUniformMatrix4fv (view_loc, 1, GL_FALSE, view_mat.m);
	}

	return 0;
}