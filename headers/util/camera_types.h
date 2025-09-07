#pragma once

#include <glm/glm.hpp>

struct ViewData
{
	glm::mat4 view{ glm::mat4(1.0f) };
	glm::mat4 proj{ glm::mat4(1.0f) };
	glm::mat4 viewProj{ glm::mat4(1.0f) };
	glm::mat4 inverseProjection{ glm::mat4(1.0f) };
	glm::mat4 inverseView{ glm::mat4(1.0f) };
	glm::vec3 position{ glm::vec3(0.0f) };


	glm::ivec2 viewportExt{ glm::ivec2(0) };
	float nearPlane{ 0.0f };
	float farPlane{ 0.0f };
};