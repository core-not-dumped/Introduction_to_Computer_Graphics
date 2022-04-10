#include "cgmath.h"		// slee's simple math library
#include "stb_image.h"
#define STB_IMAGE_IMPLEMENTATION
#include "cgut.h"		// slee's OpenGL utility
#include "trackball.h"

//*************************************
// global constants
static const char*	window_name = "solar_system";
static const char*	vert_shader_path = "../bin/shaders/transform.vert";
static const char*	frag_shader_path = "../bin/shaders/transform.frag";
static const char*	moon_image_path		= "../bin/images/moonmap.jpg";
static const char*	sun_image_path		= "../bin/images/sunmap.jpg";
static const char*	mercury_image_path	= "../bin/images/mercurymap.jpg";
static const char*	venus_image_path	= "../bin/images/venusmap.jpg";
static const char*	earth_image_path	= "../bin/images/earthmap.jpg";
static const char*	mars_image_path		= "../bin/images/marsmap.jpg";
static const char*	jupiter_image_path	= "../bin/images/jupitermap.jpg";
static const char*	saturn_image_path	= "../bin/images/saturnmap.jpg";
static const char*	uranus_image_path	= "../bin/images/uranusmap.jpg";
static const char*	neptune_image_path	= "../bin/images/neptunemap.jpg";
static const char*	saturnring_image_path = "../bin/images/saturnringmap.jpg";

uint				NUM_planet = 9;	// initial instances
uint				NUM_satellite = 12;

uint NUM_TESS = 40;
uint tri_num = 0;
std::vector<vertex>	unit_sphere_vertices;
std::vector<vertex> unit_ring_vertices;
bool b_wireframe = false;
float rotate_time = 0.0f;
bool front;
bool back;
dvec2 pos;
dvec2 later_pos;

bool press = 0;
bool right_press = 0;
bool middle_press = 0;

struct Sphere {
	float distance[8] = {1.3f, 2.2f, 3.0f, 4.0f, 5.5f, 8.0f, 10.0f, 13.0f};
	float radius[8] = { 0.12f, 0.19f, 0.23f, 0.16f, 0.56f, 0.48f, 0.35f, 0.33f };
	float revolution[8] = { 0.00142f, 0.00102f, 0.00064f, 0.00045f, 0.0003f, 0.0002f, 0.00015f, 0.00011f };
	float revolution_time[8] = { 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f };
}sphere;

struct Satellite {
	int parent[15] = { 2,3,3,4,4,4,6,6,6,7,7,7 };
	float radius[15] = { 0.07f, 0.05f, 0.03f,0.08f, 0.1f, 0.06f,0.08f, 0.10f, 0.06f,0.08f, 0.10f, 0.06f };
	float distance[15] = { 0.4f, 0.3f, 0.4f, 0.7f, 0.9f, 1.1f,0.7f, 0.9f, 1.1f, 0.7f, 0.9f, 1.1f };
	float revolution[15] = { 0.001f, 0.001f, 0.0004f,0.001f, 0.00045f, 0.00034f,0.001f, 0.00045f, 0.00034f,0.001f, 0.00045f, 0.00034f };
	float revolution_time[15] = { 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f };
}satellite;

struct light_t
{
	vec4	position = vec4(1.0f, 1.0f, 1.0f, 0.0f);   // directional light
	vec4	ambient = vec4(0.2f, 0.2f, 0.2f, 1.0f);
	vec4	diffuse = vec4(0.8f, 0.8f, 0.8f, 1.0f);
	vec4	specular = vec4(1.0f, 1.0f, 1.0f, 1.0f);
};

struct material_t
{
	vec4	ambient = vec4(0.2f, 0.2f, 0.2f, 1.0f);
	vec4	diffuse = vec4(0.8f, 0.8f, 0.8f, 1.0f);
	vec4	specular = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	float	shininess = 500.0f;
};

//*************************************
// window objects
GLFWwindow* window = nullptr;
ivec2		window_size = cg_default_window_size(); // initial window size

//*************************************
// common structures
struct camera
{
	vec3	eye = vec3( 0, 0, 7 );
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
GLuint	vertex_array_ring = 0;

GLuint	Moon = 0;
GLuint	Sun = 0;
GLuint	Mercury = 0;			
GLuint	Venus = 0;			
GLuint	Earth = 0;
GLuint	Mars = 0;
GLuint	Jupiter = 0;
GLuint	Saturn = 0;
GLuint	Uranus = 0;
GLuint	Neptune = 0;
GLuint	Saturnring = 0;

//*************************************
// global variables
int		frame = 0;		// index of rendering frames

//*************************************
// scene objects
camera	cam;
trackball	tb;
bool p_shift;
bool p_ctrl;
light_t		light;
material_t	material;

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

