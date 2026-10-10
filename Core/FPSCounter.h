#pragma once
#include <chrono>
#include <string>

namespace Engine
{
    class FPSCounter
    {
    public:
        FPSCounter()
            : m_lastTime(std::chrono::high_resolution_clock::now())
            , m_fpsTimer(std::chrono::high_resolution_clock::now())
        {
        }

        /// <summary>
        /// 実時間（std::chrono）で正確に毎フレームの時間を計測します。
        /// </summary>
        void Update()
        {
            auto now = std::chrono::high_resolution_clock::now();

            // 前フレームからの経過時間 (ミリ秒)
            std::chrono::duration<float, std::milli> frameDelta = now - m_lastTime;
            m_lastTime = now;
            m_frameTimeMs = frameDelta.count();

            m_frameCount++;

            // 0.5 秒ごとに FPS の平均値を更新
            std::chrono::duration<float> elapsed = now - m_fpsTimer;
            if (elapsed.count() >= 0.5f)
            {
                m_fps = static_cast<float>(m_frameCount) / elapsed.count();
                m_frameCount = 0;
                m_fpsTimer = now;
            }
        }

        float GetFPS() const { return m_fps; }
        float GetFrameTimeMs() const { return m_frameTimeMs; }

        std::wstring GetFPSString(size_t entityCount = 0) const
        {
            std::wstring str = L"FPS: " + std::to_wstring(static_cast<int>(m_fps)) +
                L" (" + std::to_wstring(m_frameTimeMs).substr(0, 4) + L" ms)";
            if (entityCount > 0)
            {
                str += L"\nEntities: " + std::to_wstring(entityCount);
            }
            return str;
        }

    private:
        std::chrono::high_resolution_clock::time_point m_lastTime;
        std::chrono::high_resolution_clock::time_point m_fpsTimer;
        int   m_frameCount = 0;
        float m_fps = 0.0f;
        float m_frameTimeMs = 0.0f;
    };
}