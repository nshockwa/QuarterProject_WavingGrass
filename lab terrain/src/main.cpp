/*
CPE/CSC 471 Lab base code Wood/Dunn/Eckhardt
*/

#include <iostream>
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#define PIH 3.1415926/2.

#include "WindowManager.h"
#include "Shape.h"
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
using namespace std;
using namespace glm;
shared_ptr<Shape> shape;

struct distCalc{
	float distance;
	vec3 pos;
};

bool disComp(distCalc a, distCalc b)
{
	return a.distance > b.distance;
}

double get_last_elapsed_time()
{
	static double lasttime = glfwGetTime();
	double actualtime =glfwGetTime();
	double difference = actualtime- lasttime;
	lasttime = actualtime;
	return difference;
}

class Mouse
{
private:
	bool mousemove = false;
public:
	bool is_mousemove() { return mousemove; }
	void swap_mousemove(GLFWwindow *window)
	{
		if (!mousemove)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
			double dcurrentx, dcurrenty;
			glfwGetCursorPos(window, &dcurrentx, &dcurrenty);
			holdx = dcurrentx;
			holdy = dcurrenty;
		}
		else
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		mousemove = !mousemove;
	}

	int holdx, holdy;
	int currentx, currenty;
	//void set_current(bool )
	Mouse() {}
	void process(GLFWwindow *window, vec3 *camerarotation)
	{
		if (!mousemove) return;
		double dcurrentx, dcurrenty;
		glfwGetCursorPos(window, &dcurrentx, &dcurrenty);
		currentx = dcurrentx;
		currenty = dcurrenty;
		vec2 diff = vec2(holdx - currentx, holdy - currenty);
		glfwSetCursorPos(window, (double)holdx, (double)holdy);
		*camerarotation -= (float)0.005*vec3(diff.y, diff.x, 0);

	}
};

class camera
{
private:
	glm::mat4 View;
public:
	glm::vec3 pos;
	glm::vec3 rot;
	int w, a, s, d;

	glm::mat4 get_viewmatrix() { return View; }
	camera()
	{
		w = a = s = d = 0;
		pos = rot = glm::vec3(0, 0, 0);
		pos = glm::vec3(0, 0, 0);
		rot = glm::vec3(0, 0, 0);
	}
	void process()
	{
		float going_forward = 0.0;
		float going_side = 0.0;
		if (w == 1)
			going_forward += 0.3;
		if (s == 1)
			going_forward -= 0.3;
		if (a == 1)
			going_side -= 0.1;
		if (d == 1)
			going_side += 0.1;

		if (rot.x > PIH) rot.x = PIH;
		if (rot.x < -PIH) rot.x = -PIH;
		glm::mat4 Ry = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
		glm::mat4 Rx = glm::rotate(glm::mat4(1), rot.x, glm::vec3(1, 0, 0));

		glm::mat4 R = Rx * Ry;

		glm::vec4 rpos = glm::vec4(going_side, 0, -going_forward, 1);

		rpos = rpos * R;
		pos.x -= rpos.x;
		pos.y -= rpos.y;
		pos.z -= rpos.z;

		glm::mat4 T = glm::translate(glm::mat4(1), glm::vec3(pos.x, pos.y, pos.z));
		View = R * T;
	}

};

camera mycam;

