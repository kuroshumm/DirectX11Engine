#pragma once
#include <cstdint>
#include <vector>
#include <cassert>

namespace Engine
{
    using Entity = uint32_t;
    const Entity NullEntity = 0xFFFFFFFF; // 無効なエンティティを表す定数

    class ISparceSet
    {
    public:
        virtual ~ISparceSet() = default;

        virtual size_t Size() const = 0;
        virtual bool Contains(Entity entity) const = 0;
        virtual void Remove(Entity entity) = 0;
    };

    template<typename T>
    class SparceSet : public ISparceSet
    {
    public:
        SparceSet() = default;

    public:
        /// <summary>
        /// 登録されてるEntityの数を取得
        /// </summary>
        /// <returns>m_instances.size() によって示される要素数を size_t 型で返します。</returns>
        size_t Size() const override
        {
            return m_instances.size();
        }

        /// <summary>
        /// Entityが登録されてるか
        /// </summary>
        /// <param name="entity"></param>
        /// <returns></returns>
        bool Contains(Entity entity) const override
        {
            if (entity >= m_sparce.size())
            {
                return false;
            }

            int denseIndex = m_sparce[entity];
            if (denseIndex >= m_dense.size())
            {
                return false;
            }

            if (m_dense[denseIndex] != entity)
            {
                return false;
            }

            return true;
        }

        /// <summary>
        /// 登録してるEntityを消去
        /// </summary>
        /// <param name="entity"></param>
        void Remove(Entity entity) override
        {
            if (!Contains(entity))
            {
                return;
            }

            // 消去対象が格納されている配列のインデックス
            uint32_t index = m_sparce[entity];

            // 末尾データの取得
            Entity lastDense = m_dense[m_dense.size()-1];
            T lastInstance = m_instances[m_instances.size()-1];

            // 指定のEntityを末尾データに上書き
            m_dense[index] = lastDense;
            m_instances[index] = std::move(lastInstance);

            // 末尾データの消去
            m_dense.pop_back();
            m_instances.pop_back();

            // m_sparceを更新
            m_sparce[lastDense] = index;
            m_sparce[entity] = NullEntity;
        }


        /// <summary>
        /// EntityとComponentを追加
        /// </summary>
        /// <param name="entity"></param>
        /// <param name="component"></param>
        /// <returns></returns>
        T& Emplace(Entity entity, T&& component)
        {
            // 既に存在する場合は既存データの参照を返す
            if (Contains(entity))
            {
                return Get(entity);
            }

            // m_sparceを必要に応じて拡張
            if (entity >= m_sparce.size())
            {
                m_sparce.resize(entity + 1, NullEntity);
            }
            m_sparce[entity] = static_cast<uint32_t>(m_dense.size());

            // 
            m_dense.push_back(entity);
            m_instances.push_back(std::move(component));

            return m_instances.back();
        }
        /// <summary>
        /// EntityからComponentを取得
        /// </summary>
        /// <param name="entity"></param>
        /// <returns></returns>
        T& Get(Entity entity)
        {
            assert(Contains(entity));

            int index = m_sparce[entity];
            return m_instances[index];
        }

        Entity GetEntityAt(size_t index) const
        {
            assert(m_dense.size() > index);

            return m_dense[index];
        }

    private:
        std::vector<uint32_t> m_sparce; // Entity IDをインデックスとして、m_dense内の位置を指す配列
        std::vector<Entity> m_dense;    // 格納されているEntityのIDを隙間なく並べる配列
        std::vector<T> m_instances;     // コンポーネントの実データを格納する配列
    };
}
