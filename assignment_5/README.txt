How to use the mesh loader code:

GLfloat* vp = NULL; // array of vertex points
GLfloat* vn = NULL; // array of vertex normals (we haven't used these yet)
GLfloat* vt = NULL; // array of texture coordinates (or these)
int g_point_count = 0;
assert (load_obj_file ("my_mesh_file.obj", vp, vt, vn, g_point_count));
