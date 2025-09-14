#pragma once

#include "util.h"

#include <glm/glm.hpp>


// Based on paper: https://schneide.blog/2016/07/15/generating-an-icosphere-in-c/

struct Icosphere
{
	std::vector<glm::vec3> vertices;
	std::vector<u32> indices;
};

namespace mathhelpers
{
	using VertexList = std::vector<glm::vec3>;
	using TriangleList = std::vector<glm::uvec3>;
	using Lookup = std::map<std::pair<u32, u32>, u32>;

	u32 VertexForEdge(Lookup& lookup, VertexList& vertices, u32 first, u32 second);
	TriangleList Subdivide(VertexList& vertices, TriangleList triangles);
	Icosphere GenerateIcosphere(u32 subdivisions);
};