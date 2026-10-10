#pragma once
#include <algorithm>

namespace Engine
{
	class GameTime
	{
	public:
		inline static float deltaTime  = 0.0016f;  // 前フレームの経過時間
		inline static float time	   = 0.0f;	   // アプリ起動からの累計時間
		inline static float timeScale  = 1.0f;	   // 時間の倍率
		inline static unsigned int frameCount = 0; // 累計フレーム数

		// 毎フレーム冒頭で App::Run から1回だけ呼ばれる更新関数
		static void Update(float rawDeltaTime)
		{
			// 上限クランプ（0.1秒 = 最低でも10FPS相当で頭打ちにして発散を防ぐ）
			float clampedDt = (std::min)(rawDeltaTime, 0.1f);
			// timeScale を乗算した時間を deltaTime とする
			deltaTime = clampedDt * timeScale;
			time += deltaTime;
			frameCount++;
		}
	};
}