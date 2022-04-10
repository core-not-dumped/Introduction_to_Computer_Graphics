#include "cgmath.h"		// slee's simple math library
#include "cgut.h"		// slee's OpenGL utility

//*************************************
// global constants
static const char*	window_name = "project-A2";
static const char*	vert_shader_path = "../bin/shaders/transform.vert";
static const char*	frag_shader_path = "../bin/shaders/transform.frag";
static const uint	MIN_INSTANCE = 1;	// minimum instances
static const uint	MAX_INSTANCE = 10;	// maximum instances
uint				NUM_INSTANCE = 1;	// initial instances

int gra = 0;
uint NUM_TESS = 40;
uint tri_num = 0;
std::vector<vertex>	unit_sphere_vertices;
bool b_wireframe = false;
bool rotate = false;
float rotate_time = 0.0f;
float small_rotate_time = 0.0f;
bool front;
bool back;
int press = 0;
dvec2 pos;
dvec2 later_pos;
std::vector<float> sphere_radius;
std::vector<int> sphere_color;
bool small_rotate;

//*************************************
// window objects
GLFWwindow* window = nullptr;
ivec2		window_size = ivec2(1280, 720); // initial window size

//*************************************
// common structures
struct camera
{
	vec3	eye = vec3( 0, 0, 2 );
	vec3	at = vec3( 0, 0, 0 );
	vec3	up = vec3( 0, 1, 0 );
	mat4	view_matrix = mat4::look_at( eye, at, up );
		
	float	fovy = PI/3.0f; // must be in radian
	float	aspect_ratio = window_size.x / float(window_size.y);
	float	dNear = 1.0f;
	float	dFar = 1000.0f;
	mat4	projection_matrix;
};


//*************************************
// OpenGL objects
GLuint	program	= 0;	// ID holder for GPU program
GLuint	vertex_array = 0;	// ID holder for vertex array object

//*************************************
// global variables
int		frame = 0;		// index of rendering frames

//*************************************
// scene objects
camera	cam;

//*************************************
void update()
{
	// update projection matrix
	cam.aspect_ratio = window_size.x/float(window_size.y);
	cam.projection_matrix = mat4::perspective(cam.fovy, cam.aspect_ratio, cam.dNear, cam.dFar);

	// update uniform variables in vertex/fragment shaders
	GLint uloc;
	uloc = glGetUniformLocation( program, "view_matrix" );			if(uloc>-1) glUniformMatrix4fv( uloc, 1, GL_TRUE, cam.view_matrix );		// update the view matrix (covered later in viewing lecture)
	uloc = glGetUniformLocation( program, "projection_matrix" );	if(uloc>-1) glUniformMatrix4fv( uloc, 1, GL_TRUE, cam.projection_matrix );	// update the projection matrix (covered later in viewing lecture)
}

inline float randf(float m = 0, float M = 1.0f)
{
	float r = rand() / float(RAND_MAX);
	return r * (M - m) + m;
}

void render()
{
	// clear screen (with background color) and clear depth buffer
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	// notify GL that we use our own program
	glUseProgram( program );
	
	// bind vertex array object
	glBindVertexArray( vertex_array );

	if (back)	cam.eye += normalize(cam.eye) * 0.001f;
	if (front)	cam.eye -= normalize(cam.eye) * 0.001f;
	if (press == 1)
	{
		vec3 ax = vec3(0, 1, 0);
		dvec2 diff = later_pos - pos;
		pos = later_pos;
		float angle_x = -PI / 2.0f * float(diff.x) / 200.0f;
		mat4 cam_eye_tmp =
		{
			0,0,0,cam.eye.x,
			0,0,0,cam.eye.y,
			0,0,0,cam.eye.z,
			0,0,0,1
		};
		mat4 tmp = mat4::rotate(ax, angle_x) * cam_eye_tmp;

		cam.eye.x = tmp[3];
		cam.eye.y = tmp[7];
		cam.eye.z = tmp[11];
	}

	if(back || front || press)	cam.view_matrix = mat4::look_at(cam.eye, cam.at, cam.up);

	if (rotate == 1)		rotate_time += 0.001f;
	if (small_rotate == 1)	small_rotate_time += 0.001f;

	// render vertices: trigger shader programs to process vertex data
	for (int k = 0, kn = int(NUM_INSTANCE); k < kn; k++)
	{
		// configure transformation parameters
		float theta1 = rotate_time;
		float theta2 = small_rotate_time;
		float pla = (2.0f * float(k) * PI) / (NUM_INSTANCE - 1);
		float mul = 2.0f;

		mat4 model_matrix;
		// build the model matrix

		if (k == 0)
		{
			model_matrix =	mat4::translate(0.0f, 0.0f, 0.0f) *
							mat4::translate(cam.at) *
							mat4::rotate(vec3(0, 1, 0), theta1) *
							mat4::translate(-cam.at);
		}
		else
		{
			model_matrix =	mat4::rotate(vec3(0, 1, 0), theta2) *
							mat4::translate(mul * normalize(vec3(cos(pla), 0.0f, sin(pla)))) *
							mat4::translate(cam.at) *
							mat4::rotate(vec3(0, 1, 0), theta1) *
							mat4::scale(vec3(sphere_radius[k - 1])) *
							mat4::translate(-cam.at);
		}
		// update the uniform model matrix and render
		if (k == 0)
		{
			glUniform1i(glGetUniformLocation(program, "gra"), gra);
		}
		else
		{
			glUniform1i(glGetUniformLocation(program, "gra"), sphere_color[k-1]);
		}
		glUniformMatrix4fv( glGetUniformLocation( program, "model_matrix" ), 1, GL_TRUE, model_matrix );
		glDrawElements( GL_TRIANGLES, tri_num * 3, GL_UNSIGNED_INT, nullptr );
	}

	// swap front and back buffers, and display to screen
	glfwSwapBuffers( window );
}

