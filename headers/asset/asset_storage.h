#pragma once
#include "../util/util.h"
#include "../scene/component.h"

#include <glm/glm.hpp>

struct Geometry
{
	glm::vec3 position{glm::vec3(0.0f)};
	glm::vec3 normals{ glm::vec3(0.0f) };
	glm::vec3 tangents{ glm::vec3(0.0f) };
	glm::vec2 UVs{ glm::vec2(0.0f) };
};

struct Mesh
{
	TranslationComponent* transform;
	Geometry* geometryPtr;
	//ptr* materialPtr;
};

class AssetStorage
{
private:
	std::vector<Geometry> _allGeometryStorage;
public:
	Geometry* GetRawData()  { return _allGeometryStorage.data(); }
	size_t GetRawDataSize() const { return _allGeometryStorage.size(); }
	void StoreGeometry(const std::vector<Geometry>& geometry);
};