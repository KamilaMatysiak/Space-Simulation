#include <iostream>
#include <cmath>
#include <ctime>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "glew.h"
#include "freeglut.h"
#include "glm.hpp"
#include "ext.hpp"
#include "Physics.h"
#include "Shader_Loader.h"
#include "Camera.h"
#include "Texture.h"
#include "Object.h"
#include "model.h"
#include <WinUser.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static PxFilterFlags simulationFilterShader(PxFilterObjectAttributes attributes0,
	PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1,
	PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
	pairFlags =
		PxPairFlag::eCONTACT_DEFAULT | // default contact processing
		PxPairFlag::eNOTIFY_CONTACT_POINTS | // contact points will be available in onContact callback
		PxPairFlag::eNOTIFY_TOUCH_PERSISTS |
		PxPairFlag::eNOTIFY_TOUCH_FOUND; // onContact callback will be called for this pair

	return physx::PxFilterFlag::eDEFAULT;
}

class SimulationEventCallback : public PxSimulationEventCallback
{
public:
	void onContact(const PxContactPairHeader& pairHeader,
		const PxContactPair* pairs, PxU32 nbPairs)
	{
		// HINT: You can check which actors are in contact
		// using pairHeader.actors[0] and pairHeader.actors[1]
		auto ac = pairHeader.actors[0];
		auto ac2 = pairHeader.actors[1];
		/*if (ac->userData == renderables.back() || ac2->userData == renderables.back())
		{
			std::cout << "Liczba CP:" << nbPairs << std::endl;

			for (PxU32 i = 0; i < nbPairs; i++)
			{
				const PxContactPair& cp = pairs[i];

				// HINT: two get the contact points, use
				// PxContactPair::extractContacts

				std::vector<PxContactPairPoint> buffer;
				for (int i = 0; i < cp.contactCount; i++)
					buffer.push_back(PxContactPairPoint());

				cp.extractContacts(&buffer[0], sizeof(buffer));

				for (int i = 0; i < buffer.size(); i++)
				{
					auto position = buffer[i].position;
					std::cout << position.x << ' ' << position.y << ' ' << position.x << std::endl;
				}
			}
		}*/
	}
	virtual void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) {}
	virtual void onWake(PxActor** actors, PxU32 count) {}
	virtual void onSleep(PxActor** actors, PxU32 count) {}
	virtual void onTrigger(PxTriggerPair* pairs, PxU32 count) {}
	virtual void onAdvance(const PxRigidBody*const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) {}
};

// Initalization of physical scene (PhysX)
SimulationEventCallback simulationEventCallback;
Physics pxScene(0.0 /* gravity (m/s^2) */, simulationFilterShader,
	&simulationEventCallback);

// fixed timestep for stable and deterministic simulation
const double physicsStepTime = 1.f / 60.f;
double physicsTimeToProcess = 0;

int SCR_WIDTH = 1240;
int SCR_HEIGHT = 720;
int winId;
Core::Shader_Loader shaderLoader;

//shader programs
GLuint programTex;
GLuint programSkybox;
GLuint programSun;
GLuint programBlur;
GLuint programBloom;
GLuint programNormal;
GLuint programParticle;
GLuint programAsteroid;

//bloompart
unsigned int pingpongFBO[2];
unsigned int pingpongColorbuffers[2];
unsigned int FBO;
unsigned int colorBuffers[2];

//particlepart
GLuint VertexArrayID;
float lastTime;
static GLfloat* g_particule_position_size_data;
static GLubyte* g_particule_color_data;
GLuint particle_vertex_buffer;
GLuint particles_position_buffer;
GLuint particles_color_buffer;
bool bothEngines = true;

//textures
GLuint sunTexture;
GLuint earthTexture;
GLuint marsTexture;
GLuint moonTexture;
GLuint skyboxTexture;
GLuint particleTexture;

//assimp
std::shared_ptr<Model> cube;
std::shared_ptr<Model> sphere;
std::shared_ptr<Model> corvette;
std::shared_ptr<Model> asteroid;
std::shared_ptr<Model> crewmate;

//asteroids
GLuint bufferAsteroids;
int asteroidAmount = 100;

