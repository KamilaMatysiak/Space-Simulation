#include "glew.h"
#include "freeglut.h"
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>
#include <ctime>
#include <vector>
#include "Shader_Loader.h"
#include "Render_Utils.h"
#include "Camera.h"
#include "Texture.h"

#include "Box.cpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "model.h"
#include <WinUser.h>

int SCR_WIDTH = 1240;
int SCR_HEIGHT = 720;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
int winId;
GLuint programTex;
GLuint programSkybox;
GLuint programSun;
GLuint programBlur;
GLuint programBloom;
GLuint programParticle;
GLuint VertexArrayID;

unsigned int pingpongFBO[2];
unsigned int pingpongColorbuffers[2];
unsigned int FBO;
unsigned int colorBuffers[2];
//particlepart
double lastTime;
static GLfloat* g_particule_position_size_data;
static GLubyte* g_particule_color_data;
GLuint particle_vertex_buffer;
GLuint particles_position_buffer;
GLuint particles_color_buffer;

Core::Shader_Loader shaderLoader;

Core::RenderContext armContext;
std::vector<Core::Node> arm;
int ballIndex;
bool bothEngines = true;


GLuint textureShip_normals;
GLuint sunTexture;
GLuint earthTexture;
GLuint moonTexture;
GLuint skyboxTexture;
GLuint shipTexture;
GLuint particleTexture;
obj::Model sphereModel;
obj::Model cubeModel;
obj::Model shipModel;

Core::RenderContext sphereContext;
Core::RenderContext cubeContext;
Core::RenderContext shipContext;

//assimp
std::shared_ptr<Model> corvette;
//std::vector<Core::RenderContext> corvetteMeshes;
std::shared_ptr<Model> crewmate;

float cameraAngle = 0;
glm::vec3 cameraPos = glm::vec3(-6, 0, 0);
glm::vec3 cameraDir;
glm::vec3 cameraSide;


glm::mat4 cameraMatrix, perspectiveMatrix;

glm::vec3 sunPos = glm::vec3(10.0f, 0.0f, -5.0f);
glm::vec3 sunPos2 = glm::vec3(25.0f, -1.0f, 10.0f);
//particlepart
struct Particle {
	glm::vec3 pos, speed;
	unsigned char r, g, b, a; // Color
	float size, angle, weight;
	float life; // Remaining life of the particle. if <0 : dead and unused.
	float cameradistance; // *Squared* distance to the camera. if dead : -1.0f

	bool operator<(const Particle& that) const {
		return this->cameradistance > that.cameradistance;
	}
};

const int MaxParticles = 1000;
Particle ParticlesContainer[MaxParticles];
int LastUsedParticle = 0;

void SortParticles() {
	std::sort(&ParticlesContainer[0], &ParticlesContainer[MaxParticles]);
}

int FindUnusedParticle() {

	for (int i = LastUsedParticle; i < MaxParticles; i++) {
		if (ParticlesContainer[i].life < 0) {
			LastUsedParticle = i;
			return i;
		}
	}

	for (int i = 0; i < LastUsedParticle; i++) {
		if (ParticlesContainer[i].life < 0) {
			LastUsedParticle = i;
			return i;
		}
	}

	return 0; // All particles are taken, override the first one
}

//Light
struct Light {
	glm::vec3 position;
	glm::vec3 color;
	float intensity;
};

int engineLightTimer = 50;

//wczytywanie skyboxa (musi byc jpg!)
std::vector<std::string> faces
{
	"skybox/right.jpg",
	"skybox/left.jpg",
	"skybox/top.jpg",
	"skybox/bottom.jpg",
	"skybox/front.jpg",
	"skybox/back.jpg"
};


std::vector<Light> lights;