	// setup light properties
	glUniform4fv(glGetUniformLocation(program, "Ia"), 1, light.ambient);
	glUniform4fv(glGetUniformLocation(program, "Id"), 1, light.diffuse);
	glUniform4fv(glGetUniformLocation(program, "Is"), 1, light.specular);

	// setup material properties
	glUniform4fv(glGetUniformLocation(program, "Ka"), 1, material.ambient);
	glUniform4fv(glGetUniformLocation(program, "Kd"), 1, material.diffuse);
	glUniform4fv(glGetUniformLocation(program, "Ks"), 1, material.specular);
	glUniform1f(glGetUniformLocation(program, "shininess"), material.shininess);
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

	if ((p_shift && press) || right_press)
	{
		dvec2 diff = pos - later_pos;
		pos = later_pos;
		vec3 to_zero = vec3(cam.view_matrix[3], cam.view_matrix[7], cam.view_matrix[11]);
		cam.view_matrix =
			cam.view_matrix *
			mat4::scale(vec3(1.0f + float(diff.y) * 0.03f));
	}
	else if ((p_ctrl && press) || middle_press)
	{
		dvec2 diff = pos - later_pos;
		pos = later_pos;

		cam.view_matrix =
			mat4::translate(-float(diff.x) * 0.01f, float(diff.y) * 0.01f, 0) *
			cam.view_matrix;
	}

	rotate_time += 0.001f;

	// render vertices: trigger shader programs to process vertex data
	for (int k = 0, kn = int(NUM_planet); k < kn; k++)
	{
		// configure transformation parameters
		float theta1 = rotate_time;

		mat4 model_matrix;
		// build the model matrix

		if (k == 0)
		{
			model_matrix = mat4::translate(0.0f, 0.0f, 0.0f) *
				mat4::rotate(vec3(0, 1, 0), theta1);
		}
		else
		{
			sphere.revolution_time[k - 1] += sphere.revolution[k - 1];
			model_matrix =	mat4::rotate(vec3(0, 1, 0), sphere.revolution_time[k-1]) *
							mat4::translate(vec3(sphere.distance[k - 1], 0, 0.0f)) *
							mat4::rotate(vec3(0, 1, 0), theta1) *
							mat4::scale(vec3(sphere.radius[k - 1]));
			light.position = vec4(-model_matrix[3], -model_matrix[7], -model_matrix[11], 0.0f);
		}

		glUniform1i(glGetUniformLocation(program, "mode"), k);

		if (k == 0)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, Sun);
			glUniform1i(glGetUniformLocation(program, "TEX_SUN"), 0);
		}
		else if (k == 1)
		{
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, Mercury);
			glUniform1i(glGetUniformLocation(program, "TEX_MERCURY"), 1);
		}
		else if (k == 2)
		{
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, Venus);
			glUniform1i(glGetUniformLocation(program, "TEX_VENUS"), 2);
		}
		else if (k == 3)
		{
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, Earth);
			glUniform1i(glGetUniformLocation(program, "TEX_EARTH"), 3);
		}
		else if (k == 4)
		{
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, Mars);
			glUniform1i(glGetUniformLocation(program, "TEX_MARS"), 4);
		}
		else if (k == 5)
		{
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_2D, Jupiter);
			glUniform1i(glGetUniformLocation(program, "TEX_JUPITER"), 5);
		}
		else if (k == 6)
		{
			glActiveTexture(GL_TEXTURE6);
			glBindTexture(GL_TEXTURE_2D, Saturn);
			glUniform1i(glGetUniformLocation(program, "TEX_SATURN"), 6);
		}
		else if (k == 7)
		{
			glActiveTexture(GL_TEXTURE7);
			glBindTexture(GL_TEXTURE_2D, Uranus);
			glUniform1i(glGetUniformLocation(program, "TEX_URANUS"), 7);
		}
		else if (k == 8)
		{
			glActiveTexture(GL_TEXTURE8);
			glBindTexture(GL_TEXTURE_2D, Neptune);
			glUniform1i(glGetUniformLocation(program, "TEX_NEPTUNE"), 8);
		}

		glUniform4fv(glGetUniformLocation(program, "light_position"), 1, light.position);

		// update the uniform model matrix and render
		glUniformMatrix4fv( glGetUniformLocation( program, "model_matrix" ), 1, GL_TRUE, model_matrix );
		glDrawElements( GL_TRIANGLES, tri_num * 3, GL_UNSIGNED_INT, nullptr );
	}

	for (int k = 0, kn = int(NUM_satellite); k < kn; k++)
	{
		// configure transformation parameters
		float theta1 = rotate_time;
		float loc_theta = randf(0, PI / 2.0f);

		mat4 model_matrix;
		// build the model matrix
		satellite.revolution_time[k] += satellite.revolution[k];

		model_matrix = mat4::rotate(vec3(0, 1, 0), sphere.revolution_time[satellite.parent[k]]) *
			mat4::translate(vec3(sphere.distance[satellite.parent[k]], 0, 0.0f)) *
			mat4::rotate(vec3(0, 1, 0), satellite.revolution_time[k]) *
			mat4::translate(vec3(satellite.distance[k], 0, 0.0f)) *
			mat4::scale(vec3(satellite.radius[k])) *
			mat4::rotate(vec3(0, 1, 0), theta1);

		light.position = vec4(-model_matrix[3], -model_matrix[7], -model_matrix[11], 0);

		glUniform1i(glGetUniformLocation(program, "mode"), 9);
		glActiveTexture(GL_TEXTURE9);
		glBindTexture(GL_TEXTURE_2D, Moon);
		glUniform1i(glGetUniformLocation(program, "TEX_MOON"), 9);

		// update the uniform model matrix and render
		glUniform4fv(glGetUniformLocation(program, "light_position"), 1, light.position);
		glUniformMatrix4fv(glGetUniformLocation(program, "model_matrix"), 1, GL_TRUE, model_matrix);

		glDrawElements(GL_TRIANGLES, tri_num * 3, GL_UNSIGNED_INT, nullptr);
	}

	// ring
	glBindVertexArray(vertex_array_ring);
	for (int k = 0, kn = 1; k < kn; k++)
	{
		// configure transformation parameters
		float theta1 = rotate_time;

		mat4 model_matrix;
		// build the model matrix

		model_matrix =
			mat4::rotate(vec3(0, 1, 0), sphere.revolution_time[5]) *
			mat4::translate(vec3(sphere.distance[5], 0, 0)) *
			mat4::scale(vec3(1.0f)) *
			mat4::rotate(vec3(0, 1, 0), theta1);

		light.position = vec4(0,1,0, 0);

		glUniform1i(glGetUniformLocation(program, "mode"), 10);
		glActiveTexture(GL_TEXTURE10);
		glBindTexture(GL_TEXTURE_2D, Saturnring);
		glUniform1i(glGetUniformLocation(program, "TEX_SATURNRING"), 10);

		// update the uniform model matrix and render
		glUniform4fv(glGetUniformLocation(program, "light_position"), 1, light.position);
		glUniformMatrix4fv(glGetUniformLocation(program, "model_matrix"), 1, GL_TRUE, model_matrix);

		glDrawElements(GL_TRIANGLES, NUM_TESS * 6, GL_UNSIGNED_INT, nullptr);
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
		}
		else if (key == GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_MINUS)
		{
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
		else if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL)
			p_ctrl = 1;
		else if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)
			p_shift = 1;
