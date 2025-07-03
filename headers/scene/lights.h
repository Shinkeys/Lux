#pragma once

#include <glm/glm.hpp>


struct PointLight
{
	glm::vec3 position{ glm::vec3(0.0f) };
	glm::vec3 color{ glm::vec3(1.0f) };
	float intenstity{ 0.0f };
	float radius{ 1.0f };
};