#pragma once
#include <windows.h>
#include <memory>
#include <wrl/client.h>
#include <entt/entt.hpp>

#include "Graphics.h"
#include "Mesh.h"
#include "Common.h"
#include "Model.h"
#include "Camera.h"
#include "SceneCameraOperator.h"
#include "Registry.h"
#include "RenderSystem.h"
#include "MovementSystem.h"
#include "TransformSystem.h"
#include "AnimationSystem.h"
#include "AnimatorSystem.h"
#include "FPSCounter.h"
#include "TextRenderer.h"
#include "BoneTransformSystem.h"
#include "BonePaletteUploadSystem.h"
#include "IKSystem.h"
#include "HierarchyTransformSystem.h"
#include "CameraControllerSystem.h"
#include "CameraSystem.h"
#include "ModelLoader.h"
#include "CullingSystem.h"
#include "ThirdPersonCameraSystem.h"
#include "PlayerInputSystem.h"
#include "PlayerMovementSystem.h"
#include "CombatSystem.h"
#include "CharacterActionStateSystem.h"
#include "LocomotionStateSystem.h"
#include "AttackingStateSystem.h"
#include "ActionApplierSystem.h"
#include "AnimEventSystem.h"
#include "CollisionSpawnSystem.h"
#include "CollisionVisualizerSystem.h"
#include "AnimationLoader.h"
#include "ShaderLoader.h"
#include "CharacterFactory.h"
#include "CollisionDetectionSystem.h"
#include "DamageResolutionSystem.h"
#include "DamageStateSystem.h"
#include "DeadStateSystem.h"

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
        HRESULT LoadTexture(const wchar_t* filePath);

        static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
        void Update(float dt);
        void Render();

        void InitPlayer();
        void InitSceneCamera(int width, int height);
        void InitPlayerCamera(int width, int height);

        void InitEnemy();

        HWND m_hWnd;
        std::unique_ptr<entt::registry> m_registry;
        std::unique_ptr<Graphics>       m_pGraphics;
        std::shared_ptr<Texture>        m_pTexture;
        std::shared_ptr<Shader>         m_pLambertianShader;
        std::shared_ptr<Shader>         m_pStaticPBRSHader;

        std::unique_ptr<CharacterFactory> m_characterFactory;
        std::unique_ptr<ModelLoader> m_modelLoader;
        std::unique_ptr<AnimationLoader> m_animLoader;

        RenderSystem m_renderSystem;
        TransformSystem m_transformSystem;
        MovementSystem m_movementSystem;
        AnimationSystem m_animationSystem;
        AnimatorSystem m_animatorSystem;
        BoneTransformSystem m_boneTransformSystem;
        BonePaletteUploadSystem m_bonePaletteUploadSystem;
        IKSystem m_ikSystem;
        HierarchyTransformSystem m_hierarchyTransformSystem;
        CameraControllerSystem m_cameraControllerSystem;
        CameraSystem m_cameraSystem;
        CullingSystem m_cullingSystem;
        ThirdPersonCameraSystem m_thirdPersonCameraSystem;
        PlayerInputSystem m_playerInputSystem;
        PlayerMovementSystem m_playerMovementSystem;
        CombatSystem m_combatSystem;
        CharacterActionStateSystem m_characterActionStateSystem;
        LocomotionStateSystem m_locomotionStateSystem;
        AttackingStateSystem m_attackingStateSystem;
        ActionApplierSystem m_actionApplierSystem;
        AnimEventSystem m_animEventSystem;
        CollisionSpawnSystem m_collisionSpawnSystem;
        CollisionDetectionSystem m_collisionDetectionSystem;
        CollisionVisualizerSystem m_collisionVisualizerSystem;
        DamageResolutionSystem m_damageResolutionSystem;
        DamageStateSystem m_damageStateSystem;
        DeadStateSystem m_deadStateSystem;

        std::unique_ptr<TextRenderer> m_pTextRenderer;
        FPSCounter m_fpsCounter;
    };
}