#ifndef GL_ES_VERSION_2_0
		else if (key == GLFW_KEY_W)
		{
			b_wireframe = !b_wireframe;
			glPolygonMode(GL_FRONT_AND_BACK, b_wireframe ? GL_LINE : GL_FILL);
			printf("> using %s mode\n", b_wireframe ? "wireframe" : "solid");
		}
#endif
	}
	if (action == GLFW_RELEASE)
	{
		if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL)
			p_ctrl = 0;
		else if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)
			p_shift = 0;
	}
}

void mouse(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action == GLFW_PRESS)
		{
			right_press = 1;
			glfwGetCursorPos(window, &pos.x, &pos.y);
		}
		else if (action == GLFW_RELEASE)right_press = 0;
	}
	if (button == GLFW_MOUSE_BUTTON_MIDDLE)
	{
		if (action == GLFW_PRESS)
		{
			middle_press = 1;
			glfwGetCursorPos(window, &pos.x, &pos.y);
		}
		else if (action == GLFW_RELEASE) middle_press = 0;
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS)
		{
			press = 1;
			glfwGetCursorPos(window, &pos.x, &pos.y);
		}
		else if (action == GLFW_RELEASE)	press = 0;
		vec2 npos = cursor_to_ndc(pos, window_size);
		if (action == GLFW_PRESS)			tb.begin(cam.view_matrix, npos);
		else if (action == GLFW_RELEASE)	tb.end();
	}
}

void motion(GLFWwindow* window, double x, double y)
{
	later_pos.x = x;
	later_pos.y = y;
	if (!right_press && !middle_press && !p_shift && !p_ctrl)
	{
		vec2 npos = cursor_to_ndc(dvec2(x, y), window_size);
		if (!tb.is_tracking()) return;
		cam.view_matrix = tb.update(npos);
	}
}

