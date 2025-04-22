/*  Base code by Alun Evans 2016 LaSalle (aevanss@salleurl.edu) modified by: Conrado Ruiz, Ferran Ruiz 2024*/

// student name: 
//
// Chan, Nyles
// Ermitano, Kate

//include some standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>

//include OpenGL libraries
#include <GL/glew.h>
#include <GLFW/glfw3.h>

//include some custom code files
#include "glfunctions.h"	//include all OpenGL stuff
#include "Shader.h"			// class to compile shaders

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cstdlib> 
#include <ctime>

using namespace std;
using namespace glm;

//global variables to help us do things
int g_ViewportWidth = 512; int g_ViewportHeight = 512; // Default window size, in pixels
double mouse_x, mouse_y;	//variables storing mouse position
const vec3 g_backgroundColor(0.0f, 0.0f, 0.0f); // background colour - a GLM 3-component vector

// uniform location variables
GLuint model_loc, view_loc, projection_loc, normal_loc, texture_loc, texture_normal_loc, texture_spec_loc, texture_night_loc;
GLuint light_loc, light_intensity_loc, cam_pos_loc, ambient_loc, diffuse_loc, specular_loc, shininess_loc, alpha_loc;
GLuint offset_loc, has_multi_loc;

// if using orthographic project, and if using orbital camera settings
bool orthographic = false;
bool orbital = false;
// for debugging purposes

// camera variables
glm::vec3 cameraPos = vec3(0.0f, 5.0f, 5.0f);								// Pos: position of camera
glm::vec3 cameraTarget = vec3(0.0f, 4.0f, -2.0f);							// Center: where you wanna look at in world space
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 cameraRight = glm::normalize(glm::cross(cameraTarget, cameraUp));
float cameraSpeed = 0.25f;
// would have created struct, but everything is declared here already, and they aren't used as often as other structs

// fps camera variables
bool firstMouse = true;
float camera_yaw = -90.0f; //yaw
float camera_pitch = 0.0f; //pitch
float lastX = 512.0f / 2.0f;
float lastY = 512.0f / 2.0f;
float fov = 90.0f; //field of view

GLuint g_simpleShader = 0;				// shader identifier
std::vector <GLuint> g_vao;				// vao vector
std::vector <GLuint> g_NumTriangles;	// num triangles vector

std::vector <std::string> objects;		// object vector
std::vector < std::vector < tinyobj::shape_t > > shapesVector; // shapes vector

std::vector <std::string> textures;		// textures vector
std::vector <GLuint> texture_ids;		// texture id vector

// for setting for loops and vector sizes
GLuint objCount;						// objects.size(), but to be called later
GLuint texCount;						// textures.size(), but to be called later

// light
glm::vec3 g_light(10.0f, 10.0f, 10.0f);
GLfloat light_intensity = 1.0f;

// variables to transform objects
GLfloat earth_rot = 0.0f;
GLfloat moon_rot = 0.0f;
GLfloat earthX = 0.0f;
GLfloat earthZ = 0.0f;
GLfloat ring_rot = 90.0f;
GLfloat coin_rot = 90.0f;
GLfloat hex_rot = 90.0f;
GLfloat saturn_rot = 90.0f;
GLfloat starX = 0.0f;
GLfloat starY = 3.0f;
GLfloat starZ = 0.0f;

// skybox settings
//GLuint g_simpleShader_sky = 0;			// skybox shader identifier
//GLuint g_vao_sky = 0;					// skybox vao
//GLuint g_NumTriangles_sky = 0;			// num tris skybox
//GLuint texture_id_sky = 0;				// global texture id
//GLuint model_loc_sky, view_loc_sky, projection_loc_sky;
// skybox settings no longer needed as it is stored in index 0 of vectors

// matrices for projection, view, and model
// model matrices stored in vector for mesh parenting
mat4 view_matrix, projection_matrix;
std::vector <mat4> models;

// delta time variables, for animation purposes
GLfloat currentTime = 0.0f;
GLfloat lastTime = 0.0f;
GLfloat deltaTime = 0.0f;

// particle variables
std::vector<Particle> particles;
initializeParticles(particles, 100); // Create 100 stars

struct TransformationValues {
	// struct for transformation values
	glm::vec3 translation, rotation, scale;

	// first constructor accepts 9 float values for translate's, rotate's, scale's xyz values
	TransformationValues(float tx, float ty, float tz, float rx, float ry, float rz, float sx, float sy, float sz) : translation(tx, ty, tz), rotation(rx, ry, rz), scale(sx, sy, sz) {}

	// second constructor accepts 3 vec3 values for translate, rotate, scale vec3 values
	TransformationValues(glm::vec3 t, glm::vec3 r, glm::vec3 s) : translation(t), rotation(r), scale(s) {}
};

struct MaterialProperties {
	// struct for material properties
	glm::vec3 ambient, diffuse, specular;
	GLfloat shininess, alpha;

	// constructor accepts 3 vec3 values for ambient, diffuse, specular, then 2 Glfloat values for shininess and alpha
	MaterialProperties(glm::vec3 ambi, glm::vec3 diff, glm::vec3 spec, GLfloat shiny, GLfloat transparency) : ambient(ambi), diffuse(diff), specular(spec), shininess(shiny), alpha(transparency) {}
};

