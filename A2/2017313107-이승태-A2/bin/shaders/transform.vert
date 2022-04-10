// vertex attributes
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 texcoord;

// matrices
uniform mat4 model_matrix;
uniform mat4 view_matrix;
uniform mat4 projection_matrix;
uniform int gra;

out vec3 gracolor;
out vec3 norm;

void main()
{
	if(gra == 0)		gracolor = vec3(texcoord.xy, 0.0f);
	else if(gra == 1)	gracolor = vec3(texcoord.xxx);
	else if(gra == 2)	gracolor = vec3(texcoord.yyy);
	else if(gra == 3)	gracolor = vec3(texcoord.xxy);
	else if(gra == 4)	gracolor = vec3(texcoord.xyx);
	else if(gra == 5)	gracolor = vec3(texcoord.yxx);
	else if(gra == 6)	gracolor = vec3(texcoord.xyy);
	else if(gra == 7)	gracolor = vec3(texcoord.yyx);
	else				gracolor = vec3(texcoord.yxy);



	vec4 wpos = model_matrix * vec4(position,1);
	vec4 epos = view_matrix * wpos;
	gl_Position = projection_matrix * epos;

	norm = normalize(mat3(view_matrix*model_matrix)*normal);

	// pass eye-coordinate normal to fragment shader
}
