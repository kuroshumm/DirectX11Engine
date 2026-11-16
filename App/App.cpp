#include <algorithm>
#include <chrono>
#include <d3dcompiler.h>
#include <fstream>
#include <vector>

#include "AnimSequenceLoader.h"
#include "AnimationClip.h"
#include "App.h"
#include "Components.h"
#include "GameTime.h"
#include "InputSystem.h"
#include "PrimitiveFactory.h"

#pragma comment(lib, "d3dcompiler.lib")

namespace Engine
{
    App::App() : m_hWnd(nullptr)
    {
    }
    App::~App()
    {
    }

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
        if (FAILED(InitGeometry())) return E_FAIL;

        InitSceneCamera(width, height);

        PrimitiveFactory::Initialize(m_pGraphics.get(), m_pStaticPBRSHader);
        entt::entity groundEntity = PrimitiveFactory::CreatePrimitive(*m_registry, PrimitiveType::Plane);

        auto pGroundTexture = std::make_shared<Texture>();
        if (SUCCEEDED(pGroundTexture->LoadFromFile(m_pGraphics->GetDevice(), L"Model/Grid_1m.png")))
        {
            auto& mat = m_registry->get<MaterialComponent>(groundEntity);
            mat.pTexture = pGroundTexture;
            mat.roughness = 0.8f;
            mat.metallic = 0.0f;
        }

        m_pTextRenderer = std::make_unique<TextRenderer>();
        m_pTextRenderer->Initialize(m_pGraphics->GetSwapChain());

        m_modelLoader = std::make_unique<ModelLoader>(m_pGraphics.get());
        m_characterFactory = std::make_unique<CharacterFactory>(m_pGraphics->GetDevice(), *m_registry.get(), *m_modelLoader.get());
        m_animLoader = std::make_unique<AnimationLoader>();

        m_renderSystem.Initialize(m_pGraphics->GetDevice());

        InitPlayer();
        InitPlayerCamera(width, height);

        InitEnemy();

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
        m_hWnd = CreateWindowExW(0, CLASS_NAME, L"DirectX 11 Game Engine", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                 rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, this);

