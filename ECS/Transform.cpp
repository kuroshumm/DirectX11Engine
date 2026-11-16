#include "Transform.h"

namespace Engine
{
    using namespace DirectX;

    Transform::Transform()
        : m_position(0.0f, 0.0f, 0.0f)
        , m_rotation(0.0f, 0.0f, 0.0f)
        , m_scale(1.0f, 1.0f, 1.0f)
        , m_localMatrix(XMMatrixIdentity())
        , m_worldMatrix(XMMatrixIdentity())
        , m_pParent(nullptr)
        , m_isDirty(true)
    {
    }

    Transform::~Transform()
    {
    }

    void Transform::SetPosition(float x, float y, float z)
    {
        m_position = XMFLOAT3(x, y, z);
        SetDirty();
    }

    void Transform::SetPosition(const XMFLOAT3& pos)
    {
        m_position = pos;
        SetDirty();
    }

    void Transform::SetRotation(float pitch, float yaw, float roll)
    {
        m_rotation = XMFLOAT3(pitch, yaw, roll);
        SetDirty();
    }

    void Transform::SetRotation(const XMFLOAT3& rot)
    {
        m_rotation = rot;
        SetDirty();
    }

    void Transform::SetScale(float x, float y, float z)
    {
        m_scale = XMFLOAT3(x, y, z);
        SetDirty();
    }

    void Transform::SetScale(const XMFLOAT3& scale)
    {
        m_scale = scale;
        SetDirty();
    }

    void Transform::AddChild(Transform* pChild)
    {
        if (!pChild) return;

        // --- TODO 1: 親子関係の構築 ---
        /* 引数として渡された pChild を自身 (this) の子として登録せよ。
           1. m_children ベクタに対し pChild を push_back する。
           2. pChild の親ポインタ (m_pParent) を this に設定する。
           3. pChild の SetDirty() メソッドを呼び出し、親の変更に伴う再計算を予約する。
        */
        m_children.push_back(pChild);
        pChild->m_pParent = this;
        pChild->SetDirty();
    }

    void Transform::SetParent(Transform* pParent)
    {
        if (m_pParent == pParent)
        {
            return;
        }

        // 既に親が存在する場合は、現在の親の子リストから自身を削除する
        if (m_pParent)
        {
            auto& siblings = m_pParent->m_children;
            siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
        }
        m_pParent = pParent;
        // 新しい親が存在する場合は、その子リストに自身を追加する
        if (m_pParent)
        {
            m_pParent->m_children.push_back(this);
        }
        SetDirty();
    }

    void Transform::SetDirty()
    {
        // 既にDirty状態であれば、下位ツリーも伝播済みであるため早期リターン
        if (m_isDirty) return;

        m_isDirty = true;

        // --- TODO 2: Dirtyフラグの再帰的伝播 ---
        /* m_children ベクタに格納されている全ての子 Transform ポインタを走査し、
           それぞれの子に対して SetDirty() を呼び出せ。
        */
        for (Transform* child : m_children)
        {
            if (!child)
            {
                continue;
            }
            child->SetDirty();
        }
    }

    XMMATRIX Transform::GetLocalMatrix()
    {
        if (m_isDirty)
        {
            UpdateMatrix();
        }
        return m_localMatrix;
    }

    XMMATRIX Transform::GetWorldMatrix()
    {
        if (m_isDirty)
        {
            UpdateMatrix();
        }
        return m_worldMatrix;
    }

    void Transform::UpdateMatrix()
    {
        // --- TODO 3: ローカル行列の計算 ---
        /* DirectXMathの関数を用いて S, R, T 各行列を生成し、ローカル行列を構築せよ。
           1. XMMatrixScaling 関数に m_scale の成分を渡し、スケーリング行列 (S) を作成する。
           2. XMMatrixRotationRollPitchYaw 関数に m_rotation の成分 (Pitch(x), Yaw(y), Roll(z)) を渡し、回転行列 (R) を作成する。
           3. XMMatrixTranslation 関数に m_position の成分を渡し、平行移動行列 (T) を作成する。
           4. DirectXMath は行優先 (Row-Major) であるため、S * R * T の順序で乗算を行い m_localMatrix へ代入せよ。
        */
        XMMATRIX scaleMatrix = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
        XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
        XMMATRIX translationMatrix = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
        m_localMatrix = scaleMatrix * rotationMatrix * translationMatrix;

        // --- TODO 4: ワールド行列の導出 ---
        /* 自身の m_localMatrix と、親のワールド行列を合成して m_worldMatrix を構築せよ。
           1. m_pParent が nullptr でない場合:
              親の GetWorldMatrix() を呼び出し、それに自身の m_localMatrix を掛け合わせて m_worldMatrix へ代入する。
              (乗算順序: m_localMatrix * 親のワールド行列)
           2. m_pParent が nullptr の場合 (ルートノード):
              m_localMatrix をそのまま m_worldMatrix へ代入する。
        */

        m_worldMatrix = m_pParent == nullptr
            ? m_localMatrix 
            : m_localMatrix * m_pParent->GetWorldMatrix();

        // 再計算が完了したためフラグをリセット
        m_isDirty = false;
    }
}