void keyboard(unsigned char key, int x, int y)
{
	float angleSpeed = 0.1f;
	float moveSpeed = 0.1f;
	switch (key)
	{
	case 'q':
	{
		cameraAngle -= angleSpeed;
		lights[3].intensity = 0.005;
		engineLightTimer = 0;
		break;
	}

	case 'e':
	{
		cameraAngle += angleSpeed;
		lights[2].intensity = 0.005;
		engineLightTimer = 0;
		break;
	}

	case 'w':
	{
		cameraPos += cameraDir * moveSpeed;
		lights[2].intensity = 0.005;
		lights[3].intensity = 0.005;
		engineLightTimer = 0;
		break;
	}
	case 's': cameraPos -= cameraDir * moveSpeed; break;
	case 'd': cameraPos += glm::cross(cameraDir, glm::vec3(0, 1, 0)) * moveSpeed; break;
	case 'a': cameraPos -= glm::cross(cameraDir, glm::vec3(0, 1, 0)) * moveSpeed; break;
	case 'z': cameraPos += glm::cross(cameraDir, glm::vec3(0, 0, 1)) * moveSpeed; break;
	case 'x': cameraPos -= glm::cross(cameraDir, glm::vec3(0, 0, 1)) * moveSpeed; break;
	case 27: glutDestroyWindow(winId); break;
	}
}


unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

glm::mat4 createCameraMatrix()
{
	// Obliczanie kierunku patrzenia kamery (w plaszczyznie x-z) przy uzyciu zmiennej cameraAngle kontrolowanej przez klawisze.
	cameraDir = glm::vec3(cosf(cameraAngle), 0.0f, sinf(cameraAngle));
	glm::vec3 up = glm::vec3(0, 1, 0);

	cameraSide = glm::cross(cameraDir, up);

	return Core::createViewMatrix(cameraPos, cameraDir, up);
}
float frustumScale = 1.f;



void drawObject(GLuint program, Core::RenderContext context, glm::mat4 modelMatrix, glm::vec3 color)
{
	glUseProgram(program);

	glUniform3f(glGetUniformLocation(program, "objectColor"), color.x, color.y, color.z);

	glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;


	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniformMatrix4fv(glGetUniformLocation(program, "transformation"), 1, GL_FALSE, (float*)&transformation);

	Core::DrawContext(context);
	glUseProgram(0);
}

//funkcja rysujaca modele za pomoca assimpa
void drawFromAssimpModel(GLuint program, std::shared_ptr<Model> model, glm::mat4 modelMatrix, glm::vec3 color)
{
	glUseProgram(program);

	glUniform3f(glGetUniformLocation(program, "objectColor"), color.x, color.y, color.z);

	glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;

	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniformMatrix4fv(glGetUniformLocation(program, "transformation"), 1, GL_FALSE, (float*)&transformation);

	model->Draw(program);

	glUseProgram(0);
}

//Skybox
unsigned int loadCubemap(std::vector<std::string> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}
void drawSkybox(GLuint program, Core::RenderContext context, GLuint texID)
{
	glUseProgram(program);
	glDepthFunc(GL_LEQUAL);
	glm::mat4 transformation = perspectiveMatrix * glm::mat4(glm::mat3(cameraMatrix));
	glUniformMatrix4fv(glGetUniformLocation(program, "transformation"), 1, GL_FALSE, (float*)&transformation);
	//glDepthMask(GL_FALSE);
	//Core::SetActiveTexture(texID, "skybox", program, 0);
	glUniform1i(glGetUniformLocation(program, "skybox"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texID);
	Core::DrawContext(context);
	glDepthFunc(GL_LESS);
	//glDepthMask(GL_TRUE);
	glUseProgram(0);
}

void drawParticles(int ParticlesCount, glm::mat4 &transformation)
{
	glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
	glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
	glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLfloat) * 4, g_particule_position_size_data);

	glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
	glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
	glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLubyte) * 4, g_particule_color_data);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, particleTexture);
	glUniform1i(glGetUniformLocation(programParticle, "sprite"), 0);
	glUniform3f(glGetUniformLocation(programParticle, "CameraRight_worldspace"), cameraSide.x, cameraSide.y, cameraSide.z);
	glUniform3f(glGetUniformLocation(programParticle, "CameraUp_worldspace"), 0, 1, 0);

	glUniformMatrix4fv(glGetUniformLocation(programParticle, "VP"), 1, GL_FALSE, &transformation[0][0]);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, particle_vertex_buffer);
	glVertexAttribPointer(
		0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
	glVertexAttribPointer(
		1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		4,                                // size : x + y + z + size => 4
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		0,                                // stride
		(void*)0                          // array buffer offset
	);
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
	glVertexAttribPointer(
		2,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		4,                                // size : r + g + b + a => 4
		GL_UNSIGNED_BYTE,                 // type
		GL_TRUE,                          // normalized?    *** YES, this means that the unsigned char[4] will be accessible with a vec4 (floats) in the shader ***
		0,                                // stride
		(void*)0                          // array buffer offset
	);
	glVertexAttribDivisor(0, 0); // particles vertices : always reuse the same 4 vertices -> 0
	glVertexAttribDivisor(1, 1); // positions : one per quad (its center)                 -> 1
	glVertexAttribDivisor(2, 1); // color : one per quad                                  -> 1
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, ParticlesCount);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
}