// mat4 model_parent, TransformationValues transform, MaterialProperties material

struct Mesh {
	// struct for mesh
	TransformationValues transform;
	MaterialProperties material;
	GLuint object_index;
	GLuint texture_index;
	mat4 model_matrix;
	string mesh_name;

	// constructor 
	Mesh(string name, GLuint object_i, GLuint texture_i, TransformationValues t_values, MaterialProperties m_props) : mesh_name(name), object_index(object_i), texture_index(texture_i), transform(t_values), material(m_props) {}
};

std::vector <Mesh> meshes;

struct Particle {
	glm::vec3 position;   // 3D position of the star
	glm::vec3 velocity;   // Velocity of the star (falling motion)
	float lifetime;       // Time until the star disappears
	float size;           // Size of the star
	glm::vec4 color;      // Color (RGBA) of the star
	bool active;          // Whether the particle is active
};


float randomFloat(float min, float max) {
	float random = static_cast<float>(rand()) / static_cast<float>(RAND_MAX); // Generate [0, 1]
	return min + random * (max - min); // Scale to [min, max]
}

// ------------------------------------------------------------------------------------------
// Initialization of scene
// ------------------------------------------------------------------------------------------
void load()
{

	// optimized version

	// load skybox shader
	//Shader simpleShaderSky("src/shader_sky.vert", "src/shader_sky.frag");
	//g_simpleShader_sky = simpleShaderSky.program;
	// although regular shader file was redesigned to not need this

	//load regular shader
	Shader simpleShader("src/shader.vert", "src/shader.frag");
	g_simpleShader = simpleShader.program;

	// put obj file paths into a vector
	objects.push_back("assets/sphere.obj");
	objects.push_back("assets/Thread.obj");
	objects.push_back("assets/Stellar.obj");
	objects.push_back("assets/Cloud.obj");
	objects.push_back("assets/Star.obj");
	objects.push_back("assets/Crescent.obj");
	objects.push_back("assets/Icosahedron.obj");
	objects.push_back("assets/Coin.obj");
	objects.push_back("assets/Tetrahedron.obj");
	objects.push_back("assets/Octahedron.obj");
	objects.push_back("assets/Heart.obj");
	objects.push_back("assets/Hex.obj");
	objects.push_back("assets/AmongUs.obj");
	objects.push_back("assets/plane.obj");

	objCount = objects.size();
	models.resize(objCount);
	g_vao.resize(objCount);
	g_NumTriangles.resize(objCount);

	// shapes vector getting its size based on number of objects
	for (int i = 0; i < objCount; i++)
		shapesVector.emplace_back();

	std::vector <bool> ret(objCount);
	for (int i = 0; i < objCount; i++) {
		ret[i] = tinyobj::LoadObj(shapesVector[i], objects[i].c_str());

		if (ret[i]) {
			cout << "OBJ File: " << objects[i] << " successfully loaded!\n";
		}
		else {
			cout << "OBJ File: " << objects[i] << " cannot be found or is not valid OBJ file.\n";
		}
	}

	for (int i = 0; i < objCount; i++) {
		GLuint chosen_shader = g_simpleShader;
		g_vao[i] = gl_createAndBindVAO();
		std::cout << "vao: " << g_vao[i] << "\n";

		// for skybox settings, they use regular shaders, but a component will indicate it is a skybox

		gl_createAndBindAttribute(&(shapesVector[i][0].mesh.positions[0]),
			shapesVector[i][0].mesh.positions.size() * sizeof(float),
			chosen_shader, "a_vertex", 3
		);

		gl_createIndexBuffer(&(shapesVector[i][0].mesh.indices[0]),
			shapesVector[i][0].mesh.indices.size() * sizeof(unsigned int)
		);

		gl_createAndBindAttribute(&(shapesVector[i][0].mesh.texcoords[0]),
			shapesVector[i][0].mesh.texcoords.size() * sizeof(float),
			chosen_shader, "a_uv", 2
		);

		gl_createAndBindAttribute(&(shapesVector[i][0].mesh.normals[0]),
			shapesVector[i][0].mesh.normals.size() * sizeof(float),
			chosen_shader, "a_normal", 3
		);

		gl_unbindVAO();

		//store number of triangles (use in draw())
		g_NumTriangles[i] = shapesVector[i][0].mesh.indices.size() / 3;
	}
	
	// put texture file paths into a vector
	textures.push_back("textures/milkyway.bmp");
	textures.push_back("textures/Thread.png");
	textures.push_back("textures/earth.bmp");
	textures.push_back("textures/moon.bmp");
	textures.push_back("textures/saturn.jpg");
	textures.push_back("textures/Stellar.png");
	textures.push_back("textures/Cloud.png");
	textures.push_back("textures/Star.png");
	textures.push_back("textures/Crescent.png");
	textures.push_back("textures/Icosahedron.png");
	textures.push_back("textures/Coin.png");
	textures.push_back("textures/Tetrahedron.png");
	textures.push_back("textures/Octahedron.png");
	textures.push_back("textures/Heart.png");
	textures.push_back("textures/Hex.png");
	textures.push_back("textures/AmongUs.jpg");
	textures.push_back("textures/rings.png");
	textures.push_back("textures/Starflake.png");
	textures.push_back("textures/earthnormal.bmp");		// index 18
	textures.push_back("textures/earthspec.bmp");		// index 19
	textures.push_back("textures/earthnight.bmp");		// index 20 - 3 indices noted for hard-coded multi-texturing

	texCount = textures.size();
	texture_ids.resize(texCount);

	for (int i = 0; i < texCount; i++) {
		int width, height, numChannels;

		stbi_set_flip_vertically_on_load(true); // remove if texture is flipped
		unsigned char* pixels = stbi_load(textures[i].c_str(), &width, &height, &numChannels, 0);
		glGenTextures(1, &texture_ids[i]);
		glBindTexture(GL_TEXTURE_2D, texture_ids[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		cout << "texture_index[" << i << "]: ";

		if (pixels) {
			// if-else statement created to not run into error when numChannels is different (RGB, RGBA)
			if (numChannels == 4) {
				glTexImage2D(
					GL_TEXTURE_2D, // target
					0, // level = 0 base, no mipmap
					GL_RGBA, // how the data will be stored (Grayscale, RGB, RGBA)
					width, //width of the image
					height, //height of the image
					0, // border
					GL_RGBA, // format of original data
					GL_UNSIGNED_BYTE, // type of data
					pixels
				);
				std::cout << "Successfully loaded: Texture " << textures[i].c_str() << " with a width of " << width << ", a height of " << height << ", and uses 4 channels." << std::endl;
			}
			else if (numChannels == 3) {
				glTexImage2D(
					GL_TEXTURE_2D, // target
					0, // level = 0 base, no mipmap
					GL_RGB, // how the data will be stored (Grayscale, RGB, RGBA)
					width, //width of the image
					height, //height of the image
					0, // border
					GL_RGB, // format of original data
					GL_UNSIGNED_BYTE, // type of data
					pixels
				);
				std::cout << "Successfully loaded: Texture " << textures[i].c_str() << " with a width of " << width << ", a height of " << height << ", and uses 3 channels." << std::endl;
			}
			else {
				std::cout << "Failed to load: Texture " << textures[i].c_str() << " with a width of " << width << ", a height of " << height << ", and uses " << numChannels << " channels. (error: channel)" << std::endl;
			}

			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else {
			std::cout << "Failed to load: Texture: " << textures[i].c_str() << " with a width of " << width << ", a height of " << height << ", and uses " << numChannels << " channels. (error: pixels)" << std::endl;
		}
		stbi_image_free(pixels);

		texture_loc = glGetUniformLocation(g_simpleShader, "u_texture");
		glUniform1i(texture_loc, i);
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, texture_ids[i]);
	}

	// put meshes into a vector

	// meshes[0] = thread
	meshes.push_back(Mesh(
		"thread",
		1,
		1,
		TransformationValues(
			glm::vec3(0.0f, 6.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.05f, 0.7f, 0.05f)
		),
		MaterialProperties(
			glm::vec3(0.1f, 0.1f, 0.1f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			100.0f,
			1.0f
		)
	));

	// meshes[1] = earth
	meshes.push_back(Mesh(
		"earth",
		0,
		2,
		TransformationValues(
			glm::vec3(earthX, 0.0f, earthZ),
			glm::vec3(0.0f, earth_rot, 0.0f),
			glm::vec3(0.5f, 0.5f, 0.5f)
		),
		MaterialProperties(
			glm::vec3(0.1f, 0.1f, 0.1f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			5.0f,
			1.0f
		)
	));

	// meshes[2] = moon
	meshes.push_back(Mesh(
		"moon",
		0,
		3,
		TransformationValues(
			glm::vec3(0.0f, 1.0f, 0.0f),
			glm::vec3(0.0f, moon_rot, 0.0f),
			glm::vec3(0.125f, 0.125f, 0.125f)
		),
		MaterialProperties(
			glm::vec3(0.1f, 0.1f, 0.1f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			10.0f,
			1.0f
		)
	));

	// meshes[4] = stellar (stellated dodecahedron)
	meshes.push_back(Mesh(
		"stellar",
		2,
		5,
		TransformationValues(
			glm::vec3(0.0f, 1.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.15f, 0.15f, 0.15f)
		),
		MaterialProperties(
			glm::vec3(0.1f, 0.1f, 0.1f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			20.0f,
			1.0f
		)
	));

	// meshes[5] = cloud
	meshes.push_back(Mesh(
		"cloud",
		3,
		6,
		TransformationValues(
			glm::vec3(0.0f, 2.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.125f, 0.125f, 0.125f)
		),
		MaterialProperties(
			glm::vec3(0.1f, 0.1f, 0.1f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			20.0f,
			1.0f
		)
	));

	// meshes[6] = star
	meshes.push_back(Mesh(
		"star",
		4,
		7,
		TransformationValues(
			glm::vec3(starX, starY, starZ),
			glm::vec3(90.0f, 0.0f, 0.0f),
			glm::vec3(0.5f, 0.5f, 0.5f)
		),
		MaterialProperties(
			glm::vec3(0.1f, 0.1f, 0.1f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			1.0f,
			1.0f
		)
	));

	// meshes[7] = crescent
	meshes.push_back(Mesh(
		"crescent",
		5,
		8,
		TransformationValues(
			glm::vec3(0.0f, 4.0f, 0.0f),
			glm::vec3(90.0f, 90.0f, 0.0f),
			glm::vec3(0.5f, 0.5f, 0.5f)
		),
		MaterialProperties(
			glm::vec3(0.1f, 0.1f, 0.1f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			10.0f,
			1.0f
		)
	));

	// meshes[8] = icosahedron
	meshes.push_back(Mesh(
		"icosahedron",
		6,
		9,
		TransformationValues(
			glm::vec3(0.0f, 5.0f, 0.0f),
			glm::vec3(90.0f, 0.0f, 0.0f),
			glm::vec3(0.2f, 0.2f, 0.2f)
		),
		MaterialProperties(
			glm::vec3(0.1f, 0.1f, 0.1f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			10.0f,
			1.0f
		)
	));

	// meshes[9] = coin
	meshes.push_back(Mesh(
		"coin",
		7,
		10,
		TransformationValues(
			glm::vec3(0.0f, 6.0f, 0.0f),
			glm::vec3(90.0f, 0.0f, 0.0f),
			glm::vec3(0.5f, 0.5f, 0.5f)
		),
		MaterialProperties(
			glm::vec3(0.1f, 0.1f, 0.1f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			10.0f,
			1.0f
		)
	));

	// meshes[10] = tetrahedron
	meshes.push_back(Mesh(
		"tetrahedron",
		8,
		11,
		TransformationValues(
			glm::vec3(0.0f, 7.0f, 0.0f),
			glm::vec3(90.0f, 90.0f, 90.0f),
			glm::vec3(0.5f, 0.5f, 0.5f)
		),
		MaterialProperties(
			glm::vec3(0.1f, 0.1f, 0.1f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			10.0f,
			1.0f
		)
	));

	// meshes[3] = saturn
	meshes.push_back(Mesh(
		"saturn",
		0,
		4,
		TransformationValues(
			glm::vec3(0.0f, 8.0f, 0.0f),
			glm::vec3(0.0f, saturn_rot, 0.0f),
			glm::vec3(0.4f, 0.4f, 0.4f)
		),
		MaterialProperties(
			glm::vec3(0.1f, 0.1f, 0.1f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			20.0f,
			1.0f
		)
	));

	// meshes[11] = octahedron
	meshes.push_back(Mesh(
		"octahedron",
		9,
		12,
		TransformationValues(
			glm::vec3(0.0f, 9.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.2f, 0.2f, 0.2f)
		),
		MaterialProperties(
			glm::vec3(0.1f, 0.1f, 0.1f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			100.0f,
			1.0f
		)
	));

	// meshes[12] = heart
	meshes.push_back(Mesh(
		"heart",
		10,
		13,
		TransformationValues(
			glm::vec3(0.0f, 10.0f, 0.0f),
			glm::vec3(90.0f, 0.0f, 0.0f),
			glm::vec3(0.2f, 0.2f, 0.2f)
		),
		MaterialProperties(
			glm::vec3(0.1f, 0.1f, 0.1f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			20.0f,
			1.0f
		)
	));

	// meshes[13] = hex
	meshes.push_back(Mesh(
		"hex",
		11,
		14,
		TransformationValues(
			glm::vec3(0.0f, 11.0f, 0.0f),
			glm::vec3(90.0f, hex_rot, 0.0f),
			glm::vec3(0.25f, 0.25f, 0.25f)
		),
		MaterialProperties(
			glm::vec3(0.1f, 0.1f, 0.1f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			25.0f,
			1.0f
		)
	));

	// meshes[14] = amogus
	meshes.push_back(Mesh(
		"amogus",
		12,
		15,
		TransformationValues(
			glm::vec3(0.0f, 12.0f, 0.0f),
			glm::vec3(0.5f, 1.0f, 0.0f),
			glm::vec3(0.008f, 0.008f, 0.008f)
		),
		MaterialProperties(
			glm::vec3(0.1f, 0.1f, 0.1f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			20.0f,
			1.0f
		)
	));

	// meshes[15] = rings
	meshes.push_back(Mesh(
		"rings",
		13,
		16,
		TransformationValues(
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(ring_rot, 0.0f, 0.0f),
			glm::vec3(4.0f, 4.0f, 4.0f)
		),
		MaterialProperties(
			glm::vec3(0.1f, 0.1f, 0.1f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			20.0f,
			-1.0f					// use -1.0f for alpha maps (just like skybox)
		)
	));

}

void renderObject(GLuint shader, Mesh mesh, vec3 animation_translation);

// ^ to tell the program these functions exists below

// ------------------------------------------------------------------------------------------
// This function actually draws to screen and called non-stop, in a loop
// ------------------------------------------------------------------------------------------
void draw()
{
	glClearColor(g_backgroundColor.x, g_backgroundColor.y, g_backgroundColor.z, 1.0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// camera settings
	// 
	// remove orthographic and orbital if there's time
	// but both are also useful during testing and debugging...

	float radius = 5.0f;
	float camX = sin(glfwGetTime()) * radius;
	float camZ = cos(glfwGetTime()) * radius;

	if (!orbital) {
		view_matrix = glm::lookAt(
			cameraPos,		//eye, where the camera is
			cameraPos+cameraTarget,	//center, where the camera is looking at
			cameraUp		//up, roll of pitch-yaw-roll
		);

		view_loc = glGetUniformLocation(g_simpleShader, "u_view");
		glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view_matrix));
	}
	else {
		view_matrix = glm::lookAt(
			glm::vec3(camX, 4.0f, camZ), //camX, 0, camZ
			glm::vec3(0.0f, 4.0f, 0.0f), //0,0,0
			glm::vec3(0.0f, 1.0f, 0.0f)  //0,1,0
		);

		view_loc = glGetUniformLocation(g_simpleShader, "u_view");
		glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view_matrix));
	}

	if (!orthographic) {

		projection_loc = glGetUniformLocation(g_simpleShader, "u_projection");
		projection_matrix = perspective(
			fov, // field of view
			1.0f, // aspect ratio 1:1
			0.1f, // near plane (distance from camera), very low number
			50.0f // far plane (distance from camera), relatively big but not too big
		);
		// on top of other code so that all vao have same projection

		glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection_matrix));
	}
	else {
		projection_loc = glGetUniformLocation(g_simpleShader, "u_projection");
		projection_matrix = ortho(
			-5.0f,
			5.0f,
			-5.0f,
			5.0f,
			-10.0f, // near plane
			10.0f // far plane
		);
		glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection_matrix));
	}

	// skybox settings activated first

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glUseProgram(g_simpleShader);

	// skybox functions
	// skybox is index 0 for models[], texture_ids[], g_vao[], g_NumTriangles[]

	model_loc = glGetUniformLocation(g_simpleShader, "u_model");
	texture_loc = glGetUniformLocation(g_simpleShader, "u_texture");
	alpha_loc = glGetUniformLocation(g_simpleShader, "u_alpha");

	models[0] = translate(mat4(1.0f), cameraPos);

	// send values to shader
	glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(models[0]));
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view_matrix));
	glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection_matrix));

	glUniform1i(texture_loc, 0);
	glUniform1f(alpha_loc, -1.0f);			// when using g_simpleShader (not g_simpleShader_sky), alpha = -1.0f signifies skybox settings
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_ids[0]);
	gl_bindVAO(g_vao[0]);
	glDrawElements(GL_TRIANGLES, 3 * g_NumTriangles[0], GL_UNSIGNED_INT, 0);



	// skybox setup done, now drawing other objects

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_TEXTURE_2D);

	// activate shader
	// glUseProgram(g_simpleShader);
	// no need to reactivate because merged shader programs already
	
	// funtion takes the following parameters:
	// index,
	// model_parent,
	// TransformationValues (translate, rotate, scale),
	// MaterialProperties (ambient, diffuse, specular, shininess, alpha)

	// Animation parameters
	float currentTime = glfwGetTime(); // Time elapsed
	float speed = 1.5f;                // Speed of movement
	float loopHeight = 15.0f;          // Total height of the motion path
	int numObjects = 15;               // Total number of objects
	float spacing = loopHeight / numObjects; // Equal spacing between objects
	float totalLoopTime = loopHeight / speed;

	// render thread
	renderObject(g_simpleShader, meshes[0], vec3(0.0f));

	// procedural animation
	for (int i = 1; i < numObjects; i++) {
		// start time offset
		float startOffset = i * (loopHeight / speed / numObjects); // total time per object

		// current relative position
		float elapsedTime = fmod(currentTime - startOffset + totalLoopTime, totalLoopTime);

		if (elapsedTime < 0) elapsedTime += totalLoopTime; // no negative time

		// downward motion
		float y_offset = loopHeight - fmod(elapsedTime * speed, loopHeight);

		// horizontal zigzag motion
		float x_offset = sin(elapsedTime * speed / loopHeight + i) * 2.0f;
		float z_offset = cos(elapsedTime * speed / loopHeight + i) * 1.0f;

		vec3 position = vec3(x_offset, y_offset, z_offset);
		// combined motion

		// Only render the object if it's within the visible path
		if (y_offset >= -spacing) {
			renderObject(g_simpleShader, meshes[i], position);
		}
	}

	// settings for alpha map usage

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);

	// rings (alpha map)

	renderObject(g_simpleShader, meshes[15], vec3(0.0f));
	
	// object animations below
	meshes[15].transform.rotation.x += 0.005;					// ring_rot
	meshes[9].transform.rotation.z += 0.01;						// coin_rot
	meshes[13].transform.rotation.y += 0.025;					// hex_rot
	meshes[1].transform.rotation.y += 0.01;						// earth_rot
	meshes[2].transform.rotation.y -= 0.01;						// moon_rot

	

	// fps camera updates with code below

	cameraTarget = glm::normalize(
		vec3(
			cos(glm::radians(camera_yaw)) * cos(glm::radians(camera_pitch)),
			sin(glm::radians(camera_pitch)),
			sin(glm::radians(camera_yaw)) * cos(glm::radians(camera_pitch))
		)
	);

	cameraRight = glm::normalize(glm::cross(cameraTarget, cameraUp));

	cameraTarget = glm::normalize(
		vec3(
			cos(glm::radians(camera_yaw)) * cos(glm::radians(camera_pitch)),
			sin(glm::radians(camera_pitch)),
			sin(glm::radians(camera_yaw)) * cos(glm::radians(camera_pitch))
		)
	);

	cameraRight = glm::normalize(glm::cross(cameraTarget, cameraUp));

}

// ------------------------------------------------------------------------------------------
// This function is called to render an object to screen
// ------------------------------------------------------------------------------------------
void renderObject(GLuint shader, Mesh mesh, vec3 animation_translation)
{
	// lay out variables from struct for clarity
	GLuint object_index = mesh.object_index;
	GLuint texture_index = mesh.texture_index;
	TransformationValues transform = mesh.transform;
	MaterialProperties material = mesh.material;
	bool has_multitextures = false;

	// activate shader
	glUseProgram(shader);

	// get uniform locations
	texture_loc = glGetUniformLocation(shader, "u_texture");
	light_loc = glGetUniformLocation(shader, "u_light");
	light_intensity_loc = glGetUniformLocation(shader, "u_light_intensity");
	cam_pos_loc = glGetUniformLocation(shader, "u_cam_pos");
	ambient_loc = glGetUniformLocation(shader, "u_ambient");
	diffuse_loc = glGetUniformLocation(shader, "u_diffuse");
	specular_loc = glGetUniformLocation(shader, "u_specular");
	shininess_loc = glGetUniformLocation(shader, "u_shininess");
	alpha_loc = glGetUniformLocation(shader, "u_alpha");
	model_loc = glGetUniformLocation(shader, "u_model");
	normal_loc = glGetUniformLocation(shader, "a_normal");

	// render textures
	glUniform1i(texture_loc, texture_index);
	glActiveTexture(GL_TEXTURE0 + texture_index);
	glBindTexture(GL_TEXTURE_2D, texture_ids[texture_index]);

	// multi-texturing
	// hard-coded for earth only since it's the only one with multi-texturing
	// ideally should be included in the struct but it'll be empty for the rest so this'll do

	if (mesh.mesh_name == "earth") {
		GLuint texture_normal_index = 18;
		GLuint texture_spec_index = 19;
		GLuint texture_night_index = 20;
		has_multitextures = true;

		has_multi_loc = glGetUniformLocation(g_simpleShader, "u_has_multitextures");
		glUniform1i(has_multi_loc, has_multitextures);

		texture_normal_loc = glGetUniformLocation(shader, "u_texture_normal");
		glUniform1i(texture_normal_loc, texture_normal_index);
		glActiveTexture(GL_TEXTURE0 + texture_normal_index);
		glBindTexture(GL_TEXTURE_2D, texture_ids[texture_normal_index]);

		texture_spec_loc = glGetUniformLocation(shader, "u_texture_spec");
		glUniform1i(texture_spec_loc, texture_spec_index);
		glActiveTexture(GL_TEXTURE0 + texture_spec_index);
		glBindTexture(GL_TEXTURE_2D, texture_ids[texture_spec_index]);

		texture_night_loc = glGetUniformLocation(shader, "u_texture_night");
		glUniform1i(texture_night_loc, texture_night_index);
		glActiveTexture(GL_TEXTURE0 + texture_night_index);
		glBindTexture(GL_TEXTURE_2D, texture_ids[texture_night_index]);
	}
	else {
		has_multitextures = false;
		has_multi_loc = glGetUniformLocation(g_simpleShader, "u_has_multitextures");
		glUniform1i(has_multi_loc, has_multitextures);
		
		glActiveTexture(GL_TEXTURE0 + 18);
		glBindTexture(GL_TEXTURE_2D, 0);

		glActiveTexture(GL_TEXTURE0 + 19);
		glBindTexture(GL_TEXTURE_2D, 0);

		glActiveTexture(GL_TEXTURE0 + 20);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// object transformations
	// (formerly models[object_index])
	models[object_index] = translate(mat4(1.0f), vec3(
		transform.translation.x + animation_translation.x,
		transform.translation.y + animation_translation.y,
		transform.translation.z + animation_translation.z)) *
		rotate(mat4(1.0f), transform.rotation.x, vec3(1.0f, 0.0f, 0.0f)) *
		rotate(mat4(1.0f), transform.rotation.y, vec3(0.0f, 1.0f, 0.0f)) *
		rotate(mat4(1.0f), transform.rotation.z, vec3(0.0f, 0.0f, 1.0f)) *
		scale(mat4(1.0f), vec3(transform.scale.x, transform.scale.y, transform.scale.z));

	mesh.model_matrix = models[object_index];

	// send transformations and normals to shader
	glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(mesh.model_matrix));	//(models[object_index]));
	glm::mat4 normal_matrix = glm::transpose(glm::inverse(mesh.model_matrix));		//(models[object_index]));
	glUniformMatrix4fv(normal_loc, 1, GL_FALSE, glm::value_ptr(normal_matrix));

	// send material properties to shader
	glUniform3f(light_loc, g_light.x, g_light.y, g_light.z);
	glUniform1f(light_intensity_loc, light_intensity);
	glUniform3f(cam_pos_loc, cameraPos.x, cameraPos.y, cameraPos.z);
	glUniform3f(ambient_loc, material.ambient.x, material.ambient.y, material.ambient.z);
	glUniform3f(diffuse_loc, material.diffuse.x, material.diffuse.y, material.diffuse.z);
	glUniform3f(specular_loc, material.specular.x, material.specular.y, material.specular.z);
	glUniform1f(shininess_loc, material.shininess);
	glUniform1f(alpha_loc, material.alpha);

	// bind vao
	gl_bindVAO(g_vao[object_index]);

	// draw to screen!
	glDrawElements(GL_TRIANGLES, 3 * g_NumTriangles[object_index], GL_UNSIGNED_INT, 0);

}

// ------------------------------------------------------------------------------------------
// This function is called to initialize the particle effect
// ------------------------------------------------------------------------------------------
void initializeParticles(std::vector<Particle>& particles, int numParticles) {
	for (int i = 0; i < numParticles; ++i) {
		Particle p;
		p.position = glm::vec3(
			randomFloat(-5.0f, 5.0f), // X range
			randomFloat(5.0f, 10.0f), // Y range (above the screen)
			randomFloat(-5.0f, 5.0f)  // Z range
		);
		p.velocity = glm::vec3(0.0f, randomFloat(-1.0f, -2.0f), 0.0f); // Downward
		p.lifetime = randomFloat(2.0f, 5.0f); // Random lifetime
		p.size = randomFloat(0.1f, 0.3f);     // Random size
		p.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); // White color
		p.active = true;
		particles.push_back(p);
	}
}

void updateParticles(std::vector<Particle>& particles, float deltaTime, float floorY) {
	for (auto& p : particles) {
		if (!p.active) continue;

		// Update position
		p.position += p.velocity * deltaTime;

		// Decrease lifetime
		p.lifetime -= deltaTime;

		// Fade out based on lifetime
		p.color.a = p.lifetime > 0 ? p.lifetime / 5.0f : 0.0f;

		// Check if it reached the floor
		if (p.position.y <= floorY || p.lifetime <= 0.0f) {
			p.active = false; // Deactivate particle
		}
	}
}

void renderParticles(const std::vector<Particle>& particles) {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (const auto& p : particles) {
		if (!p.active) continue;

		// Set color and size
		glPointSize(p.size * 10.0f); // Scale size for visibility
		glColor4f(p.color.r, p.color.g, p.color.b, p.color.a);

		// Render particle
		glBegin(GL_POINTS);
		glVertex3f(p.position.x, p.position.y, p.position.z);
		glEnd();
	}

	glDisable(GL_BLEND);
}

void respawnParticles(std::vector<Particle>& particles, float topY) {
	for (auto& p : particles) {
		if (!p.active) {
			p.position = glm::vec3(
				randomFloat(-5.0f, 5.0f), // X range
				randomFloat(topY, topY + 5.0f), // Reset Y (above the screen)
				randomFloat(-5.0f, 5.0f)       // Z range
			);
			p.velocity = glm::vec3(0.0f, randomFloat(-1.0f, -2.0f), 0.0f);
			p.lifetime = randomFloat(2.0f, 5.0f);
			p.size = randomFloat(0.1f, 0.3f);
			p.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			p.active = true;
		}
	}
}


// ------------------------------------------------------------------------------------------
// This function is called every time you press a screen
// ------------------------------------------------------------------------------------------
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	//quit
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, 1);
	//reload
	if (key == GLFW_KEY_R && action == GLFW_PRESS)
		load();
	if (key == GLFW_KEY_W && action == GLFW_PRESS && !orbital) {
		cameraPos += cameraTarget * cameraSpeed;
		cout << "pressed w button, moving camera forward" << endl;
	}
	if (key == GLFW_KEY_S && action == GLFW_PRESS && !orbital) {
		cameraPos -= cameraTarget * cameraSpeed;
		cout << "pressed s button, moving camera backward" << endl;
	}
	if (key == GLFW_KEY_D && action == GLFW_PRESS && !orbital) {
		cameraPos += cameraRight * cameraSpeed;
		cout << "pressed d button, moving camera to the right" << endl;
	}
	if (key == GLFW_KEY_A && action == GLFW_PRESS && !orbital) {
		cameraPos -= cameraRight * cameraSpeed;
		cout << "pressed a button, moving camera to the left" << endl;
	}
	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
		g_light.x += 1.0f;
		cout << "pressed right button, g_light.x = " << g_light.x << endl;
	}
	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
		g_light.x -= 1.0f;
		cout << "pressed left button, g_light.x = " << g_light.x << endl;
	}
	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
		g_light.z += 1.0f;
		cout << "pressed down button, g_light.z = " << g_light.z << endl;
	}
	if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
		g_light.z -= 1.0f;
		cout << "pressed up button, g_light.z = " << g_light.z << endl;
	}
	if (key == GLFW_KEY_RIGHT_BRACKET && action == GLFW_PRESS) {
		g_light.y += 1.0f;
		cout << "pressed right bracket button, g_light.y = " << g_light.y << endl;
	}
	if (key == GLFW_KEY_LEFT_BRACKET && action == GLFW_PRESS) {
		g_light.y -= 1.0f;
		cout << "pressed left bracket button, g_light.y = " << g_light.y << endl;
	}
	if (key == GLFW_KEY_N && action == GLFW_PRESS) {
		light_intensity -= 1.0f;
		cout << "pressed n button, light intensity decreases" << endl;
	}
	if (key == GLFW_KEY_M && action == GLFW_PRESS) {
		light_intensity += 1.0f;
		cout << "pressed m button, light intensity increases" << endl;
	}
	if (key == GLFW_KEY_P && action == GLFW_PRESS) {
		//toggle persp / ortho
		if (orthographic) {
			orthographic = false;
		}
		else {
			orthographic = true;
		}
		cout << "pressed p button, orthographic = " << orthographic << endl;
	}
	if (key == GLFW_KEY_Q && action == GLFW_PRESS && !orbital) {
		cameraPos += cameraUp * cameraSpeed;
		cout << "pressed q button, moving camera upward" << endl;
	}
	if (key == GLFW_KEY_Z && action == GLFW_PRESS && !orbital) {
		cameraPos -= cameraUp * cameraSpeed;
		cout << "pressed z button, moving camera downward" << endl;
	}
	if (key == GLFW_KEY_T && action == GLFW_PRESS) {
		cout << "pressed t button, glfwGetTime() = " << glfwGetTime() << endl;
	}
	if (key == GLFW_KEY_O && action == GLFW_PRESS) {
		if (orbital) {
			orbital = false;
		}
		else {
			orbital = true;
		}
		cout << "pressed o button, orbital = " << orbital << endl;
	}
	if (key == GLFW_KEY_I && action == GLFW_PRESS) {
		earthZ += -0.25f;
		cout << "pressed i button, earthZ = " << earthZ << endl;
	}
	if (key == GLFW_KEY_K && action == GLFW_PRESS) {
		earthZ += 0.25f;
		cout << "pressed k button, earthZ = " << earthZ << endl;
	}
	if (key == GLFW_KEY_J && action == GLFW_PRESS) {
		earthX += -0.25f;
		cout << "pressed j button, earthX = " << earthX << endl;
	}
	if (key == GLFW_KEY_L && action == GLFW_PRESS) {
		earthX += 0.25f;
		cout << "pressed l button, earthX = " << earthX << endl;
	}
}

