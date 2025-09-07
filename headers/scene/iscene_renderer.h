#pragma once
#include "../util/util.h"


class Camera;
class Entity;
class ISceneRenderer
{
private:


public:
	virtual ~ISceneRenderer() {}


	virtual void Update(const Camera& camera) = 0;
	virtual void Draw() = 0;
	virtual void SubmitEntityToDraw(const Entity& entity) = 0;

};