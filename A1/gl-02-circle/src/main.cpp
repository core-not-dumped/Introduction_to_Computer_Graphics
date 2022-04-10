#include "cgmath.h"		// slee's simple math library
#include "cgut.h"		// slee's OpenGL utility
#include "circle.h"		// circle class definition

//*************************************
// global constants
static const char*	window_name = "cgbase - circle";
static const char*	vert_shader_path = "../bin/shaders/circ.vert";
static const char*	frag_shader_path = "../bin/shaders/circ.frag";
uint				NUM_TESS = 32;			// initial tessellation factor of the circle as a polygon

//*************************************
// window objects
GLFWwindow*	window = nullptr;
//ivec2		window_size = cg_default_window_size();
ivec2		window_size = ivec2(1280, 720); // initial window size

//*************************************
// OpenGL objects
GLuint	program = 0;		// ID holder for GPU program
GLuint	vertex_array = 0;	// ID holder for vertex array object

//*************************************
// global variables
int		frame = 0;						// index of rendering frames
float	t = 0.0f;						// current simulation parameter
bool	b_solid_color = true;			// use circle's color?
bool	b_index_buffer = true;			// use index buffering?

// color
bool	b_red_color = false;
bool	b_green_color = false;
bool	b_blue_color = false;
bool	b_shuffle_color = false;
bool	b_main_color = false;

//mouse
dvec2 pos_left;						// left mouse postion
dvec2 pos_right;					// right mouse postion
dvec2 pos;
float CHANGE_LEFT_RATIO = 0.5f;		// * circle radius
float CHANGE_RIGHT_RATIO = 1.5f;	// * circle radius

// gravity
int		b_gravity = 0;
float	gravity_x[5] = { 0.0f, 0.0f, 0.000001f, 0.0f, -0.000001f };		// gravity_apply
float	gravity_y[5] = { 0.0f, -0.000001f, 0.0f, 0.000001f, 0.0f };		// gravity_apply

float speed_limit_for_wall = 0.00005f;		// smallest speed
float reflect_e_for_wall = 0.8f;			// Coefficient of restitution for wall

// ball_size
bool	CONTROL_PRESS = false;
bool	big_ball = false;
bool	small_ball = false;

// ball_speed
bool	SHIFT_PRESS = false;
bool	fast_speed = false;

// mouse_gravity
bool	MOUSE_GRAVITY = false;
float	mouse_gravity_size = 0.000001f;
float	speed_limit_for_ball = 0.0006f;
float	reflect_e_for_ball = 0.95f;

int		circle_update_flag = 0;
#ifndef GL_ES_VERSION_2_0
bool	b_wireframe = false;
#endif
auto	circles = std::move(create_circles());
struct { bool add=false, sub=false; operator bool() const { return add||sub; } } b; // flags of keys for smooth changes

//*************************************
// holder of vertices and indices of a unit circle
std::vector<vertex>	unit_circle_vertices;	// host-side vertices

//*************************************
void reset()
{
	circles = std::move(create_circles());
}

vec2 mouse_position2screen(dvec2 mouse_)
{
	vec2 mouse;
	mouse.x = float(mouse_.x);
	mouse.y = float(mouse_.y);

	vec2 mouse_on_screen;
	vec2 w;
	w.x = float(window_size.x) / 2.0f;
	w.y = float(window_size.y) / 2.0f;
	if (window_size.x * 9.0f / 16.0f > window_size.y)
	{
		mouse_on_screen.x = (mouse.x - w.x) / w.y;
		mouse_on_screen.y = -(mouse.y - w.y) / w.y;
	}
	else
	{
		mouse_on_screen.x = (mouse.x - w.x) / w.x * 16.0f / 9.0f;
		mouse_on_screen.y = -(mouse.y - w.y) / w.x * 16.0f / 9.0f;
	}
	return mouse_on_screen;
}

