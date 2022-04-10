#ifdef GL_ES
	#ifndef GL_FRAGMENT_PRECISION_HIGH	// highp may not be defined
		#define highp mediump
	#endif
	precision highp float; // default precision needs to be defined
#endif

// input from vertex shader
in vec4 epos;
in vec3 norm;
in vec2 tc;

// the only output variable
out vec4 fragColor;

uniform mat4	view_matrix;
uniform float	shininess;
uniform vec4	light_position, Ia, Id, Is;	// light
uniform vec4	Ka, Kd, Ks;

uniform sampler2D	TEX_SUN;
uniform sampler2D	TEX_MERCURY;
uniform sampler2D	TEX_VENUS;
uniform sampler2D	TEX_EARTH;
uniform sampler2D	TEX_MARS;
uniform sampler2D	TEX_JUPITER;
uniform sampler2D	TEX_SATURN;
uniform sampler2D	TEX_URANUS;
uniform sampler2D	TEX_NEPTUNE;
uniform sampler2D	TEX_MOON;
uniform sampler2D	TEX_SATURNRING;
uniform int mode;

vec4 phong( vec3 l, vec3 n, vec3 h, vec4 Kd )
{
	vec4 Ira = Ka*Ia;									// ambient reflection
	vec4 Ird = max(Kd*dot(l,n)*Id,0.0);					// diffuse reflection
	vec4 Irs = max(Ks*pow(dot(h,n),shininess)*Is,0.0);	// specular reflection
	return Ira + Ird + Irs;
}

void main()
{
	if (mode == 0)	fragColor = texture( TEX_SUN, tc);
	else if(mode == 10)
	{
		fragColor = texture( TEX_SATURNRING, tc) * vec4(1.0f,1.0f,1.0f,0.3f);
	}
	else
	{
		// light position in the eye space
		vec4 lpos = view_matrix*light_position;

		vec3 n = normalize(norm);	// norm interpolated via rasterizer should be normalized again here
		vec3 p = epos.xyz;			// 3D position of this fragment
		vec3 l = normalize(lpos.xyz - (lpos.a==0.0 ? vec3(0) : p));	// lpos.a==0 means directional light
		vec3 v = normalize(-p);		// eye-epos = vec3(0)-epos
		vec3 h = normalize(l+v);	// the halfway vector

		vec4 iKd;
		if(mode==1)			iKd = texture( TEX_MERCURY, tc);
		else if(mode==2)	iKd = texture( TEX_VENUS, tc);
		else if(mode==3)	iKd = texture( TEX_EARTH, tc);
		else if(mode==4)	iKd = texture( TEX_MARS, tc);
		else if(mode==5)	iKd = texture( TEX_JUPITER, tc);
		else if(mode==6)	iKd = texture( TEX_SATURN, tc);
		else if(mode==7)	iKd = texture( TEX_URANUS, tc);
		else if(mode==8)	iKd = texture( TEX_NEPTUNE, tc);
		else if(mode==9)	iKd = texture( TEX_MOON, tc);
		
		fragColor = phong( l, n, h, iKd );
	}
}