// ------------------------------------------------------------------------------------------
// This function is called every time you click the mouse
// ------------------------------------------------------------------------------------------
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        cout << "Left mouse down at" << mouse_x << ", " << mouse_y << endl;
		//xAxis = (mouse_x - 255) / 256.0f;
		//yAxis = -(mouse_y - 255) / 256.0f;
		// ^ old code about clicking to screen to move objects to that position
    }
}

// ------------------------------------------------------------------------------------------
// This function is called every time the screen detects the cursor's presence
// ------------------------------------------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	if (!orbital) {

		lastX = 256;
		lastY = 256;

		if (firstMouse) {
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos;
		lastX = xpos;
		lastY = ypos;

		xoffset *= 0.001f;
		yoffset *= 0.001f;

		camera_yaw += xoffset;
		camera_pitch += yoffset;

		if (camera_pitch > 89.0f)
			camera_pitch = 89.0f;
		if (camera_pitch < -89.0f)
			camera_pitch = -89.0f;

		cameraTarget = glm::normalize(
			vec3(
				cos(glm::radians(camera_yaw)) * cos(glm::radians(camera_pitch)),
				sin(glm::radians(camera_pitch)),
				sin(glm::radians(camera_yaw)) * cos(glm::radians(camera_pitch))
			)
		);
	}
}

// ------------------------------------------------------------------------------------------
// This function is called every time the mouse is scrolled
// ------------------------------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	fov -= (float)yoffset;
	if (fov < 20.0f)
		fov = 20.0f;
	if (fov > 160.0f)
		fov = 160.0f;
	cout << "fov = " << fov << endl;
}

int main(void)
{
	//setup window and other stuff, defined in glfunctions.cpp
	GLFWwindow* window;
	if (!glfwInit())return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(g_ViewportWidth, g_ViewportHeight, "Hello OpenGL!", NULL, NULL);
	if (!window) {glfwTerminate();	return -1;}
	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;
	glewInit();

	//input callbacks
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetCursorPosCallback(window, mouse_callback);

	glClearColor(g_backgroundColor.x, g_backgroundColor.y, g_backgroundColor.z, 1.0f);

	//load all the resources
	load();

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window))
    {
		draw();

        // Swap front and back buffers
        glfwSwapBuffers(window);
        
        // Poll for and process events
        glfwPollEvents();
        
        //mouse position must be tracked constantly (callbacks do not give accurate delta)
        glfwGetCursorPos(window, &mouse_x, &mouse_y);
    }

    //terminate glfw and exit
    glfwTerminate();
    return 0;
}