//*************************************
void update()
{
	// update global simulation parameter
	t = float(glfwGetTime())*0.4f;

	// tricky aspect correction matrix for non-square window
	float aspect = float(window_size.x)/float(window_size.y);

	float x, y;
	if (aspect >= 16.0f/9.0f)
	{
		x = std::min(1 / aspect, 1.0f);
		y = std::min(aspect, 1.0f);
	}
	else
	{
		x = 9.0f / 16.0f;
		y = (9.0f / 16.0f) * aspect;
	}

	mat4 aspect_matrix = 
	{
		x, 0, 0, 0,
		0, y, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	// update common uniform variables in vertex/fragment shaders
	GLint uloc;
	uloc = glGetUniformLocation( program, "b_solid_color" );	if(uloc>-1) glUniform1i( uloc, b_solid_color );
	uloc = glGetUniformLocation( program, "aspect_matrix" );	if(uloc>-1) glUniformMatrix4fv( uloc, 1, GL_TRUE, aspect_matrix );

	// update vertex buffer by the pressed keys
	void update_circle_num(); // forward declaration
	if (circle_update_flag == 100)
	{
		if (b) update_circle_num();
		circle_update_flag = 0;
	}
	else	circle_update_flag += 1;
}

void render()
{
	// clear screen (with background color) and clear depth buffer
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// notify GL that we use our own program
	glUseProgram( program );

	// bind vertex array object
	glBindVertexArray( vertex_array );

	vec2 screen_on_left_mouse = mouse_position2screen(pos_left);
	vec2 screen_on_right_mouse = mouse_position2screen(pos_right);

	// render two circles: trigger shader program to process vertex data
	for( auto& c : circles )
	{
		// per-circle update

		//mouse screen
		if (small_ball && dis(c.center, screen_on_left_mouse) < c.radius)
			c.radius *= CHANGE_LEFT_RATIO;
		else if (big_ball && dis(c.center, screen_on_right_mouse) < c.radius)
			c.radius *= CHANGE_RIGHT_RATIO;
		else if (fast_speed && dis(c.center, screen_on_left_mouse) < c.radius)
		{
			float speed_factor = 1.0f;
			c.speed *= 2;
		}

		// mouse gravity
		if (MOUSE_GRAVITY == true)
		{
			vec2 screenmouse = mouse_position2screen(pos);
			vec2 mouse_gravity = screenmouse - c.center;
			float normalize_factor = vector_size(mouse_gravity);
			mouse_gravity = mouse_gravity / normalize_factor * mouse_gravity_size;
			c.speed.x += mouse_gravity.x;
			c.speed.y += mouse_gravity.y;
		}

		//gravity
		if (b_gravity == 5)
		{
			float speed_factor = 1.0f;
			do {
				c.speed.x = randf(-HIGH_SPEED * speed_factor, HIGH_SPEED * speed_factor);
			} while (c.speed.x < -LOW_SPEED * speed_factor && c.speed.x > LOW_SPEED * speed_factor);
			do {
				c.speed.y = randf(-HIGH_SPEED * speed_factor, HIGH_SPEED * speed_factor);
			} while (c.speed.y < -LOW_SPEED * speed_factor && c.speed.y > LOW_SPEED * speed_factor);
		}
		else
		{
			c.speed.x += gravity_x[b_gravity];
			c.speed.y += gravity_y[b_gravity];
		}

		// color update
		if (b_red_color)	{ c.color.r = c.color_tmp.r; c.color.g = 0; c.color.b = 0; }
		if (b_green_color)	{ c.color.r = 0; c.color.g = c.color_tmp.g; c.color.b = 0; }
		if (b_blue_color)	{ c.color.r = 0; c.color.g = 0; c.color.b = c.color_tmp.b; }
		if (b_shuffle_color)
		{
			c.color.r = abs(sin(c.color_tmp.r*4.2345f*t));
			c.color.g = abs(sin(c.color_tmp.g*5.4523f*t));
			c.color.b = abs(sin(c.color_tmp.b*6.9058f*t));
		}
		if (b_main_color) { c.color = c.color_tmp; }

		// circle collision
		int collision_flag = 0;
		for (uint i = 0;i<CIRCLE_NUM; i++)
		{
			if (dis(c.center, circles[i].center) != 0)
			{
				if (dis(c.center, circles[i].center) < c.radius + circles[i].radius)
				{
					vec2 v1_v2 = c.speed - circles[i].speed;
					vec2 x1_x2 = c.center - circles[i].center;
					
					// collision detect
					if (dot(c.speed, x1_x2) > 0 && dot(circles[i].speed, -x1_x2) > 0)	continue;

					if (i == c.detect)	continue;
					collision_flag = 1;
					circles[i].detect = c.circle_num;
					c.detect = i;

					// pysics for two circle collision
					float m1 = c.radius * c.radius;
					float m2 = circles[i].radius * circles[i].radius;
					float m1_tmp = 2 * m2 / (m1 + m2);
					float m2_tmp = 2 * m1 / (m1 + m2);
					float Dis_sqare = dis_sqare(c.center, circles[i].center);
					float pro = dot(v1_v2, x1_x2);
					c.speed = c.speed - m1_tmp * pro / Dis_sqare * x1_x2;
					circles[i].speed = circles[i].speed + m2_tmp * pro / Dis_sqare * x1_x2;
					if (MOUSE_GRAVITY && (vector_size(c.speed) > speed_limit_for_ball || vector_size(c.speed.x) < -speed_limit_for_ball))
					{
						c.speed *= reflect_e_for_ball;
						circles[i].speed *= reflect_e_for_ball;
					}
				}
			}
		}
		if (!collision_flag)	c.detect = 101;

		// window collision
		if (c.center.x + c.radius > 16.0f / 9.0f && c.speed.x > 0)
		{
			if (b_gravity == 2 && (c.speed.x > speed_limit_for_wall || c.speed.x < -speed_limit_for_wall)) c.speed.x = reflect_e_for_wall * c.speed.x;
			c.speed.x = -c.speed.x;
		}
		else if (c.center.x - c.radius < -16.0f / 9.0f && c.speed.x < 0)
		{
			if (b_gravity == 4 && (c.speed.x > speed_limit_for_wall || c.speed.x < -speed_limit_for_wall)) c.speed.x = reflect_e_for_wall * c.speed.x;
			c.speed.x = -c.speed.x;
		}
		if (c.center.y + c.radius > 1 && c.speed.y > 0)
		{
			if (b_gravity == 3 && (c.speed.y > speed_limit_for_wall || c.speed.y < -speed_limit_for_wall))	c.speed.y = reflect_e_for_wall * c.speed.y;
			c.speed.y = -c.speed.y;
		}
		else if (c.center.y - c.radius < -1 && c.speed.y < 0)
		{
			if (b_gravity == 1 && (c.speed.y > speed_limit_for_wall || c.speed.y < -speed_limit_for_wall))	c.speed.y = reflect_e_for_wall * c.speed.y;
			c.speed.y = -c.speed.y;
		}

		// center change
		c.center.x += c.speed.x;
		c.center.y += c.speed.y;

		c.update(t);

		// update per-circle uniforms
		GLint uloc;
		uloc = glGetUniformLocation( program, "solid_color" );		if(uloc>-1) glUniform4fv( uloc, 1, c.color );	// pointer version
		uloc = glGetUniformLocation( program, "model_matrix" );		if(uloc>-1) glUniformMatrix4fv( uloc, 1, GL_TRUE, c.model_matrix );

		// per-circle draw calls
		if(b_index_buffer)	glDrawElements( GL_TRIANGLES, NUM_TESS*3, GL_UNSIGNED_INT, nullptr );
		else				glDrawArrays( GL_TRIANGLES, 0, NUM_TESS*3 ); // NUM_TESS = N
	}

	// swap front and back buffers, and display to screen
	glfwSwapBuffers( window );

	if (b_gravity == 5)	b_gravity = 0;
	fast_speed = false;
	big_ball = false;
	small_ball = false;
	b_main_color = false;
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
	printf( "- press '+/-' to increase/decrease circle number (min=%d, max=%d)\n", MIN_CIRCLE_NUM, MAX_CIRCLE_NUM );
	printf( "- press 'i' to toggle between index buffering and simple vertex buffering\n" );
	printf("- press 'r' to reset\n\n");

	printf("- click and drag to make a mouse gravity\n\n");

	printf("- press 'shift + left_mouse' to increase speed (x2)\n");
	printf("- press 'ctrl + left_mouse' to decrease radius (RATIO: %.2f)\n", CHANGE_LEFT_RATIO);
	printf("- press 'ctrl + right_mouse' to increase radius (RATIO: %.2f)\n\n", CHANGE_RIGHT_RATIO);

	printf("- press 'z' to see red_color_circles\n");
	printf("- press 'x' to see green_color_circles\n");
	printf("- press 'c' to see blue_color_circles\n");
	printf("- press 'v' to see time-varing_color_circles\n\n");
	
	printf("gravity\n");
	printf("- press 'w' to apply up-gravity\n");
	printf("- press 'a' to apply left-gravity\n");
	printf("- press 's' to apply down-gravity\n");
	printf("- press 'd' to apply right-gravity\n");

#ifndef GL_ES_VERSION_2_0
#endif
	printf( "\n" );
}

std::vector<vertex> create_circle_vertices( uint N )
{
	std::vector<vertex> v = {{ vec3(0), vec3(0,0,-1.0f), vec2(0.5f) }}; // origin
	for( uint k=0; k <= N; k++ )
	{
		float t=PI*2.0f*k/float(N), c=cos(t), s=sin(t);
		v.push_back( { vec3(c,s,0), vec3(0,0,-1.0f), vec2(c,s)*0.5f+0.5f } );
	}
	return v;
}

void update_vertex_buffer( const std::vector<vertex>& vertices, uint N )
{
	static GLuint vertex_buffer = 0;	// ID holder for vertex buffer
	static GLuint index_buffer = 0;		// ID holder for index buffer

	// clear and create new buffers
	if(vertex_buffer)	glDeleteBuffers( 1, &vertex_buffer );	vertex_buffer = 0;
	if(index_buffer)	glDeleteBuffers( 1, &index_buffer );	index_buffer = 0;

	// check exceptions
	if(vertices.empty()){ printf("[error] vertices is empty.\n"); return; }

	// create buffers
	if(b_index_buffer)
	{
		std::vector<uint> indices;
		for( uint k=0; k < N; k++ )
		{
			indices.push_back(0);	// the origin
			indices.push_back(k+1);
			indices.push_back(k+2);
		}

		// generation of vertex buffer: use vertices as it is
		glGenBuffers( 1, &vertex_buffer );
		glBindBuffer( GL_ARRAY_BUFFER, vertex_buffer );
		glBufferData( GL_ARRAY_BUFFER, sizeof(vertex)*vertices.size(), &vertices[0], GL_STATIC_DRAW);

		// geneation of index buffer
		glGenBuffers( 1, &index_buffer );
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, index_buffer );
		glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(uint)*indices.size(), &indices[0], GL_STATIC_DRAW );
	}
	else
	{
		std::vector<vertex> v; // triangle vertices
		for( uint k=0; k < N; k++ )
		{
			v.push_back(vertices.front());	// the origin
			v.push_back(vertices[k+1]);
			v.push_back(vertices[k+2]);
		}

		// generation of vertex buffer: use triangle_vertices instead of vertices
		glGenBuffers( 1, &vertex_buffer );
		glBindBuffer( GL_ARRAY_BUFFER, vertex_buffer );
		glBufferData( GL_ARRAY_BUFFER, sizeof(vertex)*v.size(), &v[0], GL_STATIC_DRAW );
	}

	// generate vertex array object, which is mandatory for OpenGL 3.3 and higher
	if(vertex_array) glDeleteVertexArrays(1,&vertex_array);
	vertex_array = cg_create_vertex_array( vertex_buffer, index_buffer );
	if(!vertex_array){ printf("%s(): failed to create vertex aray\n",__func__); return; }
}