void reshape( GLFWwindow* window, int width, int height )
{
	// set current viewport in pixels (win_x, win_y, win_width, win_height)
	// viewport: the window area that are affected by rendering 
	window_size = ivec2(width,height);
	glViewport( 0, 0, width, height );
}

void print_help()
{
	printf( "[help]\n" );
	printf( "- press ESC or 'q' to terminate the program\n" );
	printf( "- press F1 or 'h' to see help\n" );
	printf( "- press '+/-' to increase/decrease the number of instances (min=%d, max=%d)\n", MIN_INSTANCE, MAX_INSTANCE );
	printf( "\n" );
}

void keyboard( GLFWwindow* window, int key, int scancode, int action, int mods )
{
	if(action==GLFW_PRESS)
	{
		if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q)	glfwSetWindowShouldClose(window, GL_TRUE);
		else if (key == GLFW_KEY_H || key == GLFW_KEY_F1)	print_help();
		else if (key == GLFW_KEY_KP_ADD || (key == GLFW_KEY_EQUAL && (mods & GLFW_MOD_SHIFT))/* + */)
		{
			if (NUM_INSTANCE >= MAX_INSTANCE) return;
			printf("> NUM_INSTANCE = % -4d\r", ++NUM_INSTANCE);
			sphere_radius.push_back(randf(0.1f, 0.4f));
			sphere_color.push_back(int(randf(0.0f, 9.0f - 0.00001f)));
		}
		else if (key == GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_MINUS)
		{
			if (NUM_INSTANCE <= MIN_INSTANCE) return;
			printf("> NUM_INSTANCE = % -4d\r", --NUM_INSTANCE);
			sphere_radius.pop_back();
			sphere_color.pop_back();
		}
		else if (key == GLFW_KEY_R)	rotate = !rotate;
		else if (key == GLFW_KEY_D)
		{
			gra = (gra + 1) % 3;
			printf("color : %d\n", gra);
		}
		else if (key == GLFW_KEY_Z)
		{
			back = !back;
			front = false;
		}
		else if (key == GLFW_KEY_X)
		{
			front = !front;
			back = false;
		}
		else if (key == GLFW_KEY_T)
		{
			small_rotate = !small_rotate;
		}
#ifndef GL_ES_VERSION_2_0
		else if (key == GLFW_KEY_W)
		{
			b_wireframe = !b_wireframe;
			glPolygonMode(GL_FRONT_AND_BACK, b_wireframe ? GL_LINE : GL_FILL);
			printf("> using %s mode\n", b_wireframe ? "wireframe" : "solid");
		}
#endif
	}
}

void mouse( GLFWwindow* window, int button, int action, int mods )
{
	if(button==GLFW_MOUSE_BUTTON_LEFT&&action==GLFW_PRESS )
	{
		press = 1;
		glfwGetCursorPos(window,&pos.x,&pos.y);
		printf( "> Left mouse button pressed at (%d, %d)\n", int(pos.x), int(pos.y) );
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		press = 0;
	}
}

void motion( GLFWwindow* window, double x, double y )
{
	glfwGetCursorPos(window, &x, &y);
	later_pos.x = x;
	later_pos.y = y;
	printf("> Left mouse button see at (%d, %d)\r", int(x), int(y));
}

