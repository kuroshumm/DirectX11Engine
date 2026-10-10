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
#include "AnimationLoader.h"
#include "ShaderLoader.h"

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
        HRESULT InitPlayer();
        void InitSceneCamera();
        void InitPlayerCamera();

        HWND m_hWnd;
        std::unique_ptr<Graphics>     m_pGraphics;
		std::shared_ptr<Texture>      m_pTexture;
        std::shared_ptr<Shader>       m_pLambertianShader;
        std::shared_ptr<Shader>       m_pStaticPBRSHader;

        std::unique_ptr<ModelLoader> m_modelLoader;
        std::unique_ptr<AnimationLoader> m_animLoader;

        std::shared_ptr<AnimationClip> m_pAnimClipIdle;
        std::shared_ptr<AnimationClip> m_pAnimClipWalk;
        std::shared_ptr<AnimationClip> m_pAnimClipWalkRight;
        std::shared_ptr<AnimationClip> m_pAnimClipWalkLeft;
        std::shared_ptr<AnimationClip> m_pAnimClipWalkBack;
        std::shared_ptr<AnimationClip> m_pAnimClipRun;
        std::shared_ptr<AnimationClip> m_pAnimClipCombo1;
        std::shared_ptr<AnimationClip> m_pAnimClipCombo2;

        std::unique_ptr<entt::registry> m_registry;
        std::unique_ptr<RenderSystem> m_renderSystem;
        std::unique_ptr<TransformSystem> m_transformSystem;
        std::unique_ptr<MovementSystem> m_movementSystem;
        std::unique_ptr<AnimationSystem> m_animationSystem;
        std::unique_ptr<AnimatorSystem> m_animatorSystem;
        std::unique_ptr<BoneTransformSystem> m_boneTransformSystem;
        std::unique_ptr<BonePaletteUploadSystem> m_bonePaletteUploadSystem;
        std::unique_ptr<IKSystem> m_ikSystem;
        std::unique_ptr<HierarchyTransformSystem> m_hierarchyTransformSystem;
        std::unique_ptr<CameraControllerSystem> m_cameraControllerSystem;
        std::unique_ptr<CameraSystem> m_cameraSystem;
        std::unique_ptr<CullingSystem> m_cullingSystem;
        std::unique_ptr<ThirdPersonCameraSystem> m_thirdPersonCameraSystem;
        std::unique_ptr<PlayerInputSystem> m_playerInputSystem;
        std::unique_ptr<PlayerMovementSystem> m_playerMovementSystem;
        std::unique_ptr<CombatSystem> m_combatSystem;
        std::unique_ptr<CharacterActionStateSystem> m_characterActionStateSystem;
        std::unique_ptr<LocomotionStateSystem> m_locomotionStateSystem;
        std::unique_ptr<AttackingStateSystem> m_attackingStateSystem;
        std::unique_ptr<ActionApplierSystem> m_actionApplierSystem;

        std::unique_ptr<TextRenderer> m_pTextRenderer;
        FPSCounter m_fpsCounter;
    };
}