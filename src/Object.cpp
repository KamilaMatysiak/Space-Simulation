#include "Object.h"

void Object::Draw(glm::mat4 perspectiveMatrix, glm::mat4 cameraMatrix)
{
	glUseProgram(shaderID);

	glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelM;

	glUniformMatrix4fv(glGetUniformLocation(shaderID, "modelMatrix"), 1, GL_FALSE, (float*)&modelM);
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "transformation"), 1, GL_FALSE, (float*)&transformation);

	glUniform3f(glGetUniformLocation(shaderID, "objectColor"), color.r, color.g, color.b);
	if (textureID != -1)
		Core::SetActiveTexture(textureID, "diffuseTexture", shaderID, 0);

	modelParent->Draw(shaderID);
	glUseProgram(0);
}

void Object::ChangePosition(glm::vec3 movement)
{
	modelM = glm::translate(modelM, movement);
	this->position += movement;
}

void Object::SetPosition(glm::vec3 position)
{
	this->position = position;
	SetMatrix();
}

void Object::ChangeScale(glm::vec3 scale)
{
	modelM = glm::scale(modelM, scale);
	this->scale += scale;
}

void Object::SetScale(glm::vec3 scale)
{
	this->scale = scale;
	SetMatrix();
}

void Object::ChangeRotation(glm::vec3 rotate, float angle)
{
	modelM = glm::rotate(modelM, angle, rotate);
	this->rotation += rotate;
	this->angle += angle;
}

void Object::SetRotation(glm::vec3 rotate, float angle)
{
	this->rotation = rotate;
	this->angle = angle;
	SetMatrix();

}

void Object::SetMatrix()
{
	modelM = glm::translate(glm::mat4(1.0f), position);
	modelM = glm::rotate(modelM, angle, rotation);
	modelM = glm::scale(modelM, scale);
	invModelM = glm::inverseTranspose(modelM);
}

void Object::SetMatrix(glm::vec3 position, glm::vec3 scale, glm::vec3 rotate, float angle)
{
	this->position = position;
	this->rotation = rotate;
	this->scale = scale;
	this->angle = angle;
	SetMatrix();
}

Object::Object(	std::string name,
				std::shared_ptr<Model> modelParent,
				GLuint textureID,
				GLuint shaderID,
				glm::vec3 color,
				glm::vec3 position,
				glm::vec3 rotation,
				glm::vec3 scale,
				float angle,
				bool dynamic)
{
	this->name = name;
	SetMatrix(position, scale, rotation, angle);
	this->modelParent = modelParent;
	this->textureID = textureID;
	this->shaderID = shaderID;
	this->color = color;
	this->dynamic = dynamic;
}

Object::Object(	std::string name,
				std::shared_ptr<Model> modelParent,
				GLuint shaderID,
				glm::vec3 color,
				glm::vec3 position,
				glm::vec3 rotation,
				glm::vec3 scale,
				float angle,
				bool dynamic)
{
	this->name = name;
	SetMatrix(position, scale, rotation, angle);
	this->textureID = -1;
	this->modelParent = modelParent;
	this->shaderID = shaderID;
	this->color = color;
	this->dynamic = dynamic;
}


glm::vec3 Object::getScaleFromMatrix(glm::mat4 modelMatrix)
{
	float x = glm::length(glm::vec3(modelMatrix[0][0], modelMatrix[1][0], modelMatrix[2][0]));
	float y = glm::length(glm::vec3(modelMatrix[0][1], modelMatrix[1][1], modelMatrix[2][1]));
	float z = glm::length(glm::vec3(modelMatrix[0][2], modelMatrix[1][2], modelMatrix[2][2]));

	return glm::vec3(x, y, z);
}

glm::vec3 Object::getPositionFromMatrix(glm::mat4 modelMatrix)
{
	return glm::vec3(modelMatrix[3][0], modelMatrix[3][1], modelMatrix[3][2]);
}

glm::vec3 Object::findOrbit(float time, glm::vec3 center, glm::vec3 orbit, glm::vec3 radius)
{
	glm::mat4 planetModelMatrix = glm::mat4(1.0f);
	planetModelMatrix = glm::translate(planetModelMatrix, center);
	planetModelMatrix = glm::rotate(planetModelMatrix, time, orbit);
	planetModelMatrix = glm::translate(planetModelMatrix, radius);
	return getPositionFromMatrix(planetModelMatrix);
}