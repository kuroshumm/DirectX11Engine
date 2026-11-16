#pragma once
#include "Graphics.h"
#include "Common.h"
#include <vector>

namespace Engine
{
    // 頂点・インデックスバッファのライフサイクルとIAステージへのバインドを管理するクラス
    class Mesh
    {
    public:
        Mesh(Graphics* pGfx);
        ~Mesh();

        // 頂点配列およびインデックス配列からの物理バッファ生成
        HRESULT Create(const std::vector<Vertex>& vertices, const std::vector<WORD>& indices);

        // パイプラインへのバッファバインドとドローコールの発行
        void Draw();

    private:
        Graphics* m_pGfx;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_pVertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_pIndexBuffer;
        UINT m_indexCount;
    };
}