int engineLightTimer = 50;
float frustumScale = 1.f;

//camera
float cameraAngle = 0;
glm::vec3 cameraPos = glm::vec3(-6, 0, 0);
glm::vec3 cameraDir;
glm::vec3 cameraSide;
glm::vec3 cameraUp;
glm::mat4 cameraMatrix, perspectiveMatrix;

glm::vec3 sunPos = glm::vec3(10.0f, 0.0f, -5.0f);
glm::vec3 sunPos2 = glm::vec3(25.0f, -1.0f, 10.0f);

//physics
physx::PxShape* rectangleShape;
physx::PxShape* sphereShape;
physx::PxMaterial* material;
std::vector<physx::PxRigidDynamic*> dynamicObjects;
std::vector<physx::PxRigidStatic*> staticObjects;
physx::PxRigidDynamic* getActor(std::string name);
Object* findObject(std::string name);
glm::mat4 shipRotationMatrix;

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
struct Asteroid
{
	glm::mat4 model;
	glm::mat3 inv;
};

//vectors
std::vector<Object> objects;
std::vector<Light> lights;
std::vector<Asteroid> asteroids;
std::vector<glm::mat4> asteroidsMatrixes;
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

void keyboard(unsigned char key, int x, int y)
{
	auto actor = getActor("Corvette");
	auto move = actor->getLinearVelocity();
	physx::PxVec3 dir = physx::PxVec3(cameraDir.x, cameraDir.y, cameraDir.z);
	physx::PxVec3 up = physx::PxVec3(cameraUp.x, cameraUp.y, cameraUp.z);
	glm::vec3 cross = glm::cross(cameraDir, cameraUp);
	physx::PxVec3 dirCross = physx::PxVec3(cross.x, cross.y, cross.z);

	float angleSpeed = 0.1f;
	float moveSpeed = 0.1f;
	switch (key)
	{
	case 'q':
	{
		actor->setAngularVelocity(PxVec3(0.f, 1.f, 0.f));
		//cameraAngle += angleSpeed;
		lights[2].intensity = 0.05;
		engineLightTimer = 0;
		break;
	}

	case 'e':
	{
		//cameraAngle -= angleSpeed;
		actor->setAngularVelocity(PxVec3(0.f, -1.f, 0.f));
		lights[3].intensity = 0.05;
		engineLightTimer = 0;
		break;
	}

	case 'w':
	{
		actor->setLinearVelocity(move + dir);
		//cameraPos += cameraDir * moveSpeed;
		lights[2].intensity = 0.05;
		lights[3].intensity = 0.05;
		engineLightTimer = 0;
		break;
	}
	case 's': actor->setLinearVelocity(move - dir); break;
	//cameraPos -= cameraDir * moveSpeed; break;
	case 'd': actor->setLinearVelocity(move + dirCross); break;
	//cameraPos += glm::cross(cameraDir, glm::vec3(0, 1, 0)) * moveSpeed; break;
	case 'a': actor->setLinearVelocity(move - dirCross); break;
	//cameraPos -= glm::cross(cameraDir, glm::vec3(0, 1, 0)) * moveSpeed; break;
	case 'z': actor->setLinearVelocity(move + up); break;
	//cameraPos += glm::cross(cameraDir, glm::vec3(0, 0, 1)) * moveSpeed; break;
	case 'x': actor->setLinearVelocity(move - up); break;
	//cameraPos -= glm::cross(cameraDir, glm::vec3(0, 0, 1)) * moveSpeed; break;
	case 'r': actor->setAngularVelocity(PxVec3(1.0f, 0.0f, 0.0f)); break;
	case 't': actor->setAngularVelocity(PxVec3(0.0f, 0.0f, 1.0f)); break;
	case 'f': actor->setAngularVelocity(PxVec3(-1.0f, 0.0f, 0.0f)); break;
	case 'g': actor->setAngularVelocity(PxVec3(0.0f, 0.0f, -1.0f)); break;
	case ' ':
	{
		actor->setLinearVelocity(PxVec3(0, 0, 0));
		actor->setAngularVelocity(PxVec3(0, 0, 0));
		break;
	}
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

glm::mat4 orbitAsteroids(float time, glm::vec3 objectPos, glm::mat4 asteroidModelMatrix, glm::vec3 orbit);

void updateAsteroid()
{
	for (int i=0; i<asteroids.size();i++)
	{
		
		asteroids[i].model = orbitAsteroids(lastTime, sunPos, asteroidsMatrixes[i], glm::vec3(0.0f, 1.0f, 0.0f));
		asteroids[i].inv = glm::transpose(glm::inverse(glm::mat3(asteroids[i].model)));
	}
	glBindBuffer(GL_ARRAY_BUFFER, bufferAsteroids);
	glBufferData(GL_ARRAY_BUFFER, asteroidAmount * sizeof(Asteroid), &asteroids[0], GL_DYNAMIC_DRAW);

	for (unsigned int i = 0; i < asteroid->meshes.size(); i++)
	{
		unsigned int VAO = asteroid->meshes[i].VAO;
		glBindVertexArray(VAO);
		// set attribute pointers for matrix (4 times vec4)
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Asteroid), (void*)0);
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Asteroid), (void*)(sizeof(glm::vec4)));
		glEnableVertexAttribArray(7);
		glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(Asteroid), (void*)(2 * sizeof(glm::vec4)));
		glEnableVertexAttribArray(8);
		glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(Asteroid), (void*)(3 * sizeof(glm::vec4)));


		glEnableVertexAttribArray(9);
		glVertexAttribPointer(9, 3, GL_FLOAT, GL_FALSE, sizeof(Asteroid), (void*)(4 * sizeof(glm::vec4)));
		glEnableVertexAttribArray(10);
		glVertexAttribPointer(10, 3, GL_FLOAT, GL_FALSE, sizeof(Asteroid), (void*)(4 * sizeof(glm::vec4) + sizeof(glm::vec3)));
		glEnableVertexAttribArray(11);
		glVertexAttribPointer(11, 3, GL_FLOAT, GL_FALSE, sizeof(Asteroid), (void*)(4 * sizeof(glm::vec4) + 2 * sizeof(glm::vec3)));

		glVertexAttribDivisor(5, 1);
		glVertexAttribDivisor(6, 1);
		glVertexAttribDivisor(7, 1);
		glVertexAttribDivisor(8, 1);
		glVertexAttribDivisor(9, 1);
		glVertexAttribDivisor(10, 1);
		glVertexAttribDivisor(11, 1);

		glBindVertexArray(0);
	}
}