std::vector<vertex> create_ring_vertices(uint N)
{
	float sp = 0.00001f;
	std::vector<vertex> v = { { vec3(0,-sp,0), vec3(0,1,0), vec2(-sphere.radius[5] / (1 - sphere.radius[5]),0) } }; // origin
	v.push_back({ vec3(0,sp,0), vec3(0,1,0), vec2(-sphere.radius[5]/(1-sphere.radius[5]),0) });
	for (uint k = 0; k <= N; k++)
	{
		float t = PI * 2.0f * k / float(N), c = cos(t), s = sin(t);
		v.push_back({ vec3(c,0,s), vec3(0,1,0), vec2(1,s)});
	}

	return v;
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
			v.push_back({ vec3(st * sp, ct, st * cp), vec3(st * sp,ct,st * cp) , vec2(py/ (2 * PI),1 - theta / PI) });
		}
	}
	return v;
}

void update_vertex_buffer_ring(const std::vector<vertex>& vertices, uint N)
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
	for (uint k = 1; k < N+1; k++)
	{
		indices.push_back(0);	// the origin
		indices.push_back(k + 1);
		indices.push_back(k + 2);
	}
	for (uint k = 1; k < N + 1; k++)
	{
		indices.push_back(1);	// the origin
		indices.push_back(k + 2);
		indices.push_back(k + 1);
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
	if (vertex_array_ring) glDeleteVertexArrays(1, &vertex_array_ring);
	vertex_array_ring = cg_create_vertex_array(vertex_buffer, index_buffer);
	if (!vertex_array_ring) { printf("%s(): failed to create vertex aray\n", __func__); return; }
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
	int i;
	for (i = 0; i <= 7; i++)
		sphere.revolution_time[i] = randf(0, PI * 2);
	// log hotkeys
	print_help();

	// init GL states
	glLineWidth(1.0f);
	glClearColor( 39/255.0f, 40/255.0f, 34/255.0f, 1.0f );	// set clear color
	glEnable( GL_CULL_FACE );								// turn on backface culling
	glEnable( GL_TEXTURE_2D );			// enable texturing
	glEnable( GL_DEPTH_TEST );			// turn on depth tests
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glActiveTexture( GL_TEXTURE0 );		// notify GL the current texture slot is 0
	glActiveTexture( GL_TEXTURE1 );		// notify GL the current texture slot is 1
	glActiveTexture( GL_TEXTURE2 );		// notify GL the current texture slot is 2
	glActiveTexture( GL_TEXTURE3 );		// notify GL the current texture slot is 3
	glActiveTexture( GL_TEXTURE4 );		// notify GL the current texture slot is 4
	glActiveTexture( GL_TEXTURE5 );		// notify GL the current texture slot is 5
	glActiveTexture( GL_TEXTURE6 );		// notify GL the current texture slot is 6
	glActiveTexture( GL_TEXTURE7 );		// notify GL the current texture slot is 7
	glActiveTexture( GL_TEXTURE8 );		// notify GL the current texture slot is 8
	glActiveTexture( GL_TEXTURE9 );		// notify GL the current texture slot is 9
	glActiveTexture( GL_TEXTURE10 );

	// define the position of four corner vertices
	unit_sphere_vertices = std::move(create_sphere_vertices(NUM_TESS));
	// create vertex buffer; called again when index buffering mode is toggled
	update_vertex_buffer(unit_sphere_vertices, NUM_TESS);

	unit_ring_vertices = std::move(create_ring_vertices(NUM_TESS));
	update_vertex_buffer_ring(unit_ring_vertices, NUM_TESS);

	Sun = cg_create_texture(sun_image_path, true);				if (!Sun) return false;
	Mercury = cg_create_texture(mercury_image_path, true);		if (!Mercury) return false;
	Venus = cg_create_texture(venus_image_path, true);			if (!Venus) return false;
	Earth = cg_create_texture(earth_image_path, true);			if (!Earth) return false;
	Mars = cg_create_texture(mars_image_path, true);			if (!Mars) return false;
	Jupiter = cg_create_texture(jupiter_image_path, true);		if (!Jupiter) return false;
	Saturn = cg_create_texture(saturn_image_path, true);		if (!Saturn) return false;
	Uranus = cg_create_texture(uranus_image_path, true);		if (!Uranus) return false;
	Neptune = cg_create_texture(neptune_image_path, true);		if (!Neptune) return false;
	Moon = cg_create_texture(moon_image_path, true);			if (!Moon) return false;
	Saturnring = cg_create_texture(saturnring_image_path, true);	if (!Saturnring) return false;

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
