#pragma once
#include <DirectXMath.h>

class Frustum
{
public:
    // 視錐台を構成する6つの平面 (Left, Right, Bottom, Top, Near, Far)
    DirectX::XMVECTOR m_Planes[6];

    // マトリックスから平面方程式を抽出し、正規化する関数
    void Update(const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projectionMatrix);

    // バウンディングスフィアとの交差判定を行う関数
    bool CheckSphere(const DirectX::XMFLOAT3& center, float radius) const;
};
