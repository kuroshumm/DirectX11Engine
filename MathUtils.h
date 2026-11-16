#pragma once
#include <DirectXMath.h>
#include <cmath>

namespace Engine::Math
{
    /// <summary>
    /// 方向ベクトル（XZ平面）から目標Yaw角を計算
    /// </summary>
    /// <param name="direction"></param>
    /// <returns></returns>
    inline float CalcYawFromDirection(const DirectX::XMFLOAT3& direction)
    {
        constexpr float EPSILON_SQ = 1e-6f;
        const float lengthSq = direction.x * direction.x + direction.z * direction.z;

        if (lengthSq < EPSILON_SQ)
        {
            return 0.0f;
        }

        return std::atan2(-direction.x, -direction.z);
    }

    /// <summary>
    /// 現在位置と目標位置から向くYaw角を計算
    /// </summary>
    /// <param name="fromPos"></param>
    /// <param name="toPos"></param>
    /// <returns></returns>
    inline float CalcLookAtYaw(const DirectX::XMFLOAT3& fromPos, const DirectX::XMFLOAT3& toPos)
    {
        const DirectX::XMFLOAT3 diff = { toPos.x - fromPos.x, 0.0f, toPos.z - fromPos.z };

        return CalcYawFromDirection(diff);
    }

    /// <summary>
    /// 現在位置と目標位置から向く回転クォータニオンを計算
    /// </summary>
    /// <param name="fromPos"></param>
    /// <param name="toPos"></param>
    /// <returns></returns>
    inline DirectX::XMFLOAT4 CalcLookAtRotation(
        const DirectX::XMFLOAT3& fromPos,
        const DirectX::XMFLOAT3& toPos
    )
    {
        const float yaw = CalcLookAtYaw(fromPos, toPos);
        const DirectX::XMVECTOR qRot = DirectX::XMQuaternionRotationRollPitchYaw(0.0f, yaw, 0.0f);

        DirectX::XMFLOAT4 result{};
        DirectX::XMStoreFloat4(&result, qRot);
        return result;
    }

}
