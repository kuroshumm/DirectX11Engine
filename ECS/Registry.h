#pragma once

#include <unordered_map>
#include <typeindex>
#include <memory>
#include <optional>

#include "SparceSet.h"
#include "TypeID.h"

namespace Engine
{
    class Registry
    {
    public:
        Registry() = default;

    public:
        /// <summary>
        /// 重複しない Entity ID を発行して返す
        /// </summary>
        /// <returns></returns>
        Entity Create();
        /// <summary>
        /// 指定された Entity とそれに紐づく全種類のコンポーネントを一括破棄
        /// </summary>
        /// <param name="entity"></param>
        void Destroy(Entity entity);

        /// <summary>
        /// 指定された Entity に対して型Tのコンポーネントを追加
        /// プールが存在しない場合は自動的に遅延生成する
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="entity"></param>
        /// <param name="component"></param>
        /// <returns></returns>
        template <typename T>
        T& Emplace(Entity entity, T&& component)
        {
            assert(entity != NullEntity);

            auto poolOpt = GetPool<T>();
            std::shared_ptr<SparceSet<T>> pool;

            if (!poolOpt.has_value() || !poolOpt.value())
            {
                uint32_t typeID = TypeID::Get<T>();
                if (typeID >= m_componentPool.size())
                {
                    m_componentPool.resize(typeID + 1, nullptr);
                }

                pool = std::make_shared<SparceSet<T>>();
                m_componentPool[typeID] = pool;
            }
            else
            {
                pool = poolOpt.value();
            }

            T& rtn = pool->Emplace(entity, std::move(component));
            return rtn;
        }

        /// <summary>
        /// 指定された Entity の型 T のコンポーネント参照を取得
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="entity"></param>
        /// <returns></returns>
        template <typename T>
        T& Get(Entity entity)
        {
            assert(Contains<T>(entity) && "Entity does not have the component!");

            return GetPool<T>().value()->Get(entity);
        }

        /// <summary>
        /// 指定された Entity が型 T のコンポーネントを保持しているか確認
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="entity"></param>
        /// <returns></returns>
        template <typename T>
        bool Contains(Entity entity) const
        {
            auto pool = GetPool<T>();

            return pool.has_value() && pool.value()
                ? pool.value()->Contains(entity)
                : false;
        }

        /// <summary>
        /// Registryが持つ全てのコンポーネントを返す
        /// </summary>
        /// <typeparam name="...Components"></typeparam>
        /// <returns></returns>
        template<typename... Components>
        std::vector<Entity> View()
        {
            std::vector<Entity> result;

            // 指定されたComponentが全てアタッチされてるEntityがあるか判定
            if ((!GetPool<Components>().has_value() || ...))
            {
                return result;
            }

            // 指定されたComponentの一番最初のタイプのEntityを取得
            using TargetType = std::tuple_element_t<0, std::tuple<Components...>>;
            std::shared_ptr<SparceSet<TargetType>> targetPool = GetPool<TargetType>().value();

            // メモリの再確保を防止するために領域を確保
            result.reserve(targetPool->Size());

            // 
            for (size_t i = 0; i < targetPool->Size(); ++i)
            {
                Entity entity = targetPool->GetEntityAt(i);

                if ((Contains<Components>(entity) && ...))
                {
                    result.push_back(entity);
                }
            }

            return result;
        }

    private:
        /// <summary>
        /// 【内部ヘルパー】型 T に対応する SparseSet<T> のプールを検索取得します。
        /// </summary>
        /// <typeparam name="T">取得するコンポーネントの型。TypeID::Get<T>() によってプールのインデックスを決定します。</typeparam>
        /// <returns>プールが存在する場合は std::optional に包まれた std::shared_ptr<SparceSet<T>> を返します。存在しない場合は std::nullopt（空の optional）を返します。</returns>
        template <typename T>
        std::optional<std::shared_ptr<SparceSet<T>>> GetPool() const
        {
            uint32_t typeID = TypeID::Get<T>();
            if (typeID >= m_componentPool.size())
            {
                return std::nullopt;
            }

            return std::static_pointer_cast<SparceSet<T>>(m_componentPool[typeID]);
        }

    private:
        // 次に発行するEntityID
        Entity m_nextEntity = 0;
        // 
        std::vector<std::shared_ptr<ISparceSet>> m_componentPool;
    };


}