//Textures
void drawObjectTexture(GLuint program, Core::RenderContext context, glm::mat4 modelMatrix, glm::vec3 texture, GLuint texID)
{
	glUseProgram(program);
	glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniform3f(glGetUniformLocation(program, "colorTex"), texture.x, texture.y, texture.z);
	glUniformMatrix4fv(glGetUniformLocation(program, "transformation"), 1, GL_FALSE, (float*)&transformation);

	Core::SetActiveTexture(texID, "colorTexture", program, 0);

	Core::DrawContext(context);
	glUseProgram(0);
}

//funkcja rysujaca planety (bez obracania wokol wlasnej osi bo ksiezyce sie psuja)
glm::mat4 drawPlanet(float time, glm::vec3 sunPos, glm::vec3 orbit, glm::vec3 translation, glm::vec3 scale)
{
	glm::mat4 planetModelMatrix = glm::mat4(1.0f);
	planetModelMatrix = glm::translate(planetModelMatrix, sunPos);
	planetModelMatrix = glm::rotate(planetModelMatrix, time, orbit);
	planetModelMatrix = glm::translate(planetModelMatrix, translation);
	planetModelMatrix = glm::scale(planetModelMatrix, scale);
	return planetModelMatrix;
}

//funkcja rysujaca ksiezyce orbitujace wokol danej planety
glm::mat4 drawMoon(glm::mat4 planetModelMatrix, float time, glm::vec3 orbit, glm::vec3 translation, glm::vec3 rotation, glm::vec3 scale)
{
	glm::mat4 moonModelMatrix = glm::mat4(planetModelMatrix);
	moonModelMatrix = glm::rotate(moonModelMatrix, time, orbit);
	moonModelMatrix = glm::translate(moonModelMatrix, translation);
	moonModelMatrix = glm::rotate(moonModelMatrix, time, rotation);
	moonModelMatrix = glm::scale(moonModelMatrix, scale);
	return moonModelMatrix;
}

