#pragma once

#include "glm.hpp"

namespace Core
{
	glm::mat4 createPerspectiveMatrix(float zNear = 0.1f, float zFar = 100.0f, float frustumScale = 1.f);
	glm::mat4 createViewMatrix(glm::vec3 position, glm::vec3 forward, glm::vec3 up);
}