class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;
	Mouse mouse;

	//billboard ish
		#define PI 3.14159265358979324
		// Globals.
    unsigned int billboardtex; // Array of texture indices.
	float d = 40.0; // Distance of the trees image parallel to the line of sight.
	float b = 20.0; // Displacement of the trees image left of line of sight.
	int isBillboard = 0; // Is billboarding on?
	
	
	GLuint VertexArrayIDBox, VertexBufferIDBox, VertexBufferTex;

	// Our shader program
	std::shared_ptr<Program> prog, heightshader;
	#define GRASS_ARR_SIZE 40000
	distCalc grassCalc[GRASS_ARR_SIZE];
	vec3 grassPositions[GRASS_ARR_SIZE];
	// Contains vertex information for OpenGL
	GLuint VertexArrayID;
	GLuint instanceVBO;

	// Data necessary to give our box to OpenGL
	GLuint MeshPosID, MeshTexID, IndexBufferIDBox;

	//texture data
	GLuint Texture, grassTex[6];
	GLuint Texture2,HeightTex;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		
		if (key == GLFW_KEY_W && action == GLFW_PRESS)
		{
			mycam.w = 1;
		}
		if (key == GLFW_KEY_W && action == GLFW_RELEASE)
		{
			mycam.w = 0;
		}
		if (key == GLFW_KEY_S && action == GLFW_PRESS)
		{
			mycam.s = 1;
		}
		if (key == GLFW_KEY_S && action == GLFW_RELEASE)
		{
			mycam.s = 0;
		}
		if (key == GLFW_KEY_A && action == GLFW_PRESS)
		{
			mycam.a = 1;
		}
		if (key == GLFW_KEY_A && action == GLFW_RELEASE)
		{
			mycam.a = 0;
		}
		if (key == GLFW_KEY_D && action == GLFW_PRESS)
		{
			mycam.d = 1;
		}
		if (key == GLFW_KEY_D && action == GLFW_RELEASE)
		{
			mycam.d = 0;
		}
	}

	// callback for the mouse when clicked move the triangle when helper functions
	// written

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		double posX, posY;
		float newPt[2];
		if (action == GLFW_PRESS)
		{
			glfwGetCursorPos(window, &posX, &posY);
			std::cout << "Pos X " << posX <<  " Pos Y " << posY << std::endl;

			//change this to be the points converted to WORLD
			//THIS IS BROKEN< YOU GET TO FIX IT - yay!
			newPt[0] = 0;
			newPt[1] = 0;

			std::cout << "converted:" << newPt[0] << " " << newPt[1] << std::endl;
			glBindBuffer(GL_ARRAY_BUFFER, MeshPosID);
			//update the vertex array with the updated points
			glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*6, sizeof(float)*2, newPt);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			mouse.swap_mousemove(windowManager->getHandle());

		}
	}

	//if the window is resized, capture the new size and reset the viewport
	void resizeCallback(GLFWwindow *window, int in_width, int in_height)
	{
		//get the window size - may be different then pixels for retina
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
	}
