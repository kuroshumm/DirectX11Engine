#include "AnimatorSystem.h"

namespace Engine
{
	void AnimatorSystem::Update(entt::registry& registry, float dt)
	{
		auto view = registry.view<AnimatorComponent, AnimationComponent>();
		for (auto [entity, animator, animation] : view.each())
		{
			// 遷移中でない、前クリップが存在しない場合はスキップ
			if (!animator.isTransitioning || !animator.pPrevClip)
			{
				continue;
			}

			// 遷移タイマーの加算
			animator.transitionTimer += dt;

			// 旧クリップの再生時刻を裏で進める (勢い/モーメンタムの維持)
			animator.prevTime += dt * animator.pPrevClip->GetTicksPerSecond() * animation.playbackSpeed;
			if (animator.pPrevClip->GetDuration() > 0.0f)
			{
				animator.prevTime = std::fmod(animator.prevTime, animator.pPrevClip->GetDuration());
			}
			// 正規化ブレンド重み (0.0 〜 1.0) の算出
			animator.transitionWeight = std::clamp(
				animator.transitionTimer / animator.transitionDuration,
				0.0f,
				1.0f
			);

			// 遷移完了判定
			if (animator.transitionTimer >= animator.transitionDuration)
			{
				animator.isTransitioning = false;
				animator.pPrevClip = nullptr;
				animator.transitionWeight = 1.0f;
			}
		}
	}

	void AnimatorSystem::Play(
		entt::registry& registry,
		entt::entity entity,
		AnimStateType sateType)
	{
		if (!registry.all_of<AnimatorComponent, AnimationComponent>(entity))
		{
			return;
		}

		auto& animator  = registry.get<AnimatorComponent>(entity);
		auto& animation = registry.get<AnimationComponent>(entity);

		auto it = animator.stateMap.find(sateType);
		if (it == animator.stateMap.end() || !it->second.pAnimSeq)
		{
			return;
		}

		animator.currentState = sateType;

		// 即時切り替えのため、進行中の遷移はキャンセル
		animator.isTransitioning = false;
		animator.pPrevClip = nullptr;
		animator.transitionWeight = 1.0f;

		// AnimationComponentにクリップとループ設定を流し込む
		animation.pCurrentClip = it->second.pAnimSeq->clip;
		animation.isLoop = it->second.isLoop;
		animation.currentTime = 0.0f;
		animation.isPlaying = true;
	}

	void AnimatorSystem::CrossFade(
		entt::registry& registry,
		entt::entity entity,
		AnimStateType stateType,
		float duration
	)
	{
		if (!registry.all_of<AnimatorComponent, AnimationComponent>(entity))
		{
			return;
		}

		auto& animator = registry.get<AnimatorComponent>(entity);
		auto& animation = registry.get<AnimationComponent>(entity);

		// 遷移先ステートの取得
		auto it = animator.stateMap.find(stateType);
		if (it == animator.stateMap.end() || !it->second.pAnimSeq)
		{
			return;
		}

		// 既に遷移先のステートに移行済みの場合は何もしない
		if (animator.currentState == stateType && animation.pCurrentClip == it->second.pAnimSeq->clip)
		{
			return;
		}

		// 遷移前のパラメータ保存
		animator.prevState = animator.currentState;
		animator.pPrevClip = animation.pCurrentClip;
		animator.prevTime = animation.currentTime;
		animator.transitionDuration = (duration > 0.0f) ? duration : 0.0001f; // ゼロ除算防止
		animator.transitionTimer = 0.0f;
		animator.transitionWeight = 0.0f;
		// 前クリップが存在し、かつ新旧が異なる場合のみ遷移を有効化
		animator.isTransitioning = (animator.pPrevClip != nullptr && animator.pPrevClip != it->second.pAnimSeq->clip);

		// 遷移先のパラメータ設定
		animator.currentState = stateType;
		animation.pCurrentClip = it->second.pAnimSeq->clip;
		animation.currentTime = 0.0f;
		animation.isLoop = it->second.isLoop;
		animation.isPlaying = true;
	}
}
