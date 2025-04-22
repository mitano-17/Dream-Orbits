#version 330
 
in vec3 a_vertex;
in vec3 a_color;
in vec2 a_uv;
in vec3 a_normal;

out vec3 v_vertex;
out vec3 v_color;
out vec2 v_uv;
out vec3 v_normal;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

void main()
{
	// translate vertex position using u_model transform matrix: 4x4 * 4x1 matrix operation
	gl_Position = u_projection * u_view * u_model * vec4( a_vertex , 1.0 );
	// M V P - but here it's the opposite since it's multiplied right to left

	mat4 normal_matrix = transpose(inverse(u_model));
	v_normal = (normal_matrix * vec4(a_normal, 1.0)).xyz;

	v_vertex = (u_model * vec4(a_vertex, 1.0)).xyz;

	// pass the colour to the fragment shader
	v_color = a_color;

	// pass uv coords to fragment shader
	v_uv = a_uv;
}

