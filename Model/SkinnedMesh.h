#pragma once

#include "Graphics.h"

namespace Engine
{
    class SkinnedMesh
    {
    public:
        SkinnedMesh(Graphics* pGfx);
        ~SkinnedMesh();

        // 頂点配列およびインデックス配列からの物理バッファ生成
        HRESULT Create(const std::vector<SkinnedVertex>& vertices, const std::vector<WORD>& indices);
        // パイプラインへのバッファバインドとドローコールの発行
        void Draw();

    private:
        Graphics* m_pGfx;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_pVertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_pIndexBuffer;
        UINT m_indexCount;
    };
}