        return m_hWnd ? S_OK : E_FAIL;
    }

    HRESULT App::InitGeometry()
    {
        HRESULT hr = S_OK;

        m_pLambertianShader =
            ShaderLoader::LoadSkinned(m_pGraphics->GetDevice(), L"Shaders/PBRVertexShader.hlsl", L"Shaders/PBRPixelShader.hlsl");
        if (!m_pLambertianShader)
        {
            return E_FAIL;
        }

        m_pStaticPBRSHader =
            ShaderLoader::LoadStatic(m_pGraphics->GetDevice(), L"Shaders/StaticPBRVertexShader.hlsl", L"Shaders/PBRPixelShader.hlsl");
        if (!m_pStaticPBRSHader)
        {
            return E_FAIL;
        }

        hr = LoadTexture(L"Model/Paladin_diffuse.png");
        if (FAILED(hr))
        {
            return hr;
        }

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
        // 1. 入力層 (Input Layer)
        m_playerInputSystem.Update(*m_registry.get(), dt);

        // 2. カメラ更新 (Camera Layer)
        m_cameraControllerSystem.Update(*m_registry.get(), dt);
        m_thirdPersonCameraSystem.Update(*m_registry.get(), dt);
        m_cameraSystem.Update(*m_registry.get());

        // 3. 要求・提案層 (Proposal Layer)
        m_playerMovementSystem.Update(*m_registry.get(), dt);
        m_combatSystem.Update(*m_registry.get());

        // 3.5. ダメージ解決層 (Damage Resolution Layer)
        m_damageResolutionSystem.Update(*m_registry.get());

        // 4. 状態調停層 (Arbitration Layer)
        m_characterActionStateSystem.Update(*m_registry.get(), dt);

        // 5. 状態別実行層 (State Execution Layer)
        m_locomotionStateSystem.Update(*m_registry.get());
        m_damageStateSystem.Update(*m_registry.get(), dt);
        m_deadStateSystem.Update(*m_registry.get(), dt);
        m_attackingStateSystem.Update(*m_registry.get(), dt);

        // 6. 結果反映層 (Action Applier Layer)
        m_actionApplierSystem.Update(*m_registry.get(), m_animatorSystem, dt);

        // 7. 物理積分層 (Physics Layer)
        m_movementSystem.Update(*m_registry.get(), dt);

        // 8. アニメーション時間・イベント評価・コリジョン生成・可視化パイプライン
        m_animatorSystem.Update(*m_registry.get(), dt);
        m_animEventSystem.Update(*m_registry.get());
        m_collisionSpawnSystem.Update(*m_registry.get());
        m_collisionVisualizerSystem.Update(*m_registry.get());
        m_collisionDetectionSystem.Update(*m_registry.get());

        // 9. カリング・スケルトン姿勢・ワールド行列・GPUパレット転送パイプライン
        m_cullingSystem.Update(*m_registry.get());
        m_animationSystem.Update(*m_registry.get(), dt);

        m_boneTransformSystem.Update(*m_registry.get());

        m_ikSystem.Update(*m_registry.get(), dt);
        m_hierarchyTransformSystem.Update(*m_registry.get()); // 階層トランスフォーム更新
        m_transformSystem.Update(*m_registry.get());

        m_bonePaletteUploadSystem.Update(*m_registry.get(), *m_pGraphics->GetContext());

        // 更新の一番最後にマウスの変化量をリセットしておく
        InputSystem& inputSystem = InputSystem::GetInstance();
        inputSystem.PostUpdateKey();
    }

    void App::Render()
    {
        int drawnObjectsCount = 0; // 描画された数を数える変数

        m_pGraphics->BeginScene(0.45f, 0.65f, 0.85f, 1.0f);

        m_pGraphics->SetRender();
        m_pGraphics->SetTextureAndSampler();

        m_renderSystem.Render(*m_registry.get(), *m_pGraphics);

        if (m_pTextRenderer)
        {
            m_pTextRenderer->RenderTextRightTop(m_fpsCounter.GetFPSString(1000));
        }

        m_pGraphics->EndScene();
    }

    void App::InitPlayer()
    {
        HumanoidSpawnDesc desc;
        desc.pShader = m_pLambertianShader;
        m_characterFactory->LoadModelAsset();
        entt::entity player = m_characterFactory->SpawnHumanoid(desc);

        AnimatorComponent animatorComp;
        animatorComp.AddState(AnimStateType::IDLE, AnimSequenceLoader::Load(AnimStateType::IDLE), true);
        animatorComp.AddState(AnimStateType::WALK, AnimSequenceLoader::Load(AnimStateType::WALK), true);
        animatorComp.AddState(AnimStateType::WALK_RIGHT, AnimSequenceLoader::Load(AnimStateType::WALK_RIGHT), true);
        animatorComp.AddState(AnimStateType::WALK_LEFT, AnimSequenceLoader::Load(AnimStateType::WALK_LEFT), true);
        animatorComp.AddState(AnimStateType::WALK_BACK, AnimSequenceLoader::Load(AnimStateType::WALK_BACK), true);
        animatorComp.AddState(AnimStateType::RUN, AnimSequenceLoader::Load(AnimStateType::RUN), true);
        animatorComp.AddState(AnimStateType::COMBO1, AnimSequenceLoader::Load(AnimStateType::COMBO1), false);
        animatorComp.AddState(AnimStateType::COMBO2, AnimSequenceLoader::Load(AnimStateType::COMBO2), false);
        animatorComp.AddState(AnimStateType::DAMAGE, AnimSequenceLoader::Load(AnimStateType::DAMAGE), false);
        animatorComp.AddState(AnimStateType::DEATH, AnimSequenceLoader::Load(AnimStateType::DEATH), false);
        m_registry->emplace<AnimatorComponent>(player, std::move(animatorComp));

        m_animatorSystem.Play(*m_registry.get(), player, AnimStateType::IDLE);

        m_registry->emplace<PlayerInputComponent>(player);
        m_registry->emplace<PlayerTag>(player);
    }

    void App::InitSceneCamera(int width, int height)
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
        camComponent.aspectRatio = static_cast<float>(width) / static_cast<float>(height);
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
        // m_registry->emplace<ActiveCameraTag>(cameraEntity);
    }

    void App::InitPlayerCamera(int width, int height)
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
        camComponent.aspectRatio = static_cast<float>(width) / static_cast<float>(height);
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

    void App::InitEnemy()
    {
        float radius = 5.0f;
        float angleStep = DirectX::XM_2PI / 5.0f;

        auto playerView = m_registry->view<PlayerTag>();
        if (playerView.empty()) return; // プレイヤーが存在しない場合の安全ガード
        entt::entity playerEntity = playerView.front();
        const auto& playerTransform = m_registry->get<TransformComponent>(playerEntity);

        for (int index = 0, max = 5; index < max; ++index)
        {
            float angle = angleStep * index;

            HumanoidSpawnDesc desc;
            desc.position.x = playerTransform.position.x + radius * std::sin(angle);
            desc.position.z = playerTransform.position.z + radius * std::cos(angle);

            float diffX = playerTransform.position.x - desc.position.x;
            float diffZ = playerTransform.position.z - desc.position.z;
            float yaw = std::atan2(-diffX, -diffZ);
            DirectX::XMVECTOR qRot = DirectX::XMQuaternionRotationRollPitchYaw(0.0f, yaw, 0.0f);

            DirectX::XMStoreFloat4(&desc.rotation, qRot);
            desc.pShader = m_pLambertianShader;
            entt::entity enemy = m_characterFactory->SpawnHumanoid(desc);

            AnimatorComponent animatorComp;
            animatorComp.AddState(AnimStateType::IDLE, AnimSequenceLoader::Load(AnimStateType::IDLE), true);
            animatorComp.AddState(AnimStateType::DAMAGE, AnimSequenceLoader::Load(AnimStateType::DAMAGE), false);
            animatorComp.AddState(AnimStateType::DEATH, AnimSequenceLoader::Load(AnimStateType::DEATH), false);
            m_registry->emplace<AnimatorComponent>(enemy, std::move(animatorComp));

            m_animatorSystem.Play(*m_registry.get(), enemy, AnimStateType::IDLE);

            m_registry->emplace<EnemyTag>(enemy);
        }
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

        // 左クリック
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            inputSystem.UpdateKey(VK_LBUTTON, message == WM_LBUTTONDOWN);
            break;

        // 右クリック
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            inputSystem.UpdateKey(VK_RBUTTON, message == WM_RBUTTONDOWN);
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
} // namespace Engine
