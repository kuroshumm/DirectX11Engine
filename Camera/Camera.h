#pragma once
#include <DirectXMath.h>
#include "Transform.h"
#include "Frustum.h"

namespace Engine
{
    // カメラシステムをコンポーネントとして抽象化するクラス
    class Camera
    {
    public:
        Camera();
        ~Camera();

        // プロジェクション空間の定義インターフェース
        void SetProjectionValues(float fovRadians, float aspectRatio, float nearZ, float farZ);

        // トランスフォームの取得（カメラのワールド階層位置を表現）
        Transform& GetTransform() { return m_transform; }
        Frustum& GetFrustum() { return m_frustum; }

        // ビュー・プロジェクション行列の取得
        const DirectX::XMMATRIX& GetViewMatrix();
        const DirectX::XMMATRIX& GetProjectionMatrix() const;

    private:
        // カメラ（ビュー行列）の更新処理
        void UpdateCamera();

    private:
        Transform m_transform;
        Frustum m_frustum;

        DirectX::XMMATRIX m_viewMatrix;
        DirectX::XMMATRIX m_projectionMatrix;

        // 【機能拡張】キャッシング用データ
        DirectX::XMMATRIX m_cachedWorldMatrix;

        // プロジェクションパラメータ
        float m_fov;
        float m_aspect;
        float m_nearZ;
        float m_farZ;

    };
}
