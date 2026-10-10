#include <d3dcompiler.h>
#include <vector>
#include <algorithm>
#include <chrono>
#include <fstream>

#include "App.h"
#include "InputSystem.h"
#include "AnimationClip.h"
#include "AnimSequenceLoader.h"
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
        entt::entity groundEntity = PrimitiveFactory::CreatePrimitive(
            *m_registry,
            PrimitiveType::Plane,
            DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
        );

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
        m_animLoader = std::make_unique<AnimationLoader>();

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
        m_cameraSystem = std::make_unique<CameraSystem>();
        m_cullingSystem = std::make_unique<CullingSystem>();
        m_thirdPersonCameraSystem = std::make_unique<ThirdPersonCameraSystem>();
        m_playerInputSystem = std::make_unique<PlayerInputSystem>();
        m_playerMovementSystem = std::make_unique<PlayerMovementSystem>();
        m_combatSystem = std::make_unique<CombatSystem>();
        m_characterActionStateSystem = std::make_unique<CharacterActionStateSystem>();
        m_locomotionStateSystem = std::make_unique<LocomotionStateSystem>();
        m_attackingStateSystem = std::make_unique<AttackingStateSystem>();
        m_actionApplierSystem = std::make_unique<ActionApplierSystem>();

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

        m_pLambertianShader = ShaderLoader::LoadSkinned(
            m_pGraphics->GetDevice(),
            L"Shaders/PBRVertexShader.hlsl",
            L"Shaders/PBRPixelShader.hlsl"
        );
        if (!m_pLambertianShader)
        {
            return E_FAIL;
        }

        m_pStaticPBRSHader = ShaderLoader::LoadStatic(
            m_pGraphics->GetDevice(),
            L"Shaders/StaticPBRVertexShader.hlsl",
            L"Shaders/PBRPixelShader.hlsl"
        );
        if (!m_pStaticPBRSHader)
        {
            return E_FAIL;
        }

		hr = LoadTexture(L"Model/Paladin_diffuse.png");
        if (FAILED(hr))
        {
            return hr;
		}

        m_pAnimClipIdle      = AnimationLoader::Load("YBot/BouncingFight_Idle.fbx");
        m_pAnimClipWalk      = AnimationLoader::Load("YBot/Punching_Walking.fbx");
        m_pAnimClipWalkRight = AnimationLoader::Load("YBot/Punching_Walking_Right.fbx");
        m_pAnimClipWalkLeft  = AnimationLoader::Load("YBot/Punching_Walking_Left.fbx");
        m_pAnimClipWalkBack  = AnimationLoader::Load("YBot/Punching_Walking_Back.fbx");
        m_pAnimClipRun       = AnimationLoader::Load("YBot/Running.fbx");
        m_pAnimClipCombo1    = AnimationLoader::Load("YBot/Punching_Jab_left.fbx");
        m_pAnimClipCombo2    = AnimationLoader::Load("YBot/Punching_Jab_right.fbx");

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
        m_playerInputSystem->Update(*m_registry.get(), dt);

        // 2. カメラ更新 (Camera Layer)
        m_cameraControllerSystem->Update(*m_registry.get(), dt);
        m_thirdPersonCameraSystem->Update(*m_registry.get(), dt);
        m_cameraSystem->Update(*m_registry.get());

        // 3. 要求・提案層 (Proposal Layer)
        m_playerMovementSystem->Update(*m_registry.get(), dt);
        m_combatSystem->Update(*m_registry.get());

        // 4. 状態調停層 (Arbitration Layer)
        m_characterActionStateSystem->Update(*m_registry.get(), dt);

        // 5. 状態別実行層 (State Execution Layer)
        m_locomotionStateSystem->Update(*m_registry.get());
        m_attackingStateSystem->Update(*m_registry.get(), dt);

        // 6. 結果反映層 (Action Applier Layer)
        m_actionApplierSystem->Update(*m_registry.get(), *m_animatorSystem.get(), dt);

        // 7. 物理積分層 (Physics Layer)
        m_movementSystem->Update(*m_registry.get(), dt);

        // 8. カリング判定 (Culling)
        m_cullingSystem->Update(*m_registry.get());

        // 9. アニメーション・スケルトン・トランスフォームパイプライン
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
        inputSystem.PostUpdateKey();
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
        ModelLoader::LoadResult loadResult = m_modelLoader->Load("YBot/Y_Bot.fbx");
        if (!loadResult.isSuccess)
        {
            return E_FAIL;
        }

        entt::entity entity = m_registry->create();

        TransformComponent transformComp;
        transformComp.position = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        transformComp.rotation = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        transformComp.scale = DirectX::XMFLOAT3(0.01f, 0.01f, 0.01f);
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
        animatorComp.AddState(AnimStateType::IDLE,       AnimSequenceLoader::Load(AnimStateType::IDLE), true);
        animatorComp.AddState(AnimStateType::WALK,       AnimSequenceLoader::Load(AnimStateType::WALK), true);
        animatorComp.AddState(AnimStateType::WALK_RIGHT, AnimSequenceLoader::Load(AnimStateType::WALK_RIGHT), true);
        animatorComp.AddState(AnimStateType::WALK_LEFT,  AnimSequenceLoader::Load(AnimStateType::WALK_LEFT), true);
        animatorComp.AddState(AnimStateType::WALK_BACK,  AnimSequenceLoader::Load(AnimStateType::WALK_BACK), true);
        animatorComp.AddState(AnimStateType::RUN,        AnimSequenceLoader::Load(AnimStateType::RUN), true);
        animatorComp.AddState(AnimStateType::COMBO1,     AnimSequenceLoader::Load(AnimStateType::COMBO1), false);
        animatorComp.AddState(AnimStateType::COMBO2,     AnimSequenceLoader::Load(AnimStateType::COMBO2), false);
        m_registry->emplace<AnimatorComponent>(entity, std::move(animatorComp));

        m_animatorSystem->Play(*m_registry.get(), entity, AnimStateType::IDLE);

        auto findBone = [&](std::initializer_list<const char*> names) -> int {
            for (const char* name : names) {
                int idx = loadResult.skeltonAsset->GetBoneIndex(name);
                if (idx != -1) return idx;
            }
            return -1;
        };

        IKComponent ikComp;
        int upLegIndex = findBone({ "LeftUpLeg", "thigh_l", "mixamorig:LeftUpLeg" });
        int legIndex = findBone({ "LeftLeg", "calf_l", "mixamorig:LeftLeg" });
        int footIndex = findBone({ "LeftFoot", "foot_l", "mixamorig:LeftFoot" });
        if (upLegIndex != -1 && legIndex != -1 && footIndex != -1)
        {
            ikComp.effectorBoneIndex = footIndex;
            ikComp.chainIndices = { upLegIndex, legIndex, footIndex };
            // ヒザ関節の 1D 角度制約(X 軸周りに 0°〜120°)
            ikComp.SetConstraint(legIndex, DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), 0.0f, 120.0f);
            m_registry->emplace<IKComponent>(entity, std::move(ikComp));
        }

        MaterialComponent material;
        material.pShader = m_pLambertianShader;
        //material.pTexture = m_pTexture;
        m_registry->emplace<MaterialComponent>(entity, std::move(material));

        VelocityComponent velocity;
        m_registry->emplace<VelocityComponent>(entity, std::move(velocity));

        CharacterLocomotionComponent charaLocomotion;
        m_registry->emplace<CharacterLocomotionComponent>(entity, std::move(charaLocomotion));
        CharacterMovementConfigComponent charaMovementConfig;
        m_registry->emplace<CharacterMovementConfigComponent>(entity, std::move(charaMovementConfig));

        PlayerInputComponent playerInput;
        m_registry->emplace<PlayerInputComponent>(entity, std::move(playerInput));

        LocomotionProposalComponent locomotionProposal;
        m_registry->emplace<LocomotionProposalComponent>(entity, std::move(locomotionProposal));

        CombatRequestComponent combatReq;
        m_registry->emplace<CombatRequestComponent>(entity, std::move(combatReq));

        CharacterActionStateComponent charaActionState;
        m_registry->emplace<CharacterActionStateComponent>(entity, std::move(charaActionState));

        CharacterCombatStateComponent charaCombatState;
        m_registry->emplace<CharacterCombatStateComponent>(entity, std::move(charaCombatState));

        CharacterActionResultComponent charaActionResult;
        m_registry->emplace<CharacterActionResultComponent>(entity, std::move(charaActionResult));

        LocomotionAnimComponent locomotionAnim;
        m_registry->emplace<LocomotionAnimComponent>(entity, std::move(locomotionAnim));

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
}
