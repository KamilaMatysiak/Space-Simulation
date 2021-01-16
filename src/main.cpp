#include "glew.h"
#include "freeglut.h"
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>
#include <ctime>

#include "Shader_Loader.h"
#include "Render_Utils.h"
#include "Camera.h"
#include "Texture.h"

#include "Box.cpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GLuint program;
GLuint programTex;
GLuint programSun;
Core::Shader_Loader shaderLoader;


Core::RenderContext armContext;
std::vector<Core::Node> arm;
int ballIndex;

GLuint sunTexture;
GLuint earthTexture;
obj::Model sphereModel;
Core::RenderContext sphereContext;


float cameraAngle = 0;
glm::vec3 cameraPos = glm::vec3(-6, 0, 0);
glm::vec3 cameraDir;

glm::mat4 cameraMatrix, perspectiveMatrix;

void keyboard(unsigned char key, int x, int y)
{
	float angleSpeed = 0.1f;
	float moveSpeed = 0.1f;
	switch (key)
	{
	case 'z': cameraAngle -= angleSpeed; break;
	case 'x': cameraAngle += angleSpeed; break;
	case 'w': cameraPos += cameraDir * moveSpeed; break;
	case 's': cameraPos -= cameraDir * moveSpeed; break;
	case 'd': cameraPos += glm::cross(cameraDir, glm::vec3(0, 1, 0)) * moveSpeed; break;
	case 'a': cameraPos -= glm::cross(cameraDir, glm::vec3(0, 1, 0)) * moveSpeed; break;
	case 'e': cameraPos += glm::cross(cameraDir, glm::vec3(1, 0, 0)) * moveSpeed; break;
	case 'q': cameraPos -= glm::cross(cameraDir, glm::vec3(1, 0, 0)) * moveSpeed; break;
	}
}

glm::mat4 createCameraMatrix()
{
	// Obliczanie kierunku patrzenia kamery (w plaszczyznie x-z) przy uzyciu zmiennej cameraAngle kontrolowanej przez klawisze.
	cameraDir = glm::vec3(cosf(cameraAngle), 0.0f, sinf(cameraAngle));
	glm::vec3 up = glm::vec3(0, 1, 0);

	return Core::createViewMatrix(cameraPos, cameraDir, up);
}

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

void drawObjectTexture(GLuint program, Core::RenderContext context, glm::mat4 modelMatrix, glm::vec3 texture, GLuint texID)
{
	glUseProgram(program);
	glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniformMatrix4fv(glGetUniformLocation(program, "transformation"), 1, GL_FALSE, (float*)&transformation);

	Core::SetActiveTexture(texID, "colorTexture", program, 0);

	Core::DrawContext(context);
	glUseProgram(0);
}

glm::mat4 drawPlanet(float time, glm::vec3 orbit, glm::vec3 translation, glm::vec3 scale)
{
	glm::mat4 planetModelMatrix = glm::mat4(1.0f);
	planetModelMatrix = glm::translate(planetModelMatrix, glm::vec3(2, 0, 2)); //pozycja s³oñca
	planetModelMatrix = glm::rotate(planetModelMatrix, time, orbit);
	planetModelMatrix = glm::translate(planetModelMatrix, translation);
	planetModelMatrix = glm::scale(planetModelMatrix, scale);
	return planetModelMatrix;
}

void renderScene()
{
	// Aktualizacja macierzy widoku i rzutowania. Macierze sa przechowywane w zmiennych globalnych, bo uzywa ich funkcja drawObject.
	// (Bardziej elegancko byloby przekazac je jako argumenty do funkcji, ale robimy tak dla uproszczenia kodu.
	//  Jest to mozliwe dzieki temu, ze macierze widoku i rzutowania sa takie same dla wszystkich obiektow!)
	cameraMatrix = createCameraMatrix();
	perspectiveMatrix = Core::createPerspectiveMatrix();
	float time = glutGet(GLUT_ELAPSED_TIME) / 1000.f;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glUseProgram(program);

	glm::vec3 lightPos = glm::vec3(2, 0, 2);
	//glUniform3f(glGetUniformLocation(program, "light_dir"), 2, 1, 0);
	glUniform3f(glGetUniformLocation(program, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
	glUniform3f(glGetUniformLocation(program, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
	
	glUseProgram(programSun);
	glUniform3f(glGetUniformLocation(programSun, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	glm::mat4 sunModelMatrix = glm::mat4(1.0f);
	sunModelMatrix = glm::translate(sunModelMatrix, glm::vec3(0, 0, 0));
	sunModelMatrix = glm::rotate(sunModelMatrix, time/10.0f, glm::vec3(0.0f, 1.0f, 1.0f));
	drawObjectTexture(programSun, sphereContext, sunModelMatrix, glm::vec3(0.8f, 0.5f, 0.1f), sunTexture);

	glUseProgram(programTex);

	glUniform3f(glGetUniformLocation(programTex, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
	glUniform3f(glGetUniformLocation(programTex, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
	glm::mat4 earth = drawPlanet(time / 5.0f, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(-3.5f, 0.0f, -3.5f), glm::vec3(0.5f, 0.5f, 0.5f));
	drawObjectTexture(programTex, sphereContext, earth, glm::vec3(0.8f, 0.8f, 0.8f), earthTexture);

	glUseProgram(0);
	glutSwapBuffers();
}

void init()
{
	glEnable(GL_DEPTH_TEST);
	program = shaderLoader.CreateProgram("shaders/shader_4_1.vert", "shaders/shader_4_1.frag");
	programTex = shaderLoader.CreateProgram("shaders/shader_4_tex.vert", "shaders/shader_4_tex.frag");
	programSun = shaderLoader.CreateProgram("shaders/shader_4_sun.vert", "shaders/shader_4_sun.frag");

	sphereModel = obj::loadModelFromFile("models/sphere.obj");
	sphereContext.initFromOBJ(sphereModel);
	sunTexture = Core::LoadTexture("textures/sun.png");
	earthTexture = Core::LoadTexture("textures/earth2.png");
}

void shutdown()
{
	shaderLoader.DeleteProgram(program);
}

void idle()
{
	glutPostRedisplay();
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(200, 300);
	glutInitWindowSize(600, 600);
	glutCreateWindow("GRK-PROJECT WIP");
	glewInit();

	init();
	glutKeyboardFunc(keyboard);
	glutDisplayFunc(renderScene);
	glutIdleFunc(idle);

	glutMainLoop();

	shutdown();

	return 0;
}