void renderScene()
{
	cameraMatrix = createCameraMatrix();
	perspectiveMatrix = Core::createPerspectiveMatrix(0.01f, 1000.0f, frustumScale);
	float time = glutGet(GLUT_ELAPSED_TIME) / 1000.f;

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	//ustalanie pozycji slonc (lightPos)
	glm::mat4 sunModelMatrix = glm::mat4(1.0f);
	sunModelMatrix = glm::translate(sunModelMatrix, sunPos);
	sunModelMatrix = glm::scale(sunModelMatrix, glm::vec3(3.0f, 3.0f, 3.0f));

	glm::mat4 sunModelMatrix2 = glm::mat4(1.0f);
	sunModelMatrix2 = glm::translate(sunModelMatrix2, sunPos2);


	glm::mat4 earth = drawPlanet(time / 5.0f, sunPos*glm::vec3(1.5f, 1, 1), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(-10.5f, 0.0f, -10.5f), glm::vec3(0.5f, 0.5f, 0.5f));
	glm::mat4 moon = drawMoon(earth, time / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0, 1, 1), glm::vec3(1.5f, 1.0f, 1.0f), glm::vec3(0.3f, 0.3f, 0.3f));
	earth = glm::rotate(earth, time / 3.0f, glm::vec3(0.0f, 0.0f, 1.0f));

	glm::mat4 planet1 = drawPlanet(time / 5.0f, sunPos2, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(-6.5f, 0.0f, -6.5f), glm::vec3(0.4f, 0.4f, 0.4f));
	planet1 = glm::rotate(planet1, time / 3.0f, glm::vec3(0.0f, 0.0f, 1.0f));

	glm::mat4 crewmateModelMatrix = glm::translate(glm::vec3(0, 1, 1)) * glm::rotate(time / 2, glm::vec3(1, 0, 1)) * glm::scale(glm::vec3(0.01));
	glm::mat4 shipModelMatrix = glm::translate(cameraPos + cameraDir * 0.6f + glm::vec3(0, -0.25f, 0)) * glm::rotate(-cameraAngle + glm::radians(90.0f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.0001f));


	glUseProgram(programTex);

	lights[0].position = sunPos;
	lights[1].position = sunPos2;

	glm::mat4 engineLeft = glm::translate(shipModelMatrix, glm::vec3(450, 0, -1500));
	lights[2].position = glm::vec3(engineLeft[3][0], engineLeft[3][1], engineLeft[3][2]);

	glm::mat4 engineRight = glm::translate(shipModelMatrix, glm::vec3(-450, 0, -1500));
	lights[3].position = glm::vec3(engineRight[3][0], engineRight[3][1], engineRight[3][2]);

	for (int i = 0; i < lights.size(); i++)
	{
		std::string col = "pointLights[" + std::to_string(i) + "].color";
		std::string pos = "pointLights[" + std::to_string(i) + "].position";
		std::string ins = "pointLights[" + std::to_string(i) + "].intensity";
		glUniform3f(glGetUniformLocation(programTex, col.c_str()), lights[i].color.x, lights[i].color.y, lights[i].color.z);
		glUniform3f(glGetUniformLocation(programTex, pos.c_str()), lights[i].position.x, lights[i].position.y, lights[i].position.z);
		glUniform1f(glGetUniformLocation(programTex, ins.c_str()), lights[i].intensity);
	}

	glUniform3f(glGetUniformLocation(programTex, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	drawFromAssimpModel(programTex, corvette, shipModelMatrix, glm::vec3(1));
	drawFromAssimpModel(programTex, crewmate, crewmateModelMatrix, glm::vec3(1));

	//rysowanie Ziemi z ksiezycem
	glm::mat4 earth = drawPlanet(time / 5.0f, sunPos * glm::vec3(1.5f, 1, 1), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(-10.5f, 0.0f, -10.5f), glm::vec3(0.5f, 0.5f, 0.5f));
	glm::mat4 moon = drawMoon(earth, time / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0, 1, 1), glm::vec3(1.5f, 1.0f, 1.0f), glm::vec3(0.3f, 0.3f, 0.3f));
	earth = glm::rotate(earth, time / 3.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	drawObjectTexture(programTex, sphereContext, earth, glm::vec3(0.8f, 0.8f, 0.8f), earthTexture);
	drawObjectTexture(programTex, sphereContext, moon, glm::vec3(0.9f, 1.0f, 0.9f), moonTexture);
	drawObjectTexture(programTex, sphereContext, planet1, glm::vec3(0.4f, 0.2f, 0.9f), moonTexture);

	glUseProgram(programSun);
	glUniform3f(glGetUniformLocation(programSun, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	drawObjectTexture(programSun, sphereContext, sunModelMatrix, glm::vec3(3.5f, 3.8f, 3.8f), sunTexture);
	drawObjectTexture(programSun, sphereContext, sunModelMatrix2, glm::vec3(0.9f, 0.9f, 2.0f), sunTexture);
	
	//particlepart
	glUseProgram(programParticle);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	double delta = time - lastTime;
	lastTime = time;
	
	glm::mat4 transformation = perspectiveMatrix * cameraMatrix;
	
	int newparticles = 0;
	
	if (engineLightTimer < 40)
	{
		engineLightTimer++;
		newparticles = (int)(delta * 10000.0);
		if (newparticles > (int)(0.016f * 10000.0))
			newparticles = (int)(0.016f * 10000.0);
	}
	else
	{
		lights[2].intensity = 0.00001;
		lights[3].intensity = 0.00001;
	}

	for (int i = 0; i < newparticles; i++) {
		int particleIndex = FindUnusedParticle();
		ParticlesContainer[particleIndex].life = 100.0f;
		if (lights[2].intensity > 0.001 && lights[3].intensity > 0.001)
		{
			if (rand() % 2)
				ParticlesContainer[particleIndex].pos = lights[2].position;

			else
				ParticlesContainer[particleIndex].pos = lights[3].position;
		}
		else if(lights[2].intensity > 0.001) 
			ParticlesContainer[particleIndex].pos = lights[2].position;

		else if (lights[3].intensity > 0.001) 
			ParticlesContainer[particleIndex].pos = lights[3].position;
	

		float spread = 1.0f;
		glm::vec3 maindir = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 randomdir = glm::vec3(
			(rand() % 2000 - 1000.0f) / 5000.0f,
			(rand() % 2000 - 1000.0f) / 5000.0f,
			(rand() % 2000 - 1000.0f) / 5000.0f
		);

		ParticlesContainer[particleIndex].speed = maindir + randomdir * spread;


		// Very bad way to generate a random color
		ParticlesContainer[particleIndex].r = rand() % 100 + 100;
		ParticlesContainer[particleIndex].g = 0;
		ParticlesContainer[particleIndex].b = rand() % 100 + 50;
		ParticlesContainer[particleIndex].a = (rand() % 256) / 3;

		ParticlesContainer[particleIndex].size = (rand() % 1000) / 50000.0f + 0.01f;

	}
	// Simulate all particles
	int ParticlesCount = 0;
	for (int i = 0; i < MaxParticles; i++) {

		Particle& p = ParticlesContainer[i]; // shortcut

		if (p.life > 0.0f) {

			// Decrease life
			p.life -= delta;
			if (p.life > 0.0f) {

				// Simulate simple physics : gravity only, no collisions
				p.speed += glm::vec3(0.0f, -9.81f, 0.0f) * (float)delta * 0.5f;
				p.pos += p.speed * (float)delta;
				p.cameradistance = glm::length2(p.pos - cameraPos);
				//ParticlesContainer[i].pos += glm::vec3(0.0f,10.0f, 0.0f) * (float)delta;

				// Fill the GPU buffer
				g_particule_position_size_data[4 * ParticlesCount + 0] = p.pos.x;
				g_particule_position_size_data[4 * ParticlesCount + 1] = p.pos.y;
				g_particule_position_size_data[4 * ParticlesCount + 2] = p.pos.z;

				g_particule_position_size_data[4 * ParticlesCount + 3] = p.size;

				g_particule_color_data[4 * ParticlesCount + 0] = p.r;
				g_particule_color_data[4 * ParticlesCount + 1] = p.g;
				g_particule_color_data[4 * ParticlesCount + 2] = p.b;
				g_particule_color_data[4 * ParticlesCount + 3] = p.a;

			}
			else {
				// Particles that just died will be put at the end of the buffer in SortParticles();
				p.cameradistance = -1.0f;
			}

			ParticlesCount++;

		}
	}
	
	SortParticles();
	drawParticles(ParticlesCount, transformation);	
	drawSkybox(programSkybox, cubeContext, skyboxTexture);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	bool horizontal = true, first_iteration = true;
	unsigned int amount = 10;
	glUseProgram(programBlur);
	for (unsigned int i = 0; i < amount; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
		glUniform1i(glGetUniformLocation(programBlur, "horizontal"), horizontal);
		glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);
		renderQuad();
		horizontal = !horizontal;
		if (first_iteration)
			first_iteration = false;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glUseProgram(programBloom);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
	renderQuad();

	glutSwapBuffers();
}

void init_particles()
{
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);
	g_particule_position_size_data = new GLfloat[MaxParticles * 4];
	g_particule_color_data = new GLubyte[MaxParticles * 4];
	for (int i = 0; i < MaxParticles; i++) {
		ParticlesContainer[i].life = 100.0f;
		ParticlesContainer[i].cameradistance = -1.0f;
	}
	static const GLfloat g_vertex_buffer_data[] = {
		 -0.5f, -0.5f, 0.0f,
		  0.5f, -0.5f, 0.0f,
		 -0.5f,  0.5f, 0.0f,
		  0.5f,  0.5f, 0.0f,
	};
	glGenBuffers(1, &particle_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, particle_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
	// The VBO containing the positions and sizes of the particles
	glGenBuffers(1, &particles_position_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
	// Initialize with empty (NULL) buffer : it will be updated later, each frame.
	glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

	// The VBO containing the colors of the particles
	glGenBuffers(1, &particles_color_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
	// Initialize with empty (NULL) buffer : it will be updated later, each frame.
	glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);
	lastTime = glutGet(GLUT_ELAPSED_TIME) / 1000.f;
}

void init_bloom()
{
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);


	glGenTextures(2, colorBuffers);
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL
		);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0
		);
	}
	unsigned int rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

	unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachments);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glGenFramebuffers(2, pingpongFBO);
	glGenTextures(2, pingpongColorbuffers);
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
		glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer not complete!" << std::endl;
	}
}