glm::mat4 createCameraMatrix()
{
	//cameraDir = glm::vec3(cosf(cameraAngle), 0.0f, sinf(cameraAngle));
	//glm::vec3 up = glm::vec3(0, 1, 0);


	Object* ship = findObject("Corvette");
	glm::mat4 shipModelMatrix = ship->GetMatrix();
	glm::mat4 offset = glm::translate(shipModelMatrix, glm::vec3(0, -2500, -8000));
	glm::mat4 cameraDirection = glm::translate(shipModelMatrix, glm::vec3(0, 0, 1000));
	glm::mat4 cameraUpwards = glm::translate(shipModelMatrix, glm::vec3(0, -1000, 0));
	cameraDir = glm::vec3(cameraDirection[3][0] - shipModelMatrix[3][0], cameraDirection[3][1] - shipModelMatrix[3][1], cameraDirection[3][2] - shipModelMatrix[3][2]);
	cameraPos = glm::vec3(offset[3][0], offset[3][1], offset[3][2]);
	cameraUp = glm::vec3(cameraUpwards[3][0] - shipModelMatrix[3][0], cameraUpwards[3][1] - shipModelMatrix[3][1], cameraUpwards[3][2] - shipModelMatrix[3][2]);
	cameraSide = glm::cross(cameraDir, cameraUp);

	//return Core::createViewMatrix(cameraPos, cameraDir, up);
	return glm::lookAt(cameraPos, ship->getPositionFromMatrix(glm::translate(shipModelMatrix, glm::vec3(0,-500,0))), cameraUp);
}

