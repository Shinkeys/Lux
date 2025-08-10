#pragma once
#include "../util/util.h"


class Camera;
class ISceneRenderer
{
private:


public:
	virtual ~ISceneRenderer() {}


	virtual void Update(const Camera& camera) = 0;
	virtual void Draw() = 0;

};