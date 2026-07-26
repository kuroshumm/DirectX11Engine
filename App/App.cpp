#include "App.h"
#include <d3dcompiler.h>
#include <vector>
#include "InputSystem.h"
#include "SkinnedModel.h"
#include "AnimationClip.h"

#pragma comment(lib, "d3dcompiler.lib")

namespace Engine
{
    App::App() : m_hWnd(nullptr) {}
    App::~App() {}

    HRESULT App::Initialize(HINSTANCE hInstance, int width, int height)
    {
        // --- フェーズ1: システムの構築 ---
        if (FAILED(InitWindow(hInstance, width, height))) return E_FAIL;

        m_pGraphics = std::make_unique<Graphics>();
        if (FAILED(m_pGraphics->Initialize(m_hWnd, width, height))) return E_FAIL;

        // --- フェーズ2: GPUリソースの構築 ---
        if (FAILED(m_pGraphics->InitConstantBuffer())) return E_FAIL;
        if (FAILED(m_pGraphics->InitPipelineStates())) return E_FAIL;
        if (FAILED(InitGeometry()))       return E_FAIL;

		m_pCamera = std::make_shared<Camera>();
        m_pCamera->GetTransform().SetPosition(0.0f, 5.0f, -15.0f);
		m_pCamera->GetTransform().SetRotation(0.3f, 0.0f, 0.0f);

        m_pSceneCameraOperator = std::make_unique<SceneCameraOperator>(m_pCamera);

        ShowWindow(m_hWnd, SW_SHOW);
        return S_OK;
    }

    HRESULT App::InitWindow(HINSTANCE hInstance, int width, int height)
    {
        // ウィンドウクラスの登録
        const wchar_t CLASS_NAME[] = L"EngineWindow";
        WNDCLASSEXW wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WindowProc;
        wcex.hInstance = hInstance;
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.lpszClassName = CLASS_NAME;
        RegisterClassExW(&wcex);

        // クライアント領域のサイズ調整
        RECT rc = { 0, 0, width, height };
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

        // ウィンドウの生成
        m_hWnd = CreateWindowExW(0, CLASS_NAME, L"DirectX 11 Game Engine", WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
            nullptr, nullptr, hInstance, this);

        return m_hWnd ? S_OK : E_FAIL;
    }

    HRESULT App::InitGeometry()
    {
		HRESULT hr = S_OK;

        hr = LoadShader(L"Shaders/PBRVertexShader.hlsl", L"Shaders/PBRPixelShader.hlsl");
        if (FAILED(hr))
        {
            return hr;
        }

		hr = LoadTexture(L"Model/Paladin_diffuse.png");
        if (FAILED(hr))
        {
            return hr;
		}

		m_pMaterial = std::make_shared<Material>();
        m_pMaterial->CreateBuffer(m_pGraphics->GetDevice());
        m_pMaterial->SetShader(m_pLambertianShader);
		m_pMaterial->SetTexture(m_pTexture);

        m_pModel = std::make_shared<Model>(m_pGraphics.get());
		m_pModel->SetMaterial(m_pMaterial);

        //hr = m_pModel->LoadMesh("Model/teapot.obj");
        /*hr = m_pModel->LoadMesh("Model/SwordMan.fbx");
        if (FAILED(hr))
        {
            return hr;
        }*/

        m_pSkinnedModel = std::make_shared<SkinnedModel>(m_pGraphics.get());
        m_pSkinnedModel->SetMaterial(m_pMaterial);
        hr = m_pSkinnedModel->LoadMesh("Model/SwordMan.fbx");
        if (FAILED(hr))
        {
            return hr;
        }

        m_pAnimator = std::make_shared<Animator>(m_pSkinnedModel);


        m_pAnimClip = std::make_shared<AnimationClip>("Animation/Sword And Shield Idle.fbx");
        m_pAnimator->AddState("Idle", m_pAnimClip, true);

        // --- 第11回追加: 天体系シーングラフの構築 ---
  //      m_pRootObject = std::make_shared<GameObject>();
  //      m_pRootObject->SetModel(m_pModel); // 同一のモデルインスタンスを共有

  //      m_pEarthObject = std::make_shared<GameObject>();
  //      m_pEarthObject->SetModel(m_pModel);

  //      m_pMoonObject = std::make_shared<GameObject>();
  //      m_pMoonObject->SetModel(m_pModel);

  //      // 親子関係の定義
  //      m_pRootObject->AddChild(m_pEarthObject);
  //      m_pEarthObject->AddChild(m_pMoonObject);

		//m_gameObjects.push_back(m_pRootObject);

        for (int i = 0; i < 1; ++i)
        {
            auto newObject = std::make_shared<GameObject>();
            newObject->SetModel(m_pSkinnedModel); // 既存のモデルデータを使い回す(メモリ節約)
            newObject->SetAnimator(m_pAnimator);
            newObject->InitIK();

            // -20.0f ～ 20.0f の間でランダムな座標を生成
            // カメラが Z = -15 の位置にあるため、目の前を避けて Z = 0 ～ 40 の奥の範囲に生成する
            float randX = (rand() % 4000) / 100.0f - 20.0f;   // -20.0f ～ 20.0f
            float randY = (rand() % 4000) / 100.0f - 20.0f;   // -20.0f ～ 20.0f
            float randZ = (rand() % 4000) / 100.0f;           // 0.0f ～ 40.0f に変更

            newObject->GetTransform().SetPosition(randX, randY, randZ);

            // モデルのサイズが大きすぎて画面を塞いでしまうのを防ぐため、縮小する
            newObject->GetTransform().SetScale(0.05f, 0.05f, 0.05f);

            // 描画対象リスト (リスト) に直接登録
            m_gameObjects.push_back(newObject);
        }

		return hr;
    }

