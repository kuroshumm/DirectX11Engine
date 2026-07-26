#include "Camera.h"

namespace Engine
{
    Camera::Camera()
    {
        /*DirectX::XMVECTOR Eye = DirectX::XMVectorSet(0.0f, 5.0f, -15.0f, 0.0f);
        DirectX::XMVECTOR At = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
        DirectX::XMVECTOR Up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        m_viewMatrix = DirectX::XMMatrixLookAtLH(Eye, At, Up);
        m_projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, 800.0f / 600.0f, 0.01f, 100.0f);*/
        m_viewMatrix = DirectX::XMMatrixIdentity();
        m_projectionMatrix = DirectX::XMMatrixIdentity();
        m_cachedWorldMatrix = DirectX::XMMatrixIdentity(); // 初期化

        m_fov = DirectX::XM_PIDIV4; // 45度
        m_aspect = 800.0f / 600.0f;
        m_nearZ = 0.1f;
        m_farZ = 1000.0f;

        SetProjectionValues(m_fov, m_aspect, m_nearZ, m_farZ);
    }

    Camera::~Camera() {}

    void Camera::SetProjectionValues(float fovRadians, float aspectRatio, float nearZ, float farZ)
    {
        m_fov = fovRadians;
        m_aspect = aspectRatio;
		m_nearZ = nearZ <= 0.0f ? 0.01f : nearZ; // 安全性の担保: Near Clip は正の値でなければならな
		m_farZ = farZ > 10000.0f ? 10000.0f : farZ; // 安全性の担保: Far Clip は適切な上限を設ける

        // 【課題1(中級)】 プロジェクション行列の構築と安全性の担保
        // TODO: 透視投影行列(左手座標系)を構築して m_projectionMatrix に代入せよ。
        // ※ヒント: Near Clip と Far Clip の比率が極端な場合におけるZ-fighting(深度精度の枯渇)を防ぐため、
        // 必要と判断した場合は変数の検証や適切な関数の使用を考慮すること。
        m_projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(m_fov, aspectRatio, m_nearZ, m_farZ);

    }

    const DirectX::XMMATRIX& Camera::GetViewMatrix()
    {
        // 【課題2(実践)】 キャッシング状態評価機構の実装
        // TODO: 毎フレーム無条件にビュー行列を再計算するのではなく、
        // m_transform.GetWorldMatrix() と m_cachedWorldMatrix を比較検証し、
        // 「カメラの姿勢（配置や回転）に変化があった場合のみ」 UpdateCamera() を呼び出すように制御せよ。
        // ※ヒント: 行列の比較にはメモリ単位での同一性チェックや固有関数の利用を検討すること。
        
		DirectX::XMMATRIX currentWorldMatrix = m_transform.GetWorldMatrix();

        if(std::memcmp(&m_cachedWorldMatrix, &currentWorldMatrix, sizeof(DirectX::XMMATRIX)) != 0)
        {
            UpdateCamera(); // 現状は制御がないため毎フレーム走ってしまう。適宜修正せよ。
        }

        return m_viewMatrix;
    }

    const DirectX::XMMATRIX& Camera::GetProjectionMatrix() const
    {
        return m_projectionMatrix;
    }

    void Camera::UpdateCamera()
    {
        DirectX::XMMATRIX camWorldMatrix = m_transform.GetWorldMatrix();
        
        // キャッシュの更新
        m_cachedWorldMatrix = camWorldMatrix;

        // --- 以下の課題3 と 課題4 はどちらか一方のアプローチを選択し実装すること ---

        // 【課題3(上級)】 メモリアクセスによる基底ベクトルの直接抽出と構築 (Extract & LookTo方式)
        // TODO: 行列 camWorldMatrix から、メモリ配置（例: XMFLOAT4X4）を介して要素行にアクセスし、
        // 1. Z軸方向（Forward）
        // 2. Y軸方向（Up）
        // 3. 絶対位置（Eye）
        // の3つのベクトルを抽出し、それらを元に DirectX::XMMATRIX のビュー行列関数を用いて m_viewMatrix を算出せよ。
        // ※ベクトル抽出関数のテンプレは削除されたので、型の定義から抽出まで完全に自身で記述すること。
        //DirectX::XMFLOAT4X4 camWorldFloat4x4;
		//DirectX::XMStoreFloat4x4(&camWorldFloat4x4, camWorldMatrix);
        //m_viewMatrix = DirectX::XMMatrixLookToLH(
        //DirectX::XMVectorSet(camWorldFloat4x4._41, camWorldFloat4x4._42, camWorldFloat4x4._43, 1.0f), // Eye
        //DirectX::XMVectorSet(camWorldFloat4x4._31, camWorldFloat4x4._32, camWorldFloat4x4._33, 0.0f), // At (Forward)
        //DirectX::XMVectorSet(camWorldFloat4x4._21, camWorldFloat4x4._22, camWorldFloat4x4._23, 0.0f)  // Up
		//);

        // 【課題4(上級)】 逆写像によるアプローチ (Inverse Matrix方式)
        // TODO: 課題3とは異なり、座学で触れた数学的定理「M_View = M_CameraWorld^{-1}」を
        // DirectXMathの逆行列算出関数のみを用いて１行（または最小限の処理）でプログラム上に表現せよ。
        // ※ヒント: 逆行列を求める計算では、必ず「行列式（Determinant）」の算出結果ベクトルが必要となる。
		m_viewMatrix = DirectX::XMMatrixInverse(nullptr, camWorldMatrix);
    }
}
