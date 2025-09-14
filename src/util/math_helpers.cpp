#include "../../headers/util/math_helpers.h"

namespace mathhelpers
{
	u32 VertexForEdge(Lookup& lookup, VertexList& vertices, u32 first, u32 second)
	{
		Lookup::key_type key(first, second);
		if (key.first > key.second)
			std::swap(key.first, key.second);

		auto inserted = lookup.insert({ key, (u32)vertices.size() });
		if (inserted.second)
		{
			auto& edge0 = vertices[first];
			auto& edge1 = vertices[second];
			auto point = glm::normalize(edge0 + edge1);
			vertices.push_back(point);
		}

		return inserted.first->second;
	}

	TriangleList Subdivide(VertexList& vertices,
		TriangleList triangles)
	{
		Lookup lookup;
		TriangleList result;

		for (auto&& each : triangles)
		{
			std::array<u32, 3> mid;
			for (int edge = 0; edge < 3; ++edge)
			{
				mid[edge] = VertexForEdge(lookup, vertices,
					each[edge], each[(edge + 1) % 3]);
			}

			result.push_back({ each[0], mid[0], mid[2] });
			result.push_back({ each[1], mid[1], mid[0] });
			result.push_back({ each[2], mid[2], mid[1] });
			result.push_back({ mid[0],  mid[1], mid[2] });
		}

		return result;
	}


	Icosphere GenerateIcosphere(u32 subdivisions)
	{
		// values are obtained via Pythagorean theorem.
		// You need 3 orthogonal rectangles to build a Icosahedron
		const float a = 0.5257311f;
		const float c = 0.8506508f;
		const float z = 0.0f;

		Icosphere sphere;
		sphere.vertices =
		{
			{-a,z,c}, {a,z,c}, {-a,z,-c}, {a,z,-c},
			{z,c,a}, {z,c,-a}, {z,-c,a}, {z,-c,-a},
			{c,a,z}, {-c,a, z}, {c,-a,z}, {-c,-a, z}
		};

		TriangleList icosahedronIndices =
		{
			{0, 4,  1}, {0,9, 4}, {9,5, 4}, {4, 5,8}, {4, 8, 1},
			{8, 10, 1}, {8,3,10}, {5,3, 8}, {5, 2,3}, {2, 7, 3},
			{7, 10, 3}, {7,6,10}, {7,11,6}, {11,0,6}, {0, 1, 6},
			{6, 1, 10}, {9,0,11}, {9,11,2}, {9, 2,5}, {7, 2, 11}
		};

		for (u32 i = 0; i < subdivisions; ++i)
		{
			icosahedronIndices = Subdivide(sphere.vertices, icosahedronIndices);
		}

		sphere.indices.resize(icosahedronIndices.size() * 3);
		memcpy(sphere.indices.data(), icosahedronIndices.data(), icosahedronIndices.size() * sizeof(icosahedronIndices[0]));

		return sphere;
	}
}