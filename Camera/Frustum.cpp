#include "Frustum.h"

using namespace DirectX;

void Frustum::Update(const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix)
{
    // 1. View行列とProjection行列を乗算し、VP行列を作成する
    XMMATRIX vpMatrix = XMMatrixMultiply(viewMatrix, projectionMatrix);

    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, vpMatrix);

    // Left Plane (x_c >= -w_c つまり x_c + w_c >= 0)
    m_Planes[0] = XMVectorSet(m._14 + m._11, m._24 + m._21, m._34 + m._31, m._44 + m._41);

    // Right Plane (x_c <= w_c つまり -x_c + w_c >= 0)
    m_Planes[1] = XMVectorSet(m._14 - m._11, m._24 - m._21, m._34 - m._31, m._44 - m._41);

    // Bottom Plane (y_c >= -w_c つまり y_c + w_c >= 0)
    m_Planes[2] = XMVectorSet(m._14 + m._12, m._24 + m._22, m._34 + m._32, m._44 + m._42);

    // Top Plane (y_c <= w_c つまり -y_c + w_c >= 0)
    m_Planes[3] = XMVectorSet(m._14 - m._12, m._24 - m._22, m._34 - m._32, m._44 - m._42);

    // Near Plane (DirectXでは z_c >= 0)
    m_Planes[4] = XMVectorSet(m._13, m._23, m._33, m._43);

    // Far Plane (DirectXでは z_c <= w_c つまり -z_c + w_c >= 0)
    m_Planes[5] = XMVectorSet(m._14 - m._13, m._24 - m._23, m._34 - m._33, m._44 - m._43);

    // 2. すべての平面の法線を正規化する
    for (int i = 0; i < 6; ++i)
    {
        m_Planes[i] = XMPlaneNormalize(m_Planes[i]);
    }
}

bool Frustum::CheckSphere(const XMFLOAT3& center, float radius) const
{
    XMVECTOR vCenter = XMLoadFloat3(&center);
    for (int i = 0; i < 6; ++i)
    {
        // TODO [課題2]: 平面と球の中心座標との距離(Dot Product / 内積)を計算する。
        // DirectXMathでは XMPlaneDotCoord 関数を用いると、簡単に中心座標と平面の符号付き距離を算出できます。
        /* TODO: XMPlaneDotCoord で m_Planes[i] と vCenter を評価 */
        XMVECTOR vDistance = XMPlaneDotCoord(m_Planes[i], vCenter);

        // ベクトルからfloat値を取り出す
        float distance = XMVectorGetX(vDistance);
        // TODO [課題2]: 球が完全に平面の裏側（外部）にあるか判定する。
        // ヒント: d < -R の場合は、視錐台の外なのでカメラに映らない。即座に false を返す。
        if (distance < -radius)
        {
            return false; // 視錐台の外部（カリング対象）
        }
    }

    // どの平面の裏側にも完全には追いやられていない場合、カメラに映る（または境界を跨いでいる）
    return true;
}