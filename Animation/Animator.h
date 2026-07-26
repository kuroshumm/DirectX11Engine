#pragma once
#include <unordered_map>
#include "SkinnedModel.h"
#include "AnimationClip.h"

namespace Engine
{
	struct AnimationState
	{
		std::string Name;
		std::shared_ptr<AnimationClip> Clip;
		bool isLoop;
	};

	class Animator
	{
	public:
		Animator(std::shared_ptr<SkinnedModel>& pModel);
		~Animator();

	public:
		void Update(float deltaTime);
		void Play(std::string stateName);
		void AddState(const std::string& stateName, const std::shared_ptr<AnimationClip>& pClip, bool isLoop);

	private:
		void UpdateTimer(float deltaTime, float duration);

		template<typename keyType>
		void CalcInterpolationFactor(
			const std::vector<keyType> keys,
			size_t& outLeftIndex,
			size_t& outRightIndex,
			float& outF
		) const;

	private:
		std::shared_ptr<SkinnedModel> m_pModel;

		std::unordered_map < std::string, AnimationState > m_stateMap;
		std::string m_currentStateName;
		float m_currentTime;
	};
}