std::vector<vertex> create_sphere_vertices(uint N)
{
	std::vector<vertex> v;
	for (uint k = 0; k <= N; k++)
	{
		float theta = PI * k / float(N);
		float ct = cos(theta), st = sin(theta);
		for (uint j = 0; j <= 2 * N; j++)
		{
			float py = PI * j / float(N);
			float cp = cos(py), sp = sin(py);
			v.push_back({ vec3(st * sp, ct, st * cp), vec3(st * sp,ct,st * cp) , vec2(py / (2 * PI),1 - theta / PI) });
		}
	}
	return v;
}

void update_vertex_buffer(const std::vector<vertex>& vertices, uint N)
{
	static GLuint vertex_buffer = 0;	// ID holder for vertex buffer
	static GLuint index_buffer = 0;		// ID holder for index buffer

	// clear and create new buffers
	if (vertex_buffer)	glDeleteBuffers(1, &vertex_buffer);	vertex_buffer = 0;
	if (index_buffer)	glDeleteBuffers(1, &index_buffer);	index_buffer = 0;

	// check exceptions
	if (vertices.empty()) { printf("[error] vertices is empty.\n"); return; }

	// create buffers
	std::vector<uint> indices;
	tri_num = 0;

	for (uint k = 0; k < N - 1; k++)
	{
		for (uint j = 0; j < 2 * N; j++)
		{
			indices.push_back((k * (2 * N + 1)) + j);
			indices.push_back((k * (2 * N + 1)) + 2 * N + 1 + j);
			indices.push_back((k * (2 * N + 1)) + 2 * N + 2 + j);
			tri_num++;
		}
	}
	for (uint k = 1; k < N; k++)
	{
		for (uint j = 0; j < 2 * N; j++)
		{
			indices.push_back((k * (2 * N + 1)) + j);
			indices.push_back((k * (2 * N + 1)) + 2 * N + 2 + j);
			indices.push_back((k * (2 * N + 1)) + 1 + j);
			tri_num++;
		}
	}

	// generation of vertex buffer: use vertices as it is
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

	// geneation of index buffer
	glGenBuffers(1, &index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * indices.size(), &indices[0], GL_STATIC_DRAW);

	// generate vertex array object, which is mandatory for OpenGL 3.3 and higher
	if (vertex_array) glDeleteVertexArrays(1, &vertex_array);
	vertex_array = cg_create_vertex_array(vertex_buffer, index_buffer);
	if (!vertex_array) { printf("%s(): failed to create vertex aray\n", __func__); return; }
}

bool user_init()
{
	// log hotkeys
	print_help();

	// init GL states
	glLineWidth(1.0f);
	glClearColor( 39/255.0f, 40/255.0f, 34/255.0f, 1.0f );	// set clear color
	glEnable( GL_CULL_FACE );								// turn on backface culling
	glEnable( GL_DEPTH_TEST );								// turn on depth tests

	// define the position of four corner vertices
	unit_sphere_vertices = std::move(create_sphere_vertices(NUM_TESS));

	// create vertex buffer; called again when index buffering mode is toggled
	update_vertex_buffer(unit_sphere_vertices, NUM_TESS);

	return true;
}

void user_finalize()
{
}

int main( int argc, char* argv[] )
{
	// create window and initialize OpenGL extensions
	if(!(window = cg_create_window( window_name, window_size.x, window_size.y ))){ glfwTerminate(); return 1; }
	if(!cg_init_extensions( window )){ glfwTerminate(); return 1; }	// version and extensions

	// initializations and validations
	if(!(program=cg_create_program( vert_shader_path, frag_shader_path ))){ glfwTerminate(); return 1; }	// create and compile shaders/program
	if(!user_init()){ printf( "Failed to user_init()\n" ); glfwTerminate(); return 1; }					// user initialization

	// register event callbacks
	glfwSetWindowSizeCallback( window, reshape );	// callback for window resizing events
    glfwSetKeyCallback( window, keyboard );			// callback for keyboard events
	glfwSetMouseButtonCallback( window, mouse );	// callback for mouse click inputs
	glfwSetCursorPosCallback( window, motion );		// callback for mouse movement

	// enters rendering/event loop
	for( frame=0; !glfwWindowShouldClose(window); frame++ )
	{
		glfwPollEvents();	// polling and processing of events
		update();			// per-frame update
		render();			// per-frame render
	}

	// normal termination
	user_finalize();
	cg_destroy_window(window);

	return 0;
}
