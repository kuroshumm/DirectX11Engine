#include <d3dcompiler.h>
#include <vector>
#include <algorithm>
#include <chrono>

#include "App.h"
#include "InputSystem.h"
#include "AnimationClip.h"
#include "Components.h"
#include "GameTime.h"
#include "PrimitiveFactory.h"

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

        m_registry = std::make_unique<entt::registry>();

        // --- フェーズ2: GPUリソースの構築 ---
        if (FAILED(m_pGraphics->InitConstantBuffer())) return E_FAIL;
        if (FAILED(m_pGraphics->InitPipelineStates())) return E_FAIL;
        if (FAILED(InitGeometry()))       return E_FAIL;

        InitSceneCamera();

        PrimitiveFactory::Initialize(m_pGraphics.get(), m_pStaticPBRSHader);
        PrimitiveFactory::CreatePrimitive(
            *m_registry,
            PrimitiveType::Plane,
            DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f) // 陰影が視認しやすいダークグレー
        );


        m_pTextRenderer = std::make_unique<TextRenderer>();
        m_pTextRenderer->Initialize(m_pGraphics->GetSwapChain());

        m_renderSystem = std::make_unique<RenderSystem>();
        m_transformSystem = std::make_unique<TransformSystem>();
        m_movementSystem = std::make_unique<MovementSystem>();
        m_animationSystem = std::make_unique<AnimationSystem>();
        m_animatorSystem = std::make_unique<AnimatorSystem>();
        m_boneTransformSystem = std::make_unique<BoneTransformSystem>();
        m_bonePaletteUploadSystem = std::make_unique<BonePaletteUploadSystem>();
        m_ikSystem = std::make_unique<IKSystem>();
        m_hierarchyTransformSystem = std::make_unique<HierarchyTransformSystem>();
        m_cameraControllerSystem = std::make_unique<CameraControllerSystem>();
        m_cameraSystem = std::make_unique < CameraSystem >();
        m_thirdPesronCameraSysten = std::make_unique<ThirdPersonCameraSystem>();

        m_renderSystem->Initialize(m_pGraphics->GetDevice());

        if (FAILED(InitPlayer()))         return E_FAIL;
        InitPlayerCamera();

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
        m_pLambertianShader = std::make_shared<Shader>();
        hr = LoadShader(m_pLambertianShader, L"Shaders/PBRVertexShader.hlsl", L"Shaders/PBRPixelShader.hlsl");
        if (FAILED(hr))
        {
            return hr;
        }

        m_pStaticPBRSHader = std::make_shared<Shader>();
        hr = LoadShader(m_pStaticPBRSHader, L"Shaders/StaticPBRVertexShader.hlsl", L"Shaders/PBRPixelShader.hlsl");
        if (FAILED(hr))
        {
            return hr;
        }

		hr = LoadTexture(L"Model/Paladin_diffuse.png");
        if (FAILED(hr))
        {
            return hr;
		}

        hr = LoadAnimationClip(m_pAnimClipIdle, "Animation/SAS_Idle.fbx");
        if (FAILED(hr))
        {
            return hr;
        }
        hr = LoadAnimationClip(m_pAnimClipWalk, "Animation/SAS_Walk.fbx");
        if (FAILED(hr))
        {
            return hr;
        }
        hr = LoadAnimationClip(m_pAnimClipRun, "Animation/SAS_Run.fbx");
        if (FAILED(hr))
        {
            return hr;
        }

		return hr;
    }

    HRESULT App::LoadShader(std::shared_ptr<Shader> pShader, const wchar_t* vsPath, const wchar_t* psPath)
    {
        HRESULT hr = S_OK;
        pShader->Initialize(m_pGraphics->GetDevice());

        // 頂点シェーダーのコンパイル
        hr = pShader->CompileVertexShader(vsPath, "VSMain");
        if (FAILED(hr))
        {
            return hr;
        }

        // ピクセルシェーダーのコンパイル
        hr = pShader->CompilePixelShader(psPath, "PSMain");
        if (FAILED(hr))
        {
            return hr;
        }

        // --- インプットレイアウトの定義 (修正箇所) ---
        // 第9回 HLSL の VS_INPUT シグネチャと厳密に一致させる必要がある
        hr = pShader->CreateSkinnrdInputLayout();

        return hr;
    }

    HRESULT App::LoadTexture(const wchar_t* filePath)
    {
		HRESULT hr = S_OK;

		m_pTexture = std::make_shared<Texture>();
		hr = m_pTexture->LoadFromFile(m_pGraphics->GetDevice(), filePath);

		return hr;
	}

    HRESULT App::LoadAnimationClip(std::shared_ptr<AnimationClip>& pAnimClip, const std::string& filePath)
    {
        HRESULT hr = S_OK;

        pAnimClip = std::make_shared<AnimationClip>(filePath);

        return hr;
    }

    int App::Run()
    {
        MSG msg = {};
        auto lastTime = std::chrono::high_resolution_clock::now();

        while (msg.message != WM_QUIT)
        {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                // 実時間の計測
                auto currentTime = std::chrono::high_resolution_clock::now();
                std::chrono::duration<float> elapsed = currentTime - lastTime;
                lastTime = currentTime;
                // Timeクラスの更新（ここで Time::deltaTime が確定する）
                GameTime::Update(elapsed.count());
                
                // 各処理の実行（引数に Time::deltaTime を渡してもよいし、中で直接参照してもよい）
                Update(GameTime::deltaTime);
                Render();
            }
        }
        return (int)msg.wParam;
    }

    void App::Update(float dt)
    {
        m_fpsCounter.Update();

        // カメラの更新（視錐台の計算）
        m_cameraControllerSystem->Update(*m_registry, dt);
        m_thirdPesronCameraSysten->Update(*m_registry, dt);
        m_cameraSystem->Update(*m_registry);

        // カリング判定
        m_cullingSystem->Update(*m_registry.get());

        m_movementSystem->Update(*m_registry.get(), dt);

        m_animatorSystem->Update(*m_registry.get(), dt);
        m_animationSystem->Update(*m_registry.get(), dt);
        
        m_boneTransformSystem->Update(*m_registry.get());

        m_ikSystem->Update(*m_registry.get(), dt);
        m_hierarchyTransformSystem->Update(*m_registry.get()); // 階層トランスフォーム更新
        m_transformSystem->Update(*m_registry.get());
        
        m_bonePaletteUploadSystem->Update(*m_registry.get(), *m_pGraphics->GetContext());

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

        m_renderSystem->Render(*m_registry.get(), *m_pGraphics);

        if (m_pTextRenderer)
        {
            m_pTextRenderer->RenderTextRightTop(m_fpsCounter.GetFPSString(1000));
        }
        
        m_pGraphics->EndScene();
    }

    HRESULT App::InitPlayer()
    {
        m_modelLoader = std::make_unique<ModelLoader>(m_pGraphics.get());
        ModelLoader::LoadResult loadResult = m_modelLoader->Load("Model/SwordMan.fbx");
        if (!loadResult.isSuccess)
        {
            return E_FAIL;
        }

        entt::entity entity = m_registry->create();

        TransformComponent transformComp;
        transformComp.position = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        transformComp.rotation = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        transformComp.scale = DirectX::XMFLOAT3(0.05f, 0.05f, 0.05f);
        m_registry->emplace<TransformComponent>(entity, std::move(transformComp));

        ModelComponent modelComp;
        modelComp.pModelAsset = loadResult.modelAsset;
        m_registry->emplace<ModelComponent>(entity, std::move(modelComp));

        SkeletonComponent skeltonComp;
        skeltonComp.bones = loadResult.skeltonAsset->initialBones;
        skeltonComp.rootBoneIndex = loadResult.skeltonAsset->rootBoneIndex;
        m_registry->emplace<SkeletonComponent>(entity, std::move(skeltonComp));

        SkinningPaletteComponent skinningPaletteComp;
        ModelLoader::CreatePaletteBuffer(m_pGraphics->GetDevice(), skinningPaletteComp.pPaletteBuffer);
        m_registry->emplace<SkinningPaletteComponent>(entity, std::move(skinningPaletteComp));

        AnimationComponent animationComp;
        m_registry->emplace<AnimationComponent>(entity, std::move(animationComp));

        AnimatorComponent animatorComp;
        animatorComp.AddState("Idle", m_pAnimClipIdle, true);
        animatorComp.AddState("Walk", m_pAnimClipWalk, true);
        animatorComp.AddState("Run", m_pAnimClipRun, true);
        m_registry->emplace<AnimatorComponent>(entity, std::move(animatorComp));

        m_animatorSystem->Play(*m_registry.get(), entity, "Idle");

        IKComponent ikComp;
        int upLegIndex = loadResult.skeltonAsset->GetBoneIndex("mixamorig:LeftUpLeg");
        int legIndex = loadResult.skeltonAsset->GetBoneIndex("mixamorig:LeftLeg");
        int footIndex = loadResult.skeltonAsset->GetBoneIndex("mixamorig:LeftFoot");
        if (upLegIndex != -1 && legIndex != -1 && footIndex != -1)
        {
            ikComp.effectorBoneIndex = footIndex;
            ikComp.chainIndices = { upLegIndex, legIndex, footIndex };
            // ヒザ関節の 1D 角度制限 (X 軸周りに 0°〜 120°)
            ikComp.SetConstraint(legIndex, DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), 0.0f, 120.0f);
            m_registry->emplace<IKComponent>(entity, std::move(ikComp));
        }

        MaterialComponent material;
        material.pShader = m_pLambertianShader;
        material.pTexture = m_pTexture;
        m_registry->emplace<MaterialComponent>(entity, std::move(material));

        // LOD
        m_registry->emplace<LODComponent>(entity);

        m_registry->emplace<PlayerTag>(entity);

        return S_OK;
    }

    void App::InitSceneCamera()
    {
        entt::entity cameraEntity = m_registry->create();

        // 1. TransformComponent (初期位置・初期向き)
        TransformComponent camTransform;
        camTransform.position = DirectX::XMFLOAT3(0.0f, 5.0f, -15.0f);
        // 初期ピッチ角 0.3 rad のクォータニオン
        DirectX::XMVECTOR qRot = DirectX::XMQuaternionRotationRollPitchYaw(0.3f, 0.0f, 0.0f);
        DirectX::XMStoreFloat4(&camTransform.rotation, qRot);
        camTransform.scale = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
        m_registry->emplace<TransformComponent>(cameraEntity, std::move(camTransform));
        
        // 2. CameraComponent (投影パラメータ)
        CameraComponent camComponent;
        camComponent.fov = DirectX::XM_PIDIV4; // 45度
        camComponent.aspectRatio = 800.0f / 600.0f; // またはクライアント領域サイズ比
        camComponent.nearZ = 0.1f;
        camComponent.farZ = 1000.0f;
        camComponent.isProjectionDirty = true;
        m_registry->emplace<CameraComponent>(cameraEntity, std::move(camComponent));
        
        // 3. FreeLookCameraControllerComponent (操作パラメータと初期オイラー角)
        FreeLookCameraControllerComponent camController;
        camController.moveSpeed = 50.0f;
        camController.sensitivity = 0.3f;
        camController.pitch = 0.3f; // camTransform の初期ピッチと一致させる
        camController.yaw = 0.0f;
        m_registry->emplace<FreeLookCameraControllerComponent>(cameraEntity, std::move(camController));

        // 4. ActiveCameraTag (メイン描画カメラとしてマーク)
        //m_registry->emplace<ActiveCameraTag>(cameraEntity);
    }

    void App::InitPlayerCamera()
    {
        entt::entity cameraEntity = m_registry->create();

        // 1. TransformComponent (初期位置・初期向き)
        TransformComponent camTransform;
        camTransform.position = DirectX::XMFLOAT3(0.0f, 5.0f, -15.0f);
        // 初期ピッチ角 0.3 rad のクォータニオン
        DirectX::XMVECTOR qRot = DirectX::XMQuaternionRotationRollPitchYaw(0.3f, 0.0f, 0.0f);
        DirectX::XMStoreFloat4(&camTransform.rotation, qRot);
        camTransform.scale = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
        m_registry->emplace<TransformComponent>(cameraEntity, std::move(camTransform));

        // 2. CameraComponent (投影パラメータ)
        CameraComponent camComponent;
        camComponent.fov = DirectX::XM_PIDIV4; // 45度
        camComponent.aspectRatio = 800.0f / 600.0f; // またはクライアント領域サイズ比
        camComponent.nearZ = 0.1f;
        camComponent.farZ = 1000.0f;
        camComponent.isProjectionDirty = true;
        m_registry->emplace<CameraComponent>(cameraEntity, std::move(camComponent));

        // 三人称視点用のカメラ
        ThirdPersonCameraComponent tpsComp;
        auto view = m_registry->view<PlayerTag>();
        if (view.each().begin() != view.each().end())
        {
            auto entity = view.front();
            tpsComp.targetEntity = entity;

        }
        m_registry->emplace<ThirdPersonCameraComponent>(cameraEntity, std::move(tpsComp));

        ThirdPersonCameraConfig tpsConfig;
        m_registry->emplace<ThirdPersonCameraConfig>(cameraEntity, std::move(tpsConfig));

        // 4. ActiveCameraTag (メイン描画カメラとしてマーク)
        m_registry->emplace<ActiveCameraTag>(cameraEntity);
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