void init()
{
	glEnable(GL_DEPTH_TEST);
	programTex = shaderLoader.CreateProgram("shaders/shader_tex.vert", "shaders/shader_tex.frag");
	programSkybox = shaderLoader.CreateProgram("shaders/shader_skybox.vert", "shaders/shader_skybox.frag");
	programSun = shaderLoader.CreateProgram("shaders/shader_sun.vert", "shaders/shader_sun.frag");
	programBlur = shaderLoader.CreateProgram("shaders/shader_blur.vert", "shaders/shader_blur.frag");
	programBloom = shaderLoader.CreateProgram("shaders/shader_bloom.vert", "shaders/shader_bloom.frag");
	programParticle = shaderLoader.CreateProgram("shaders/shader_particle.vert", "shaders/shader_particle.frag");

	glUseProgram(programBlur);
	glUniform1i(glGetUniformLocation(programBlur, "image"), 0);
	glUseProgram(programBloom);
	glUniform1i(glGetUniformLocation(programBloom, "scene"), 0);
	glUniform1i(glGetUniformLocation(programBloom, "bloomBlur"), 1);
	glUseProgram(0);


	corvette = std::make_shared<Model>("models/Corvette-F3.obj");
	crewmate = std::make_shared<Model>("models/space_humster.obj");
	//shipModel = obj::loadModelFromFile("models/spaceship.obj");
	sphereModel = obj::loadModelFromFile("models/sphere.obj");
	cubeModel = obj::loadModelFromFile("models/cube.obj");

	sphereContext.initFromOBJ(sphereModel);
	cubeContext.initFromOBJ(cubeModel);
	//shipContext.initFromOBJ(shipModel);
	shipTexture = Core::LoadTexture("textures/spaceship.png");
	sunTexture = Core::LoadTexture("textures/sun.png");
	earthTexture = Core::LoadTexture("textures/earth2.png");
	moonTexture = Core::LoadTexture("textures/moon.png");
	particleTexture = Core::LoadTexture("textures/sun.png");
	skyboxTexture = loadCubemap(faces);

	init_particles();
	init_bloom();


	Light l1;
	l1.position = sunPos;
	l1.color = glm::vec3(0.8f, 0.8f, 0.7f);
	l1.intensity = 7;
	lights.push_back(l1);

	Light l2;
	l2.position = sunPos2;
	l2.color = glm::vec3(0.5f, 0.5f, 0.5f);
	l2.intensity = 2;
	lights.push_back(l2);

	Light l3;
	l3.position = glm::vec3(0);
	l3.color = glm::vec3(1.0f, 0.0f, 0.0f);
	l3.intensity = 0.0001;
	lights.push_back(l3);

	Light l4;
	l4.position = glm::vec3(0);
	l4.color = glm::vec3(1.0f, 0.0f, 0.0f);
	l4.intensity = 0.0001;
	lights.push_back(l4);

}

void shutdown()
{

}

void onReshape(int width, int height)
{
	SCR_WIDTH = width;
	SCR_HEIGHT = height;
	frustumScale = (float)width / (float)height;
	glViewport(0, 0, width, height);
}
void idle()
{
	glutPostRedisplay();
}

int main(int argc, char** argv)
{
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	SCR_WIDTH = screenWidth; SCR_HEIGHT = screenHeight;

	glutInit(&argc, argv);
	glutSetOption(GLUT_MULTISAMPLE, 8);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(SCR_WIDTH, SCR_HEIGHT);
	//glutCreateWindow("GRK-PROJECT WIP");
	winId = glutCreateWindow("GRK-PROJECT WIP");
	glutFullScreen();
	glewInit();


	init();
	glutKeyboardFunc(keyboard);
	//to sprawia, że obiekty ukryte przed kamerą  nie są renderowane
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	//glFrontFace(GL_CW);

	glutDisplayFunc(renderScene);
	glutIdleFunc(idle);

	glutReshapeFunc(onReshape);
	glutMainLoop();

	shutdown();

	return 0;
}