void update_circle_num()
{
	uint n = CIRCLE_NUM; if(b.add) n++; if(b.sub) n--;
	if(n==CIRCLE_NUM||n<MIN_CIRCLE_NUM||n>MAX_CIRCLE_NUM) return;
	CIRCLE_NUM = n;

	reset();
	printf( "> CIRCLE_NUM = % -4d\r", CIRCLE_NUM );
}

void keyboard( GLFWwindow* window, int key, int scancode, int action, int mods )
{
	if(action==GLFW_PRESS)
	{
		if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q)	glfwSetWindowShouldClose(window, GL_TRUE);
		else if(key==GLFW_KEY_H||key==GLFW_KEY_F1)	print_help();

		// reset
		else if (key == GLFW_KEY_R)	reset();

		// circle num increase
		else if (key == GLFW_KEY_KP_ADD || (key == GLFW_KEY_EQUAL && (mods & GLFW_MOD_SHIFT))){b.add = true;}
		// circle num decrease
		else if (key == GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_MINUS){	b.sub = true; }
		
		// gravity
		else if (key == GLFW_KEY_S) { if (b_gravity == 1) { b_gravity = 5; printf("Random move\n"); } else { b_gravity = 1; printf("Apply down-gravity\n"); } }
		else if (key == GLFW_KEY_D) { if (b_gravity == 2) { b_gravity = 5; printf("Random move\n"); } else { b_gravity = 2; printf("Apply right-gravity\n"); } }
		else if (key == GLFW_KEY_W) { if (b_gravity == 3) { b_gravity = 5; printf("Random move\n"); } else { b_gravity = 3; printf("Apply up-gravity\n"); } }
		else if (key == GLFW_KEY_A) { if (b_gravity == 4) { b_gravity = 5; printf("Random move\n"); } else { b_gravity = 4; printf("Apply left-gravity\n"); } }

		// color change
		else if (key == GLFW_KEY_Z)
		{
			b_red_color = !b_red_color;
			if (!b_red_color) {
				printf("Back to original color\n");
				b_main_color = true;
			}else printf("Red color\n");

			b_green_color = false;	b_blue_color = false;	b_shuffle_color = false;
		}
		else if (key == GLFW_KEY_X)
		{
			b_green_color = !b_green_color;
			if (!b_green_color) {
				printf("Back to original color\n");
				b_main_color = true;
			}
			else printf("Green color\n");

			b_red_color = false;	b_blue_color = false;	b_shuffle_color = false;
		}
		else if (key == GLFW_KEY_C)
		{ 
			b_blue_color = !b_blue_color;
			if (!b_blue_color) {
				printf("Back to original color\n");
				b_main_color = true;
			}
			else printf("Blue color\n");

			b_red_color = false;	b_green_color = false;	b_shuffle_color = false;
		}
		else if (key == GLFW_KEY_V)
		{ 
			b_shuffle_color = !b_shuffle_color;
			if (!b_shuffle_color) {
				printf("Back to original color\n");
				b_main_color = true;
			}
			else printf("shuffle color\n");
			
			b_red_color = false;	b_green_color = false;	b_blue_color = false;
		}

		// change buffer
		else if(key==GLFW_KEY_I)
		{
			b_index_buffer = !b_index_buffer;
			update_vertex_buffer( unit_circle_vertices,NUM_TESS );
			printf( "> using %s buffering\n", b_index_buffer?"index":"vertex" );
		}

		else if (key == GLFW_KEY_LEFT_CONTROL) { CONTROL_PRESS = true; }
		else if (key == GLFW_KEY_LEFT_SHIFT) { SHIFT_PRESS = true; }
#ifndef GL_ES_VERSION_2_0
#endif
	}
	else if(action==GLFW_RELEASE)
	{
		if(key==GLFW_KEY_KP_ADD||(key==GLFW_KEY_EQUAL&&(mods&GLFW_MOD_SHIFT)))	b.add = false;
		else if(key==GLFW_KEY_KP_SUBTRACT||key==GLFW_KEY_MINUS) b.sub = false;
		else if (key == GLFW_KEY_LEFT_CONTROL) { CONTROL_PRESS = false; }
		else if (key == GLFW_KEY_LEFT_SHIFT) { SHIFT_PRESS = false; }
	}
}