void drawAsteroids()
{
	glUseProgram(programAsteroid);
	glUniformMatrix4fv(glGetUniformLocation(programAsteroid, "projection"), 1, GL_FALSE, (float*)&perspectiveMatrix);
	glUniformMatrix4fv(glGetUniformLocation(programAsteroid, "view"), 1, GL_FALSE, (float*)&cameraMatrix);
	asteroid->DrawInstances(programAsteroid, asteroidAmount);
}

glm::mat4 orbitAsteroids(float time, glm::vec3 objectPos, glm::mat4 asteroidModelMatrix, glm::vec3 orbit)
{
	glm::mat4 orbitModelMatrix = glm::translate(objectPos);
	orbitModelMatrix = glm::rotate(orbitModelMatrix, time / 100, orbit);
	//orbitModelMatrix = glm::translate(asteroidModelMatrix, -objectPos) * orbitModelMatrix;
	
	return orbitModelMatrix * asteroidModelMatrix;
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
void drawSkybox(GLuint program, std::shared_ptr<Model> cubeModel, GLuint texID)
{
	glUseProgram(program);
	glDepthFunc(GL_LEQUAL);
	glm::mat4 transformation = perspectiveMatrix * glm::mat4(glm::mat3(cameraMatrix));
	glUniformMatrix4fv(glGetUniformLocation(program, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniform1i(glGetUniformLocation(program, "skybox"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texID);
	cubeModel->Draw(program);
	glDepthFunc(GL_LESS);
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
	glUniform3f(glGetUniformLocation(programParticle, "CameraUp_worldspace"), cameraUp.x, cameraUp.y, cameraUp.z);

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

void drawBloom()
{
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
}

Object* findObject(std::string name)
{
	for (int i = 0; i < objects.size(); i++)
	{
		if (objects[i].GetName() == name)
			return &objects[i];
	}
	return nullptr;
}

physx::PxRigidDynamic* getActor(std::string name)
{
	for (int i = 0; i < dynamicObjects.size(); i++)
	{
		if (((Object*)dynamicObjects[i]->userData)->GetName() == name)
			return dynamicObjects[i];
	}
	return nullptr;
}

void updateObjects()
{
	Object* obj = findObject("Corvette");
	glm::mat4 shipModelMatrix = obj->GetMatrix();
	//glm::translate(cameraPos + cameraDir * 0.7f + glm::vec3(0, -0.25f, 0)) * glm::rotate(-cameraAngle + glm::radians(90.0f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.0001f));
	//obj->modelM = shipModelMatrix;

	glm::mat4 engineLeft = glm::translate(shipModelMatrix, glm::vec3(450, 0, -1500));
	lights[2].position = glm::vec3(engineLeft[3][0], engineLeft[3][1], engineLeft[3][2]);

	glm::mat4 engineRight = glm::translate(shipModelMatrix, glm::vec3(-450, 0, -1500));
	lights[3].position = glm::vec3(engineRight[3][0], engineRight[3][1], engineRight[3][2]);

	Object *moon = findObject("Moon");
	Object *earth = findObject("Earth");

	auto earthPos = earth->findOrbit(lastTime / 5.0f, sunPos, glm::vec3(0, 1, 0), glm::vec3(-10.5f, 0.0f, -10.5f));
	earth->SetPosition(earthPos);
	earth->SetRotation(glm::vec3(0, 0, 1), lastTime);
	auto actorEarth = getActor("Earth");
	actorEarth->setKinematicTarget(PxTransform(PxVec3(earthPos.x, earthPos.y, earthPos.z)));

	auto moonPos = moon->findOrbit(lastTime, earthPos, glm::vec3(1,0,0), glm::vec3(0, 1.5, 0));
	moon->SetPosition(moonPos);
	moon->SetRotation(glm::vec3(1, 0, 0), lastTime / 5.0f);
	auto actorMoon = getActor("Moon");
	actorMoon->setKinematicTarget(PxTransform(PxVec3(moonPos.x, moonPos.y, moonPos.z)));

	Object *mars = findObject("Mars");	
	auto marsPos = mars->findOrbit(lastTime / 5.0f, sunPos2, glm::vec3(0, 1, 0), glm::vec3(-6.5f, 0.0f, -6.5f));
	mars->SetPosition(marsPos);
	mars->SetRotation(glm::vec3(0, 0, 1), lastTime / 2.0f);
}

void updatePhysics()
{
	auto actorFlags = PxActorTypeFlag::eRIGID_DYNAMIC;// | PxActorTypeFlag::eRIGID_STATIC;
	PxU32 nbActors = pxScene.scene->getNbActors(actorFlags);
	if (nbActors)
	{
		std::vector<PxRigidActor*> actors(nbActors);
		pxScene.scene->getActors(actorFlags, (PxActor**)&actors[0], nbActors);
		for (auto actor : actors)
		{
			// We use the userData of the objects to set up the model matrices
			// of proper renderables.
			if (!actor->userData) continue;
			Object *obj = (Object*)actor->userData;
			if (obj->isKinematic() == true)
				continue;
			// get world matrix of the object (actor)
			PxTransform transform = actor->getGlobalPose();
			//auto &c0 = transform.column0;
			//auto &c1 = transform.column1;
			//auto &c2 = transform.column2;
			//auto &c3 = transform.column3;

			physx::PxVec3 pxPos = transform.p;
			glm::vec3 pos(pxPos.x, pxPos.y, pxPos.z);
			physx::PxQuat rot = transform.q;
			glm::quat rotQuat = glm::quat(rot.x, rot.y, rot.z, rot.w);
			glm::mat4 rotationMatrix = glm::toMat4(rotQuat);
			if (obj->GetName() == "Corvette")
				shipRotationMatrix = rotationMatrix;
			
			glm::vec3 scale = obj->GetScale();
			glm::mat4 model = glm::translate(pos)*rotationMatrix*glm::scale(scale);

			// set up the model matrix used for the rendering
			obj->SetMatrix(model);/* = glm::mat4(
				c0.x, c0.y, c0.z, c0.w,
				c1.x, c1.y, c1.z, c1.w,
				c2.x, c2.y, c2.z, c2.w,
				c3.x, c3.y, c3.z, c3.w);
	*/	}
	}
}

void updateLights(GLuint program)
{
	for (int i = 0; i < lights.size(); i++)
	{
		std::string col = "pointLights[" + std::to_string(i) + "].color";
		std::string pos = "pointLights[" + std::to_string(i) + "].position";
		std::string ins = "pointLights[" + std::to_string(i) + "].intensity";
		glUniform3f(glGetUniformLocation(program, col.c_str()), lights[i].color.x, lights[i].color.y, lights[i].color.z);
		glUniform3f(glGetUniformLocation(program, pos.c_str()), lights[i].position.x, lights[i].position.y, lights[i].position.z);
		glUniform1f(glGetUniformLocation(program, ins.c_str()), lights[i].intensity);
	}
}

void renderScene()
{
	cameraMatrix = createCameraMatrix();
	perspectiveMatrix = Core::createPerspectiveMatrix(0.01f, 1000.0f, frustumScale);
	float time = glutGet(GLUT_ELAPSED_TIME) / 1000.f;
	double delta = time - lastTime;
	lastTime = time;

	if (delta < 1.f) {
		physicsTimeToProcess += delta;
		while (physicsTimeToProcess > 0) {
			pxScene.step(physicsStepTime);
			physicsTimeToProcess -= physicsStepTime;
		}
	}

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(programTex);
	glUniform1i(glGetUniformLocation(programTex,"LightsCount"), lights.size());
	updateLights(programTex);
	glUniform3f(glGetUniformLocation(programTex, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	glUseProgram(programNormal);
	glUniform1i(glGetUniformLocation(programNormal, "LightsCount"), lights.size());
	updateLights(programNormal);
	glUniform3f(glGetUniformLocation(programNormal, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	glUseProgram(programSun);
	glUniform3f(glGetUniformLocation(programSun, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	updatePhysics();
	updateObjects();

	//for (Object & obj : objects)
	//	drawObject(obj);

	for (Object &obj : objects)
		obj.Draw(perspectiveMatrix, cameraMatrix);

	//asteroidpart
	glUseProgram(programAsteroid);
	glUniform1i(glGetUniformLocation(programAsteroid, "LightsCount"), lights.size());
	updateLights(programAsteroid);
	glUniform3f(glGetUniformLocation(programAsteroid, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
	updateAsteroid();
	drawAsteroids();

	//particlepart
	glUseProgram(programParticle);
	glm::mat4 transformation = perspectiveMatrix * cameraMatrix;
	int newparticles = 0;

	if (engineLightTimer < 30)
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
		ParticlesContainer[particleIndex].life = 2.0f;
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
	

		float spread = 0.5;
		glm::vec3 maindir = -1 * cameraDir;
		glm::vec3 randomdir = glm::vec3(
			(rand() % 2000 - 1000.0f) / 5000.0f,
			(rand() % 2000 - 1000.0f) / 5000.0f,
			(rand() % 2000 - 1000.0f) / 5000.0f
		);

		ParticlesContainer[particleIndex].speed = maindir + randomdir * spread;
		ParticlesContainer[particleIndex].r = rand() % 100 + 100;
		ParticlesContainer[particleIndex].g = 0;
		ParticlesContainer[particleIndex].b = rand() % 100 + 50;
		ParticlesContainer[particleIndex].a = (rand() % 256) / 3;
		ParticlesContainer[particleIndex].size = (rand() % 1000) / 5000.0f + 0.01f;

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
				p.speed += glm::vec3(0.0f, 0.0f, 0.0f) * (float)delta * 0.5f;
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

	drawSkybox(programSkybox, cube, skyboxTexture);

	drawBloom();

	glutSwapBuffers();
}

void initPhysics()
{
	material = pxScene.physics->createMaterial(0.5, 0.5, 0.5);

	rectangleShape = pxScene.physics->createShape(PxBoxGeometry(1, 1, 1), *material);

	for (auto &obj : objects)
	{
		if (obj.isDynamic() == true)
		{
			glm::vec3 pos = obj.GetPosition();
			glm::vec3 rot = obj.GetRotation();
			auto dynamicObj = pxScene.physics->createRigidDynamic(PxTransform(pos.x, pos.y, pos.z));
			physx::PxMat44 mat;
			mat.setPosition(PxVec3(pos.x, pos.y, pos.z));
			mat.rotate(PxVec3(rot.x, rot.y, rot.z));
			dynamicObj->setGlobalPose(PxTransform(mat));
			dynamicObj->attachShape(*rectangleShape);
			dynamicObj->userData = &obj;
			dynamicObj->setLinearVelocity(physx::PxVec3(0, 0, 0));
			dynamicObj->setAngularVelocity(physx::PxVec3(0, 0, 0));
			dynamicObj->setAngularDamping(1.0);
			dynamicObj->setLinearDamping(1.0);
			pxScene.scene->addActor(*dynamicObj);

			dynamicObjects.push_back(dynamicObj);
		}
		else
		{
			sphereShape = pxScene.physics->createShape(PxSphereGeometry(obj.GetScale().x), *material);
			
			if (obj.isKinematic() == true)
			{
				glm::vec3 pos = obj.GetPosition();
				glm::vec3 rot = obj.GetRotation();
				auto dynamicObj = pxScene.physics->createRigidDynamic(PxTransform(pos.x, pos.y, pos.z));
				dynamicObj->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
				physx::PxMat44 mat;
				mat.setPosition(PxVec3(pos.x, pos.y, pos.z));
				mat.rotate(PxVec3(rot.x, rot.y, rot.z));
				dynamicObj->setGlobalPose(PxTransform(mat));
				dynamicObj->attachShape(*rectangleShape);
				dynamicObj->userData = &obj;
				dynamicObj->setLinearVelocity(physx::PxVec3(0, 0, 0));
				dynamicObj->setAngularVelocity(physx::PxVec3(0, 0, 0));
				dynamicObj->setAngularDamping(1.0);
				dynamicObj->setLinearDamping(1.0);
				pxScene.scene->addActor(*dynamicObj);

				dynamicObjects.push_back(dynamicObj);
			}
			else
			{
				glm::vec3 pos = obj.getPositionFromMatrix(obj.GetMatrix());
				staticObjects.emplace_back(pxScene.physics->createRigidStatic(PxTransform(pos.x, pos.y, pos.z)));
				physx::PxMat44 mat;
				mat.setPosition(PxVec3(pos.x, pos.y, pos.z));
				staticObjects.back()->setGlobalPose(PxTransform(mat));
				staticObjects.back()->attachShape(*sphereShape);
				staticObjects.back()->userData = &obj;
				pxScene.scene->addActor(*staticObjects.back());
			}
		sphereShape->release();
		}
	}
	rectangleShape->release();
}

void initAsteroids()
{
	int amount = asteroidAmount;
	float radius = 7.0;
	float offset = 2.0f;

	for (int i=0; i < amount; i++)
	{
		glm::mat4 model = glm::mat4(1.0f);
		float angle = (float)i / (float)amount * 360.0f;
		float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		
		float x = sin(angle) * radius + displacement;
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		
		float y = displacement * 0.1f;
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		
		float z = cos(angle) * radius + displacement;
		model = glm::translate(model, glm::vec3(x, y, z));

		float scale = (rand() % 20) / 100.0f + 0.05;
		model = glm::scale(model, glm::vec3(scale));

		float rotAngle = (rand() % 360);
		model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));
		Asteroid obj;
		obj.model = model;
		obj.inv = glm::transpose(glm::inverse(glm::mat3(model)));
		asteroidsMatrixes.push_back(model);
		asteroids.push_back(obj);
	}

	glGenBuffers(1, &bufferAsteroids);
	updateAsteroid();
}

void initParticles()
{
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);
	g_particule_position_size_data = new GLfloat[MaxParticles * 4];
	g_particule_color_data = new GLubyte[MaxParticles * 4];
	for (int i = 0; i < MaxParticles; i++) {
		ParticlesContainer[i].life = 1.0f;
		ParticlesContainer[i].cameradistance = -1.0f;
	}
	static const GLfloat g_vertex_buffer_data[] = {
		 -0.3f, -0.3f, 0.0f,
		  0.3f, -0.3f, 0.0f,
		 -0.3f,  0.3f, 0.0f,
		  0.3f,  0.3f, 0.0f,
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

void initBloom()
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

void initObjects()
{
	Object obj = Object("BigSun", sphere, sunTexture, programSun, glm::vec3(3.5f, 3.8f, 3.8f),
		sunPos, glm::vec3(0.1f), glm::vec3(3.0f, 3.0f, 3.0f), 0.0f, false, false);
	objects.push_back(obj);

	obj = Object("SmollSun", sphere, sunTexture, programSun, glm::vec3(0.9f, 0.9f, 2.0f), 
		sunPos2, glm::vec3(0.1f), glm::vec3(1), 0, false, false);
	objects.push_back(obj);

	Object planet = Object("Earth", sphere, earthTexture, programTex, glm::vec3(1.0f), 
		glm::vec3(-10.5f, 0.0f, -10.5f), glm::vec3(0.0f, 0.0f, 0.1f), glm::vec3(0.5f, 0.5f, 0.5f), 0, false, true);
	objects.push_back(planet);

	planet = Object("Mars", sphere, marsTexture, programTex, glm::vec3(1.0f), 
		glm::vec3(-6.5f, 0.0f, -6.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.4f), 0, false, true);
	objects.push_back(planet);

	Object moon = Object("Moon", sphere, moonTexture, programTex, glm::vec3(1.0f), 
		glm::vec3(0, 2, 2), glm::vec3(1.5f, 1.0f, 1.0f), glm::vec3(0.3f, 0.3f, 0.3f), 0, false, true);
	objects.push_back(moon);

	Object crewmateObj = Object("Space Humster", crewmate, programNormal, glm::vec3(1.0f), 
		glm::vec3(-5, 0, 0), glm::vec3(1, 0, 1), glm::vec3(0.1), 0, true, false);
	objects.push_back(crewmateObj);

	//glm::mat4 shipModelMatrix = glm::translate(cameraPos + cameraDir * 0.7f + glm::vec3(0, -0.25f, 0)) * glm::rotate(-cameraAngle + glm::radians(90.0f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.0001f));;
	Object ship = Object("Corvette", corvette, programNormal, glm::vec3(1.0f), 
		cameraPos+glm::vec3(-10,-0.3,0), glm::vec3(0, 1, 0), glm::vec3(0.0001f), 75, true, false);
	objects.push_back(ship);
}

void init()
{
	glEnable(GL_DEPTH_TEST);
	programTex = shaderLoader.CreateProgram("shaders/shader_tex.vert", "shaders/shader_tex.frag");
	programNormal = shaderLoader.CreateProgram("shaders/shader_normal.vert", "shaders/shader_normal.frag");
	programSkybox = shaderLoader.CreateProgram("shaders/shader_skybox.vert", "shaders/shader_skybox.frag");
	programSun = shaderLoader.CreateProgram("shaders/shader_sun.vert", "shaders/shader_sun.frag");
	programBlur = shaderLoader.CreateProgram("shaders/shader_blur.vert", "shaders/shader_blur.frag");
	programBloom = shaderLoader.CreateProgram("shaders/shader_bloom.vert", "shaders/shader_bloom.frag");
	programParticle = shaderLoader.CreateProgram("shaders/shader_particle.vert", "shaders/shader_particle.frag");
	programAsteroid = shaderLoader.CreateProgram("shaders/shader_asteroid.vert", "shaders/shader_asteroid.frag");

	glUseProgram(programBlur);
	glUniform1i(glGetUniformLocation(programBlur, "image"), 0);
	glUseProgram(programBloom);
	glUniform1i(glGetUniformLocation(programBloom, "scene"), 0);
	glUniform1i(glGetUniformLocation(programBloom, "bloomBlur"), 1);
	glUniform2f(glGetUniformLocation(programBloom, "screenSize"), 1.0f / SCR_WIDTH, 1.0f / SCR_HEIGHT);
	glUseProgram(0);

	corvette = std::make_shared<Model>("models/Corvette-F3.obj");
	crewmate = std::make_shared<Model>("models/space_humster.obj");
	asteroid = std::make_shared<Model>("models/Asteroid_X.obj");
	sphere = std::make_shared<Model>("models/sphere.obj");
	cube = std::make_shared<Model>("models/cube.obj");

	sunTexture = Core::LoadTexture("textures/sun.png");
	earthTexture = Core::LoadTexture("textures/earth2.png");
	moonTexture = Core::LoadTexture("textures/moon.png");
	particleTexture = Core::LoadTexture("textures/sun.png");
	marsTexture = Core::LoadTexture("models/textures/Mars/2k_mars.png");
	skyboxTexture = loadCubemap(faces);

	initParticles();
	initBloom();
	initObjects();
	initAsteroids();
	initPhysics();

	Light l1;
	l1.position = sunPos;
	l1.color = glm::vec3(0.8f, 0.8f, 0.7f);
	l1.intensity = 105;
	lights.push_back(l1);

	Light l2;
	l2.position = sunPos2;
	l2.color = glm::vec3(0.5f, 0.5f, 0.5f);
	l2.intensity = 55;
	lights.push_back(l2);

	Light l3;
	l3.position = glm::vec3(0);
	l3.color = glm::vec3(1.0f, 0.0f, 0.0f);
	l3.intensity = 0.001;
	lights.push_back(l3);

	Light l4;
	l4.position = glm::vec3(0);
	l4.color = glm::vec3(1.0f, 0.0f, 0.0f);
	l4.intensity = 0.001;
	lights.push_back(l4);

}

void shutdown()
{
	shaderLoader.DeleteProgram(programSun);
	shaderLoader.DeleteProgram(programParticle);
	shaderLoader.DeleteProgram(programNormal);
	shaderLoader.DeleteProgram(programAsteroid);
	shaderLoader.DeleteProgram(programTex);
	shaderLoader.DeleteProgram(programBloom);
	shaderLoader.DeleteProgram(programBlur);
	shaderLoader.DeleteProgram(programSkybox);
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
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(SCR_WIDTH, SCR_HEIGHT);
	winId = glutCreateWindow("GRK-PROJECT WIP");
	glutFullScreen();
	glewInit();

	init();
	glutKeyboardFunc(keyboard);

	glutDisplayFunc(renderScene);
	glutIdleFunc(idle);

	glutReshapeFunc(onReshape);
	glutMainLoop();

	shutdown();

	return 0;
}
