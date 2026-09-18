#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <cassert>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <entt/entt.hpp>

#include "IModel.h"
#include "AnimationClip.h"
#include "Frustum.h"
#include "Texture.h"
#include "Shader.h"
#include "ModelAsset.h"

namespace Engine
{
	struct TransformComponent
	{
		DirectX::XMFLOAT3 position{ 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT4 rotation{ 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT3 scale{ 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT4X4 worldMatrix;

		bool isDirty = true;
	};

	// 共有スケルトンアセット（ローダーが保持、Entity生成時に参照）
	struct SkeltonAsset
	{
		std::vector<Bone> initialBones;
		int rootBoneIndex;
		std::unordered_map<std::string, int> boneMapping;

		// 名前からインデックスを取得する関数を追加
		int GetBoneIndex(const std::string& name) const
		{
			auto it = boneMapping.find(name);
			if (it != boneMapping.end())
			{
				return it->second;
			}
			return -1;
		}
	};

	struct ModelComponent
	{
		std::shared_ptr<const ModelAsset> pModelAsset = nullptr;
	};

	struct MaterialComponent
	{
		std::shared_ptr<Texture> pTexture;
		std::shared_ptr<Shader>  pShader;
		DirectX::XMFLOAT4 baseColor{ 1.0f, 1.0f, 1.0f, 1.0f };
		float metallic = 0.0f;
		float roughness = 0.5f;
	};
	
	struct SkeletonComponent
	{
		std::vector<Bone> bones;       // initialBones からコピーされた可変データ
		int rootBoneIndex = -1;
	};
	
	struct SkinningPaletteComponent
	{
		Microsoft::WRL::ComPtr<ID3D11Buffer> pPaletteBuffer;
	};

	struct VelocityComponent
	{
		DirectX::XMFLOAT3 velocity{ 0.0f, 0.0f, 0.0f };
	};

	struct AnimationComponent
	{
		std::shared_ptr<AnimationClip> pCurrentClip = nullptr;
		float currentTime   = 0.0f;
		float playbackSpeed = 1.0f;
		bool isLoop			= false;
		bool isPlaying		= false;
	};

	struct AnimationState2
	{
		std::string Name;
		std::shared_ptr<AnimationClip> Clip = nullptr;
		bool isLoop = false;
	};

	struct AnimatorComponent
	{
		std::unordered_map < std::string, AnimationState2 > stateMap;
		std::string currentStateName = "";
		std::string prevStateName = "";

		std::shared_ptr<AnimationClip> pPrevClip = nullptr;
		float prevTime = 0.0f;
		float transitionDuration = 0.0f;
		float transitionTimer = 0.0f;
		float transitionWeight = 0.0f;
		bool isTransitioning = false;

		void AddState(const std::string& stateName, const std::shared_ptr<AnimationClip>& pClip, bool isLoop)
		{
			AnimationState2 animState;
			animState.Name = stateName;
			animState.Clip = pClip;
			animState.isLoop = isLoop;

			auto [it, inserted] = stateMap.try_emplace(stateName, std::move(animState));
			assert(inserted);
		}
	};

	struct JointConstraint
	{
		bool isLimited;
		DirectX::XMVECTOR localAxis;
		float minAngleRad;
		float maxAngleRad;
	};

	enum class IKSolverType
	{
		FABRIK,
		CCD
	};

	struct IKComponent
	{
		IKSolverType							 solverType = IKSolverType::FABRIK;
		int										 effectorBoneIndex = -1;    // エフェクタ (例: 足先)
		std::vector<int>						 chainIndices;              // 関節チェーン (太もも ➔ すね ➔ 足先)
		std::unordered_map<int, JointConstraint> constraints;			    // ★ 関節制約データを保持！
		DirectX::XMFLOAT3						 targetPosition{ 0, 0, 0 }; // 目標到達座標
		int										 maxIterations = 10;        // 最大反復回数
		float									 tolerance = 0.001f;        // 許容誤差
		float									 testTimer = 0.0f;          // デモ用タイマー
		bool									 isEnabled = false;          // 有効フラグ

		void SetConstraint(int boneIndex, DirectX::XMVECTOR localAxis, float minAngleDeg, float maxAngleDeg)
		{
			JointConstraint constraint;
			constraint.isLimited = true;
			constraint.localAxis = DirectX::XMVector3Normalize(localAxis); // 軸を確実に正規化
			constraint.minAngleRad = DirectX::XMConvertToRadians(minAngleDeg);
			constraint.maxAngleRad = DirectX::XMConvertToRadians(maxAngleDeg);
			constraints[boneIndex] = constraint;
		}
	};

	struct HierarchyComponent
	{
		entt::entity parent = entt::null;
		entt::entity firstChild = entt::null;
		entt::entity nextSibling = entt::null;
		entt::entity prevSibling = entt::null;

		uint32_t depth = 0; // ルートからの階層深度
	};

	// カメラの投影・ビュー情報および Frustum を保持するデータ構造
	struct CameraComponent
	{
		// 投影パラメータ
		float fov = DirectX::XM_PIDIV4; // 45度
		float aspectRatio = 16.0f / 9.0f;
		float nearZ = 0.1f;
		float farZ = 1000.0f;
		// 計算結果キャッシュ
		DirectX::XMFLOAT4X4 viewMatrix;
		DirectX::XMFLOAT4X4 projectionMatrix;
		Frustum frustum;
		// プロジェクションパラメータ変更フラグ
		bool isProjectionDirty = true;
	};
	// 自由視点操作を行うためのコントローラコンポーネント
	struct FreeLookCameraControllerComponent
	{
		float moveSpeed = 50.0f;
		float sensitivity = 0.3f;
		// ジンバルロック回避およびクランプ用のオイラー角蓄積値 (ラジアン)
		float pitch = 0.0f;
		float yaw = 0.0f;
		float minPitch = -DirectX::XM_PIDIV2 + 0.01f; // 約 -89度
		float maxPitch = DirectX::XM_PIDIV2 - 0.01f; // 約 +89度
	};
	struct ThirdPersonCameraConfig
	{
		float distance = 14.0f;		// カメラと注視点の距離（キャラ高さ約9uの約1.5倍）
		float heightOffset = 6.0f;	// 注視点の高さ（キャラの肩～首付近）
		
		float pitch = 0.18f;		// デフォルト見下ろし角（約10度）
		float minPitch = -0.5f;		// 見上げ限界（約-28度）
		float maxPitch = 1.3f;		// 見下ろし限界（約+75度）

		float rotateSensitivity = 0.005f;	// カメラ回転感度
		int t = 0;
	};
	// 三人称視点用のカメラ
	struct ThirdPersonCameraComponent
	{
		entt::entity targetEntity = entt::null; // 追従対象

		float distance = 14.0f;		// 現在のカメラ距離
		float heightOffset = 6.0f;	// 現在の注視点高さ
		float pitch = 0.18f;		// 現在の垂直仰角
		float yaw = 0.0f;			// 現在の水平方位角
	};

	struct LODComponent
	{
		int	  level = 0;				// 0=高品質、1=中、2=低
		int	  updateInterval = 0;		// 何フレームに１回更新するか（levelから算出）
		int	  frameCounter = 0;			// 現在のカウンタ
		float distanceToCamera = 0.0f;	// カメラからの距離（CullingSystemが毎フレーム計算）
	};




	// 現在のメインビューポートとして描画に使用するカメラを示す識別タグ
	struct ActiveCameraTag {};
	// 画面内に存在するエンティティに付与するマーカー
	struct VisibleTag {};
	// このフレームでアニメーション更新を行うエンティティに付与されるマーカー
	struct AnimationUpdateTag {};
	// プレイヤー識別マーカー
	struct PlayerTag {};
}