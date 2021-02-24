#pragma once
#include <iostream>
#include <cmath>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>

#include "glew.h"
#include "freeglut.h"
#include "glm.hpp"
#include "ext.hpp"
#include "model.h"
#include "Texture.h"


class Object 
{
public:


	void Draw(glm::mat4 perspectiveMatrix, glm::mat4 cameraMatrix);
	void ChangePosition(glm::vec3 movement);
	void SetPosition(glm::vec3 position);

	void ChangeScale(glm::vec3 scale);
	void SetScale(glm::vec3 scale);

	void ChangeRotation(glm::vec3 rotate, float angle);
	void SetRotation(glm::vec3 rotate, float angle);

	void SetMatrix(glm::vec3 position, glm::vec3 scale, glm::vec3 rotate, float angle);
	void SetMatrix(glm::mat4 _mat) { this->modelM = _mat;
									this->invModelM = glm::inverseTranspose(_mat); }

	void manualSetRotationM(glm::mat4 rot) {this->rotationM = rot;}
	void manualSetPosition(glm::vec3 pos) { this->position = pos; }
	glm::mat4 getRotationM() { return rotationM; }

	std::string GetName() { return this->name; }
	glm::mat4 GetMatrix() { return this->modelM; }
	glm::mat4 GetInvMatrix() { return this->invModelM; }
	glm::vec3 GetColor() { return this->color; }
	glm::vec3 GetPosition() { return position; }
	glm::vec3 GetRotation() { return rotation; }
	glm::vec3 GetScale() { return scale;  }
	bool isDynamic() { return dynamic; }
	bool isKinematic() { return kinematic; }
	std::shared_ptr<Model> GetParent() { return modelParent; }

	Object(	std::string name, std::shared_ptr<Model> modelParent, GLuint textureID, GLuint shaderID, glm::vec3 color,
						glm::vec3 position,	glm::vec3 rotation, glm::vec3 scale, float angle, bool dynamic, bool kinematic);

	Object(std::string name, std::shared_ptr<Model> modelParent, GLuint shaderID, glm::vec3 color, glm::vec3 position,
		glm::vec3 rotation, glm::vec3 scale, float angle, bool dynamic, bool kinematic);

	bool exists = true;
	glm::vec3 getScaleFromMatrix(glm::mat4 modelMatrix);
	glm::vec3 getPositionFromMatrix(glm::mat4 modelMatrix);
	glm::vec3 findOrbit(float time, glm::vec3 center, glm::vec3 orbit, glm::vec3 radius);

private:

	void SetMatrix();
	std::shared_ptr<Model> modelParent;
	std::string name;
	glm::mat4 modelM;
	glm::mat4 invModelM;
	glm::mat4 rotationM;
	GLuint textureID;
	GLuint shaderID;
	glm::vec3 color;
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
	float angle;
	bool dynamic;
	bool kinematic;

};