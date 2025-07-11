#pragma once 
#include "../util/util.h"

#include <glm/glm.hpp>


class Window;
class Camera
{
private:
	float _nearPlane;
	float _farPlane;
	float _FOV;
	float _yaw;
	float _pitch;

	float _speed;

	float _aspect;

	glm::mat4 _view;
	glm::mat4 _projection;
	glm::mat4 _inverseProj;
	glm::mat4 _viewProjection;

	glm::vec3  _position;

	glm::vec3 _up;
	glm::vec3 _forward;
	glm::vec3 _right;




	void UpdateOnKeyboard(const Window& window);
	void UpdateRotation(float yawOffset, float pitchOffset);
	void CalculateMatrices();
public:
	Camera();

	const glm::mat4& GetViewMatrix()           const { return _view; }
	const glm::mat4& GetProjectionMatrix()     const { return _projection; }
	const glm::mat4& GetViewProjectionMatrix() const { return _viewProjection; }
	const glm::mat4& GetInverseProjection()    const { return _inverseProj; }
	glm::vec3 GetPosition()                    const { return _position; }
	float GetNearPlane()                       const { return _nearPlane; }
	float GetFarPlane()                        const { return _farPlane; }
	

	void Update(const Window& window);
	void LookAt(glm::vec3 at);
	void Move(glm::vec3 direction);
	void CalculateVectors();
};