#define MESHSIZE 100
	void init_mesh()
	{

		//generate the VAO
		glGenVertexArrays(1, &VertexArrayID);
		glBindVertexArray(VertexArrayID);

		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &MeshPosID);
		glBindBuffer(GL_ARRAY_BUFFER, MeshPosID);
		vec3 vertices[MESHSIZE * MESHSIZE * 4];
		for(int x=0;x<MESHSIZE;x++)
			for (int z = 0; z < MESHSIZE; z++)
				{
				vertices[x * 4 + z*MESHSIZE * 4 + 0] = vec3(0.0, 0.0, 0.0) + vec3(x, 0, z);
				vertices[x * 4 + z*MESHSIZE * 4 + 1] = vec3(1.0, 0.0, 0.0) + vec3(x, 0, z);
				vertices[x * 4 + z*MESHSIZE * 4 + 2] = vec3(1.0, 0.0, 1.0) + vec3(x, 0, z);
				vertices[x * 4 + z*MESHSIZE * 4 + 3] = vec3(0.0, 0.0, 1.0) + vec3(x, 0, z);
				}
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * MESHSIZE * MESHSIZE * 4, vertices, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		//tex coords
		float t = 1. / 100;
		vec2 tex[MESHSIZE * MESHSIZE * 4];
		for (int x = 0; x<MESHSIZE; x++)
			for (int y = 0; y < MESHSIZE; y++)
			{
				tex[x * 4 + y*MESHSIZE * 4 + 0] = vec2(0.0, 0.0)+ vec2(x, y)*t;
				tex[x * 4 + y*MESHSIZE * 4 + 1] = vec2(t, 0.0)+ vec2(x, y)*t;
				tex[x * 4 + y*MESHSIZE * 4 + 2] = vec2(t, t)+ vec2(x, y)*t;
				tex[x * 4 + y*MESHSIZE * 4 + 3] = vec2(0.0, t)+ vec2(x, y)*t;
			}
		glGenBuffers(1, &MeshTexID);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, MeshTexID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * MESHSIZE * MESHSIZE * 4, tex, GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glGenBuffers(1, &IndexBufferIDBox);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferIDBox);
		GLushort elements[MESHSIZE * MESHSIZE * 6];
		int ind = 0;
		for (int i = 0; i<MESHSIZE * MESHSIZE * 6; i+=6, ind+=4)
			{
			elements[i + 0] = ind + 0;
			elements[i + 1] = ind + 1;
			elements[i + 2] = ind + 2;
			elements[i + 3] = ind + 0;
			elements[i + 4] = ind + 2;
			elements[i + 5] = ind + 3;
			}			
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*MESHSIZE * MESHSIZE * 6, elements, GL_STATIC_DRAW);
		glBindVertexArray(0);



	}

	void initGeom()
	{
		

		//initialize the net mesh
		init_mesh();

		string resourceDirectory = "../resources" ;
		// Initialize mesh.
		
		shape = make_shared<Shape>();
		//shape->loadMesh(resourceDirectory + "/t800.obj");
		shape->loadMesh(resourceDirectory + "/sphere.obj");
		shape->resize();
		shape->init();

		int width, height, channels;
		char filepath[1000];

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


		//texture 1
		string str = resourceDirectory + "/grass.jpg";
		strcpy(filepath, str.c_str());
		unsigned char* data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &Texture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		//texture 2
		str = resourceDirectory + "/sky.jpg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &Texture2);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, Texture2);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		//texture 3
		str = resourceDirectory + "/height.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &HeightTex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, HeightTex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		stbi_image_free(data);
		//my grass tex
		str = resourceDirectory + "/grass.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &grassTex[0]);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, grassTex[0]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


		str = resourceDirectory + "/thickgrass.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &grassTex[1]);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, grassTex[1]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		str = resourceDirectory + "/tallgrass1.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &grassTex[2]);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, grassTex[2]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		str = resourceDirectory + "/longgrass.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &grassTex[3]);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, grassTex[3]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


		str = resourceDirectory + "/variedgrass.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &grassTex[4]);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, grassTex[4]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


		str = resourceDirectory + "/africagrass.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &grassTex[5]);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, grassTex[5]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		str = resourceDirectory + "/drygrass.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &grassTex[6]);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, grassTex[6]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		str = resourceDirectory + "/reedgrass.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &grassTex[7]);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, grassTex[7]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		str = resourceDirectory + "/tallwheatgrass.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &grassTex[8]);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, grassTex[8]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		str = resourceDirectory + "/thingrass.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &grassTex[9]);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, grassTex[9]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		//[TWOTEXTURES]
		//set the 2 textures to the correct samplers in the fragment shader:
		GLuint Tex1Location = glGetUniformLocation(prog->pid, "tex");//tex, tex2... sampler in the fragment shader
		GLuint Tex2Location = glGetUniformLocation(prog->pid, "tex2");
		GLuint Tex3Location = glGetUniformLocation(prog->pid, "tex3");
		GLuint Tex4Location = glGetUniformLocation(prog->pid, "tex4");
		GLuint Tex5Location = glGetUniformLocation(prog->pid, "tex5");
		GLuint Tex6Location = glGetUniformLocation(prog->pid, "tex6");
		GLuint Tex7Location = glGetUniformLocation(prog->pid, "tex7");
		GLuint Tex8Location = glGetUniformLocation(prog->pid, "tex8");
		GLuint Tex9Location = glGetUniformLocation(prog->pid, "tex9");
		GLuint Tex10Location = glGetUniformLocation(prog->pid, "tex10");
		// Then bind the uniform samplers to texture units:
		glUseProgram(prog->pid);
		glUniform1i(Tex1Location, 0);
		glUniform1i(Tex2Location, 1);
		glUniform1i(Tex3Location, 2);
		glUniform1i(Tex4Location, 3);
		glUniform1i(Tex5Location, 4);
		glUniform1i(Tex6Location, 5);
		glUniform1i(Tex7Location, 6);
		glUniform1i(Tex8Location, 7);
		glUniform1i(Tex9Location, 8);
		glUniform1i(Tex10Location, 9);

		Tex1Location = glGetUniformLocation(heightshader->pid, "tex");//tex, tex2... sampler in the fragment shader
		Tex2Location = glGetUniformLocation(heightshader->pid, "tex2");
		// Then bind the uniform samplers to texture units:
		glUseProgram(heightshader->pid);
		glUniform1i(Tex1Location, 0);
		glUniform1i(Tex2Location, 1);

		/* GRASS POSITIONS*/

		glGenVertexArrays(1, &VertexArrayIDBox);
		glBindVertexArray(VertexArrayIDBox);

		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &VertexBufferIDBox);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferIDBox);

		GLfloat *rectangle_vertices = new GLfloat[18];
		// front
		int verccount = 0;

		rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;


		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), rectangle_vertices, GL_STATIC_DRAW);
		//we need to set up the vertex array
		glEnableVertexAttribArray(0);
		//key function to get up how many elements to pull out at a time (3)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);


		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &VertexBufferTex);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferTex);

		float t = 1. / 100.;
		GLfloat *rectangle_texture_coords = new GLfloat[12];
		int texccount = 0;
		rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 1;
		rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 1;
		rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 0;
		rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 1;
		rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 0;
		rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 0;

		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), rectangle_texture_coords, GL_STATIC_DRAW);
		//we need to set up the vertex array
		glEnableVertexAttribArray(2);
		//key function to get up how many elements to pull out at a time (3)
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);



		
		int idx = 0;
		// init grass sort obj
		for (int x = -(sqrt(GRASS_ARR_SIZE)) / 2; x < (sqrt(GRASS_ARR_SIZE)) / 2; x++) {
			for (int z = -(sqrt(GRASS_ARR_SIZE)) / 2; z < (sqrt(GRASS_ARR_SIZE)) / 2; z++) {
				grassCalc[idx].pos = glm::vec3(x *0.5, 0.0f, z *0.5);
				grassCalc[idx].distance = distance(grassCalc[idx].pos, -mycam.pos);
				idx++;
			}
		}
		// sort grass by distance
		sort(&grassCalc[0], &grassCalc[GRASS_ARR_SIZE], disComp);
		idx = 0;

		// copy into 1D array
		for (size_t i = 0; i != GRASS_ARR_SIZE; ++i)
		{
			grassPositions[idx++] = grassCalc[i].pos;
		}

		//do instance prep

		glGenBuffers(1, &instanceVBO);
		glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
		glBufferData(GL_ARRAY_BUFFER, GRASS_ARR_SIZE * sizeof(glm::vec3), grassPositions, GL_STATIC_DRAW);
		int position_loc = glGetAttribLocation(prog->pid, "instancePosOffset");
		for (size_t i = 0; i != GRASS_ARR_SIZE; ++i)
		{
			glEnableVertexAttribArray(position_loc + i);
			glVertexAttribPointer(position_loc + i, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)(sizeof(vec3)*i));
			glVertexAttribDivisor(position_loc + i, 1);
		}

		glBindVertexArray(0);


		/* debugging */
		//cout << "unSorted Array looks like this." << endl;
		//cout << "unSorted Array looks like this." << endl;
		//cout << "unSorted Array looks like this." << endl;
		//cout << "unSorted Array looks like this." << endl;
		//cout << "unSorted Array looks like this." << endl;

		////std::vector<int> grassvector(begin(grassPositions), end(grassPositions));
		//for (size_t i = 0; i != GRASS_ARR_SIZE; ++i)
		//	cout << "grassCalc[" << i << "] (" << grassCalc[i].pos.x << ", " << grassCalc[i].pos.y << ", " << grassCalc[i].pos.z << ")" << endl;

		//std::sort(grassvector.begin(), grassvector.end(), compareDistance);	//init rectangle mesh (2 triangles) for the post processing
		/*cout << "Sorted Array looks like this." << endl;
		cout << "Sorted Array looks like this." << endl;
		cout << "Sorted Array looks like this." << endl;
		cout << "Sorted Array looks like this." << endl;
		cout << "Sorted Array looks like this." << endl;
		for (size_t i = 0; i != GRASS_ARR_SIZE; ++i)
			cout << "grassCalc[" << i << "] (" << grassCalc[i].pos.x << ", " << grassCalc[i].pos.y << ", " << grassCalc[i].pos.z << ")" << endl;
*/
		
		/*cout << "grassPositions[" << i << "] (" << grassPositions[i].x << ", " << grassPositions[i].y << ", " << grassPositions[i].z << ")" << endl;*/
				


		//instanceVBO




	}

	//General OGL initialization - set OGL state here
	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		// Set background color.
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		// Initialize the GLSL program.
		prog = std::make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDirectory + "/shader_vertex.glsl", resourceDirectory + "/shader_fragment.glsl");
		if (!prog->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("campos");
		prog->addUniform("camoff");
		prog->addUniform("timeStamp");
		prog->addUniform("wind");

		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");
		prog->addAttribute("vertTex");


		// Initialize the GLSL program.
		heightshader = std::make_shared<Program>();
		heightshader->setVerbose(true);
		heightshader->setShaderNames(resourceDirectory + "/height_vertex.glsl", resourceDirectory + "/height_frag.glsl");
		if (!heightshader->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		heightshader->addUniform("P");
		heightshader->addUniform("V");
		heightshader->addUniform("M");
		heightshader->addUniform("camoff");
		heightshader->addUniform("campos");
		heightshader->addAttribute("vertPos");
		heightshader->addAttribute("vertTex");
	}


	/****DRAW
	This is the most important function in your program - this is where you
	will actually issue the commands to draw any geometry you have set up to
	draw
	********/
	void render()
	{
		double frametime = get_last_elapsed_time();


		mycam.process();

		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width/(float)height;
		glViewport(0, 0, width, height);

		// Clear framebuffer.
		glClearColor(0.8f, 0.8f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Create the matrix stacks - please leave these alone for now
		

		glm::mat4 V, M, P; //View, Model and Perspective matrix
		V = glm::mat4(1);
		M = glm::mat4(1);
		// Apply orthographic projection....
		P = glm::ortho(-1 * aspect, 1 * aspect, -1.0f, 1.0f, -2.0f, 100.0f);		
		if (width < height)
			{
			P = glm::ortho(-1.0f, 1.0f, -1.0f / aspect,  1.0f / aspect, -2.0f, 100.0f);
			}
		// ...but we overwrite it (optional) with a perspective projection.
		P = glm::perspective((float)(3.14159 / 4.), (float)((float)width/ (float)height), 0.1f, 100.0f); //so much type casting... GLM metods are quite funny ones

		//animation with the model matrix:
		static float w = 0.0;
		w += 1.0 * frametime;//rotation angle
		float trans = 0;// sin(t) * 2;

		V = mycam.get_viewmatrix();
		mouse.process(windowManager->getHandle(), &mycam.rot);



		//Height Prog...
		heightshader->bind();
		//height shader depth func different from grass
		glDepthFunc(GL_LESS);
		//translate whole height mesh
		glm::mat4 TransY = glm::translate(glm::mat4(1.0f), glm::vec3(-50.0f, -3.0f, -50));
		M = TransY;
		//camera offset to be used in shaders for repeating nature
		vec3 offset = mycam.pos;
		offset.y = 0;
		offset.x = (int)offset.x;
		offset.z = (int)offset.z;
		//cout << offset.x << endl;
		glUniformMatrix4fv(heightshader->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniformMatrix4fv(heightshader->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(heightshader->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniform3fv(heightshader->getUniform("camoff"), 1, &offset[0]);
		glUniform3fv(heightshader->getUniform("campos"), 1, &mycam.pos[0]);
		//draw mesh with textures
		glBindVertexArray(VertexArrayID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferIDBox);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, HeightTex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, Texture);
		glDrawElements(GL_TRIANGLES, MESHSIZE*MESHSIZE * 6, GL_UNSIGNED_SHORT, (void*)0);
		heightshader->unbind();



		//Grass Prog...
		prog->bind();
		//grass shader depth func different from height
		glDepthFunc(GL_NOTEQUAL);
		float time = glfwGetTime();
		vec3 wind = vec3(0.0, 1.0, 10.0);
		TransY = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -3.0f, -0.0f));
		M = TransY;
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		//send the matrices to the shaders
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniform3fv(prog->getUniform("campos"), 1, &mycam.pos[0]);
	
		glUniform3fv(prog->getUniform("camoff"), 1, &offset[0]);
		glUniform3fv(prog->getUniform("campos"), 1, &mycam.pos[0]);
		glUniform1fv(prog->getUniform("timeStamp"), 1, &time);
		glUniform3fv(prog->getUniform("wind"), 1, &wind[0]);
		//send over textures
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, grassTex[0]);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, grassTex[1]);	
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, grassTex[2]);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, grassTex[3]);		
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, grassTex[4]);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, grassTex[5]);		
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, grassTex[6]);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, grassTex[7]);		
		glActiveTexture(GL_TEXTURE8);
		glBindTexture(GL_TEXTURE_2D, grassTex[8]);
		glActiveTexture(GL_TEXTURE9);
		glBindTexture(GL_TEXTURE_2D, grassTex[9]);
		//draw instances
		glBindVertexArray(VertexArrayIDBox);
		glDrawArraysInstanced(GL_TRIANGLES, 0, 6, GRASS_ARR_SIZE);

		prog->unbind();




	}

};
//******************************************************************************************
int main(int argc, char **argv)
{
	std::string resourceDir = "../resources"; // Where the resources are loaded from
	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	/* your main will always include a similar set up to establish your window
		and GL context, etc. */
	WindowManager * windowManager = new WindowManager();
	windowManager->init(1920, 1080);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	/* This is the code that will likely change program to program as you
		may need to initialize or set up different data and state */
	// Initialize scene.
	application->init(resourceDir);
	application->initGeom();

	// Loop until the user closes the window.
	while(! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.
		application->render();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
