#version 330

in vec3 v_vertex;
in vec3 v_color;
in vec2 v_uv;
in vec3 v_normal;

out vec4 fragColor;

uniform vec3 u_color;
uniform sampler2D u_texture;
uniform sampler2D u_texture_normal;
uniform sampler2D u_texture_spec;
uniform sampler2D u_texture_night;
uniform bool u_has_multitextures;

uniform vec3 u_light;
uniform float u_light_intensity;
uniform vec3 u_cam_pos;
uniform vec3 u_ambient;
uniform vec3 u_diffuse;
uniform vec3 u_specular;
uniform float u_shininess;
uniform float u_alpha;

mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv)
{
	// get edge vectors of the pixel triangle
	vec3 dp1 = dFdx( p );
	vec3 dp2 = dFdy( p );
	vec2 duv1 = dFdx( uv );
	vec2 duv2 = dFdy( uv );
	// solve the linear system
	vec3 dp2perp = cross( dp2, N );
	vec3 dp1perp = cross( N, dp1 );
	vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
	vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
			 
	// construct a scale-invariant frame 
	float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
	return mat3( T * invmax, B * invmax, N );
}

// assume N, the interpolated vertex normal and 
// V, the view vector (vertex to eye)
vec3 perturbNormal( vec3 N, vec3 V, vec2 texcoord, vec3 normal_pixel )
{

	normal_pixel = normal_pixel * 2.0 - 1.0;
	mat3 TBN = cotangent_frame(N, V, texcoord);
	return normalize(TBN * normal_pixel);
}

void main(void)
{
	vec3 normal;
	vec3 texture_normal;
	vec3 texture_spec;
	vec3 texture_night;

	if(u_has_multitextures) {
		texture_normal = texture(u_texture_normal, v_uv).xyz;
		vec3 N = normalize(v_normal);
		vec3 N_orig = N;   // original normal
		// call function to modify normal
		N = perturbNormal(N, v_vertex, v_uv, texture_normal);
		// mix original normal with new normal
		N = mix(N_orig, N, 2.0f);

		texture_spec = texture(u_texture_spec, v_uv).xyz;
		texture_night = texture(u_texture_night, v_uv).xyz;

		normal = N;
	}

	// material color
	vec3 material = texture(u_texture, v_uv).rgb;

	// ambient
	vec3 ambient = material * u_ambient * u_light_intensity;

	// diffuse
	if(!u_has_multitextures) {
		normal = normalize(v_normal);
	}
	
	vec3 light = normalize(u_light - v_vertex);
	float n_dot_l = max(dot(normal, light), 0.0f);
	vec3 diffuse = material * n_dot_l * u_diffuse * u_light_intensity;

	// specular (phong)
	vec3 reflection = normalize(-reflect(light, normal));
	vec3 eye = normalize(u_cam_pos - v_vertex);			// view, sometimes v (r_dot_v)
	float r_dot_e = max(dot(reflection, eye), 0.0f);
	vec3 specular = material * pow(r_dot_e, u_shininess) * u_specular * u_light_intensity;

	// specular (blinn-phong)
	vec3 half_vector = normalize(light + eye);			// half-vector between light-vector and eye-vector
	float n_dot_h = max(dot(normal, half_vector), 0.0f);
	vec3 specular_blinn = material * pow(n_dot_h, u_shininess) * u_specular * u_light_intensity;

	if(!u_has_multitextures) {
		texture_spec = vec3(1.0, 1.0, 1.0);
	}
	// blinn-phong
	vec3 final_color = ambient + diffuse + specular_blinn * texture_spec;
	// replace specular_blinn with specular for phong reflectance equation

	if(n_dot_l < 0.001 && u_has_multitextures) {
		final_color = texture_night;
	}

	fragColor = vec4(final_color, u_alpha);

	// special case of alpha map and skybox
	if(u_alpha == -1.0f) {
		fragColor = texture(u_texture, v_uv);
	}

	// TEST CODES
	//fragColor = vec4(texture(u_texture, v_uv).rgb, 1.0);
	//fragColor = vec4(v_uv, 0.0f, 1.0f);

}