void mouse( GLFWwindow* window, int button, int action, int mods )
{
	if(CONTROL_PRESS && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		glfwGetCursorPos(window, &pos_left.x, &pos_left.y);
		printf("> Ctrl + Left mouse button pressed at (%d, %d)\n", int(pos_left.x), int(pos_left.y));
		small_ball = true;
	}
	else if (CONTROL_PRESS && button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		glfwGetCursorPos(window, &pos_right.x, &pos_right.y);
		printf("> Ctrl + Right mouse button pressed at (%d, %d)\n", int(pos_right.x), int(pos_right.y));
		big_ball = true;
	}
	else if (SHIFT_PRESS && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		glfwGetCursorPos(window, &pos_left.x, &pos_left.y);
		printf("> SHIFT + Right mouse button pressed at (%d, %d)\n", int(pos_left.x), int(pos_left.y));
		fast_speed = true;
	}

	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) { MOUSE_GRAVITY = true; }
	else { MOUSE_GRAVITY = false; }
}

void motion( GLFWwindow* window, double x, double y )
{
	if (MOUSE_GRAVITY)
	{
		glfwGetCursorPos(window, &pos.x, &pos.y);
		printf("left mouse cursor %4.f %4.f\r", pos.x, pos.y);
	}
}

bool user_init()
{
	// log hotkeys
	print_help();

	// init GL states
	glLineWidth( 1.0f );
	glClearColor( 39/255.0f, 40/255.0f, 34/255.0f, 1.0f );	// set clear color
	glEnable( GL_CULL_FACE );								// turn on backface culling
	glEnable( GL_DEPTH_TEST );								// turn on depth tests
	
	// define the position of four corner vertices
	unit_circle_vertices = std::move(create_circle_vertices( NUM_TESS ));

	// create vertex buffer; called again when index buffering mode is toggled
	update_vertex_buffer( unit_circle_vertices, NUM_TESS );

	return true;
}

void user_finalize()
{
}

int main( int argc, char* argv[] )
{
	// create window and initialize OpenGL extensions
	if(!(window = cg_create_window( window_name, window_size.x, window_size.y ))){ glfwTerminate(); return 1; }
	if(!cg_init_extensions( window )){ glfwTerminate(); return 1; }	// init OpenGL extensions

	// initializations and validations of GLSL program
	if(!(program=cg_create_program( vert_shader_path, frag_shader_path ))){ glfwTerminate(); return 1; }	// create and compile shaders/program
	if(!user_init()){ printf( "Failed to user_init()\n" ); glfwTerminate(); return 1; }					// user initialization

	// register event callbacks
	glfwSetWindowSizeCallback( window, reshape );	// callback for window resizing events
    glfwSetKeyCallback( window, keyboard );			// callback for keyboard events
	glfwSetMouseButtonCallback( window, mouse );	// callback for mouse click inputs
	glfwSetCursorPosCallback( window, motion );		// callback for mouse movements

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
