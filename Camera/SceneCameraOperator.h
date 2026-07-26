#pragma once
#include <memory>
#include "Camera.h"

namespace Engine
{
	class SceneCameraOperator
	{
	public:
		SceneCameraOperator(std::weak_ptr<Camera> pCamera);
		~SceneCameraOperator() = default;

		void Update(float deltaTime);
		void Rotate(float deltaTime, const std::shared_ptr<Camera>& pCamera);
		void Move(float deltaTime, const std::shared_ptr<Camera>& pCamera);

	private:
		std::weak_ptr<Camera> m_pCamera;

		const float SENSITIVITY = 0.3f;
		const float moveSpped = 5.0f;
	};
}