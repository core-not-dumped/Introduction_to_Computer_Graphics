#pragma once
#ifndef __CIRCLE_H__
#define __CIRCLE_H__

static const uint	MIN_CIRCLE_NUM = 10;	// minimum tessellation factor (down to a triangle)
static const uint	MAX_CIRCLE_NUM = 150;			// maximum tessellation factor (up to 256 triangles
static const float	HIGH_SPEED = 0.001f;
static const float	LOW_SPEED = 0.0003f;
static const float	MAX_RADIUS = 1.0f;
static const float	MIN_RADIUS = 0.5f;
uint				CIRCLE_NUM = 20;		// circle_number

struct circle_t
{
	vec2	center=vec2(0);		// 2D position for translation
	float	radius=1.0f;		// radius
	float	theta=0.0f;			// rotation angle
	vec4	color=vec4(0);		// RGBA color in [0,1]
	vec2	speed = vec2(0);		// CIRCLE speed
	uint	circle_num = MAX_CIRCLE_NUM + 1;
	uint	detect = MAX_CIRCLE_NUM + 1;
	vec4	color_tmp = vec4(0);
	mat4	model_matrix;		// modeling transformation

	// public functions
	void	update( float t );
};

inline float vector_size(vec2 vector_a)
{
	return sqrt(vector_a.x * vector_a.x + vector_a.y * vector_a.y);
}

inline float randf(float m =0, float M = 1.0f)
{
	float r = rand() / float(RAND_MAX);
	return r * (M - m) + m;
}

inline float dis(vec2 p1, vec2 p2)
{
	return sqrt((p1.x - p2.x)* (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
}

inline float dis_sqare(vec2 p1, vec2 p2)
{
	return (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
}

inline std::vector<circle_t> create_circles()
{
	std::vector<circle_t> circles;

	//define center and radius

	//init avoid collision
	std::vector<vec2> tmp_center;
	std::vector<float> tmp_radius;
	for (uint i = 0; i < CIRCLE_NUM; i++)
	{
		vec2 rand_center;
		float rand_radius;
		while(1)
		{
			float factor = 1 / sqrt(float(CIRCLE_NUM));
			rand_radius = randf(MAX_RADIUS*factor, MIN_RADIUS*factor);
			rand_center.x = randf(-16.0f/9.0f, 16.0f/9.0f);
			if ((rand_center.x - rand_radius < -16.0f/9.0f) || (rand_center.x + rand_radius > 16.0f/9.0f))	continue;
			rand_center.y = randf(-1.0f, 1.0f);
			if ((rand_center.y - rand_radius < -1.0f) || (rand_center.y + rand_radius > 1.0f))	continue;

			uint j;
			for (j = 0;j<i; j++)
			{
				if (dis(tmp_center[j], rand_center) < rand_radius + tmp_radius[j])	break;
			}
			if (j == i) {
				tmp_center.push_back(rand_center);
				tmp_radius.push_back(rand_radius);
				break;
			}
		}
	}

	for (uint k = 0; k < CIRCLE_NUM; k++)
	{
		circle_t c;
		c = { vec2(-0.5f,0),1.0f,0.0f,vec4(1.0f,0.5f,0.5f,1.0f),vec2(0,0),0,0,vec4(0)};
		c.center.x = tmp_center[k].x;
		c.center.y = tmp_center[k].y;
		c.radius = tmp_radius[k];
		c.color.r = randf();
		c.color.g = randf();
		c.color.b = randf();
		c.color_tmp.r = c.color.r;
		c.color_tmp.g = c.color.g;
		c.color_tmp.b = c.color.b;
		do{
			c.speed.x = randf(-HIGH_SPEED, HIGH_SPEED);
		} while (c.speed.x < -LOW_SPEED && c.speed.x > LOW_SPEED);
		do {
			c.speed.y = randf(-HIGH_SPEED, HIGH_SPEED);
		} while (c.speed.y < -LOW_SPEED && c.speed.y > LOW_SPEED);
		c.circle_num = k;

		circles.emplace_back(c);
	}
	return circles;
}

inline void circle_t::update( float t )
{
	// radius	= 0.35f+cos(t)*0.1f;		// simple animation
	// theta	= t;
	// float c	= cos(theta), s=sin(theta);

	// these transformations will be explained in later transformation lecture
	mat4 scale_matrix =
	{
		radius, 0, 0, 0,
		0, radius, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	mat4 translate_matrix =
	{
		1, 0, 0, center.x,
		0, 1, 0, center.y,
		0, 0, 1, 0,
		0, 0, 0, 1
	};
	
	model_matrix = translate_matrix*scale_matrix;
}

#endif