    HRESULT App::LoadShader(const wchar_t* vsPath, const wchar_t* psPath)
    {
        HRESULT hr = S_OK;
        m_pLambertianShader = std::make_shared<Shader>();
        m_pLambertianShader->Initialize(m_pGraphics->GetDevice());

        // 頂点シェーダーのコンパイル
        hr = m_pLambertianShader->CompileVertexShader(vsPath, "VSMain");
        if (FAILED(hr))
        {
            return hr;
        }

        // ピクセルシェーダーのコンパイル
        hr = m_pLambertianShader->CompilePixelShader(psPath, "PSMain");
        if (FAILED(hr))
        {
            return hr;
        }

        // --- インプットレイアウトの定義 (修正箇所) ---
        // 第9回 HLSL の VS_INPUT シグネチャと厳密に一致させる必要がある
        hr = m_pLambertianShader->CreateInputLayout();

        return hr;
    }

    HRESULT App::LoadTexture(const wchar_t* filePath)
    {
		HRESULT hr = S_OK;

		m_pTexture = std::make_shared<Texture>();
		hr = m_pTexture->LoadFromFile(m_pGraphics->GetDevice(), filePath);

		return hr;
	}

    int App::Run()
    {
        MSG msg = {};
        while (msg.message != WM_QUIT)
        {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                Update(0.016f);
                Render();
            }
        }
        return (int)msg.wParam;
    }

    void App::Update(float dt)
    {
        m_pSceneCameraOperator->Update(dt);

        for (auto gameObject : m_gameObjects)
        {
            gameObject->Update(dt);
        }

        // 更新の一番最後に
		// マウスの変化量をリセットしておく 
		InputSystem& inputSystem = InputSystem::GetInstance();
		inputSystem.ResetMouseDelta();
    }

    void App::Render()
    {
        int drawnObjectsCount = 0; // 描画された数を数える変数

        m_pGraphics->BeginScene(0.0f, 0.125f, 0.3f, 1.0f);

		m_pGraphics->SetRender();
		m_pGraphics->SetTextureAndSampler();
        
		// アクティブなカメラのビュー行列とプロジェクション行列を取得
		DirectX::XMMATRIX view = m_pCamera->GetViewMatrix();
		DirectX::XMMATRIX proj = m_pCamera->GetProjectionMatrix();

        // カメラの視錐台を更新
		Frustum& frustum = m_pCamera->GetFrustum();
		frustum.Update(view, proj);

        for (auto gameObject : m_gameObjects)
        {
            if (!gameObject)
            {
                continue;
            }

			DirectX::XMFLOAT3 center = gameObject->GetWorldPosition();
			float radius = gameObject->GetBoundingSphereRadiusScaled();

            if (!frustum.CheckSphere(center, radius))
            {
				continue; // 視錐台外のオブジェクトは描画処理をスキップ
            }

            gameObject->Draw(m_pGraphics.get(), m_pCamera->GetViewMatrix(), m_pCamera->GetProjectionMatrix());

			drawnObjectsCount++; // 描画されたオブジェクトの数をカウント
        }

        m_pGraphics->EndScene();
    }

    LRESULT CALLBACK App::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_DESTROY)
        {
            PostQuitMessage(0);
            return 0;
        }

        InputSystem& inputSystem = InputSystem::GetInstance();
        switch (message)
        {
        case WM_KEYDOWN:
        case WM_KEYUP:
            inputSystem.UpdateKey(wParam, message == WM_KEYDOWN);
            break;

        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            inputSystem.UpdateKey(VK_MBUTTON, message == WM_MBUTTONDOWN);
            break;

        case WM_MOUSEMOVE:
            inputSystem.UpdateMouse(lParam);
            break;
        }

        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}