#pragma once
#include <windows.h>
#include <memory>
#include <wrl/client.h>
#include "Graphics.h"
#include "Mesh.h"
#include "Common.h"
#include "Model.h"
#include "GameObject.h"
#include "Camera.h"
#include "SceneCameraOperator.h"

namespace Engine
{
    class SkinnedModel;

    class App
    {
    public:
        App();
        ~App();

        // メイン初期化関数
        HRESULT Initialize(HINSTANCE hInstance, int width, int height);
        int Run();

    private:
        // 分割された初期化用プライベートメソッド
        HRESULT InitWindow(HINSTANCE hInstance, int width, int height);
        HRESULT InitGeometry();
        HRESULT LoadShader(const wchar_t* vsPath, const wchar_t* psPath);
		HRESULT LoadTexture(const wchar_t* filePath);

        static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
        void Update(float dt);
        void Render();

        HWND m_hWnd;
        std::unique_ptr<Graphics>     m_pGraphics;
        std::shared_ptr<Model>        m_pModel;
        std::shared_ptr<SkinnedModel> m_pSkinnedModel;
        std::shared_ptr<Animator>     m_pAnimator;
        std::shared_ptr<AnimationClip> m_pAnimClip;
        std::shared_ptr<Material>     m_pMaterial;
		std::shared_ptr<Texture>      m_pTexture;
        std::shared_ptr<Shader>       m_pLambertianShader;

		std::vector<std::shared_ptr<GameObject>> m_gameObjects; // シーングラフ全体の管理用コンテナ

        // --- 第11回追加: シーングラフとカメラ行列 ---
        //std::shared_ptr<GameObject> m_pRootObject;  // ルートノード (太陽)
        //std::shared_ptr<GameObject> m_pEarthObject; // 第一階層 (地球)
        //std::shared_ptr<GameObject> m_pMoonObject;  // 第二階層 (月)

		// --- 第13回追加: カメラクラスの導入 ---
		std::shared_ptr<Camera> m_pCamera;

        std::unique_ptr<SceneCameraOperator> m_pSceneCameraOperator;
    };
}