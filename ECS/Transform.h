#pragma once
#include <DirectXMath.h>
#include <vector>

namespace Engine
{
    // 3D空間におけるオブジェクトの姿勢（位置・回転・スケール）および
    // シーングラフにおける階層構造を統括するコンポーネントクラス
    class Transform
    {
    public:
        Transform();
        ~Transform();

        // --- パラメータの設定 ---
        void SetPosition(float x, float y, float z);
        void SetPosition(const DirectX::XMFLOAT3& pos);

        void SetRotation(float pitch, float yaw, float roll); // ラジアン指定 (X, Y, Z軸回転)
        void SetRotation(const DirectX::XMFLOAT3& rot);

        void SetScale(float x, float y, float z);
        void SetScale(const DirectX::XMFLOAT3& scale);

        // --- パラメータの取得 ---
        DirectX::XMFLOAT3 GetPosition() const { return m_position; }
        DirectX::XMFLOAT3 GetRotation() const { return m_rotation; }
        DirectX::XMFLOAT3 GetScale()    const { return m_scale; }

        // --- 行列の取得 (遅延評価のトリガー) ---
        DirectX::XMMATRIX GetLocalMatrix();
        DirectX::XMMATRIX GetWorldMatrix();

        bool IsDirty() const { return m_isDirty; }

        // --- 階層構造の管理 ---
        void AddChild(Transform* pChild);
        void SetParent(Transform* pParent);
        Transform* GetParent() const { return m_pParent; }

    private:
        // SRT要素
        DirectX::XMFLOAT3 m_position;
        DirectX::XMFLOAT3 m_rotation; // オイラー角 (Pitch, Yaw, Roll)
        DirectX::XMFLOAT3 m_scale;

        // キャッシュされた変換行列
        DirectX::XMMATRIX m_localMatrix;
        DirectX::XMMATRIX m_worldMatrix;

        // 階層ポインタ
        Transform* m_pParent;
        std::vector<Transform*> m_children;

        // 遅延評価用フラグ
        bool m_isDirty;

        // 内部メソッド
        void SetDirty();
        void UpdateMatrix();
    };
}
