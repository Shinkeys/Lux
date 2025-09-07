#include "../../headers/scene/camera.h"
#include "../../headers/base/window.h"
#include "../../headers/util/constants.h"

#include <glm/gtc/matrix_transform.hpp>

Camera::Camera() : _view{glm::mat4(1.0f)}, _projection{glm::mat4(1.0f)}, _viewProjection{glm::mat4(1.0f)},
				   _position{glm::vec3(0.0f, 0.0f, 5.0f)}, _up{glm::vec3(0.0f, 1.0f, 0.0f)}, _forward{glm::vec3(0.0f, 0.0f, 1.0f)},
					_right{ glm::vec3(1.0f, 0.0f, 0.0f)}, _yaw{-90.0f}, _pitch{0.0f}, _speed{5.0f}, _nearPlane{Constants::cMinNearPlane}, 
					_farPlane{Constants::cMaxFarPlane}, _FOV{glm::radians(90.0f)}, _aspect{16.0f / 9.0f}
{
	CalculateVectors();
	_projection = glm::perspective(_FOV, _aspect, _nearPlane, _farPlane);
	CalculateMatrices();
}


void Camera::LookAt(glm::vec3 at)
{
	_view = glm::lookAt(_position, at, _up);

	_viewProjection = _projection * _view;
}

void Camera::CalculateMatrices()	
{
	_view = glm::lookAt(_position, _position + _forward, _up);
	_viewProjection = _projection * _view;
	_inverseProj = glm::inverse(_projection);
	_inverseView = glm::inverse(_view);
}

void Camera::Move(glm::vec3 direction)
{
	/*_position += */
}

void Camera::Update(const Window& window)
{
	const Mouse& mouseDesc = window.GetMouseDesc();
	
	const float yawOffset = mouseDesc.xOffset;
	const float pitchOffset = mouseDesc.yOffset;

	CalculateVectors();
	UpdateRotation(yawOffset, pitchOffset);
	UpdateOnKeyboard(window);
	CalculateMatrices();
}

void Camera::CalculateVectors()
{
	glm::vec3 frontDirection;
	constexpr float twoPi = 3.14159f * 2.0f;

	frontDirection.x = cos(twoPi * (_yaw / 360.0f)) * cos(twoPi * (_pitch / 360.0f));
	frontDirection.y = sin(twoPi * (_pitch / 360.0f));
	frontDirection.z = sin(twoPi * (_yaw / 360.0f)) * cos(twoPi * (_pitch / 360.0f));

	_forward = glm::normalize(frontDirection);
	_right = glm::normalize(glm::cross(_forward, glm::vec3(0.0f, 1.0f, 0.0f)));
	_up = glm::normalize(glm::cross(_right, _forward));
}

void Camera::UpdateRotation(float yawOffset, float pitchOffset)
{
	constexpr float mouseSpeed = 0.1f;

	_yaw -= yawOffset * mouseSpeed;
	_pitch += pitchOffset * mouseSpeed;

	if (_pitch > 89.0f)
		_pitch = 89.0f;
	if (_pitch < -89.0f)
		_pitch = -89.0f;
}

void Camera::UpdateOnKeyboard(const Window& window)
{
	const float deltaTime = window.GetDeltaTime();

	if (window.GetKeyStatus(SDL_SCANCODE_W))
		_position += _forward * _speed * deltaTime;
	if (window.GetKeyStatus(SDL_SCANCODE_S))
		_position -= _forward * _speed * deltaTime;

	if (window.GetKeyStatus(SDL_SCANCODE_A))
		_position -= _right * _speed * deltaTime;
	if (window.GetKeyStatus(SDL_SCANCODE_D))
		_position += _right * _speed * deltaTime;
}

