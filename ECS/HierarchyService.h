// HierarchyUtils.h
#pragma once
#include <entt/entt.hpp>
#include "Components.h"

namespace Engine
{
    class HierarchyService
    {
    public:
        // 循環参照検知（parentCandidate が child の子孫でないか検証）
        static bool IsAncestorOf(entt::registry& registry, entt::entity potentialAncestor, entt::entity target)
        {
            entt::entity curr = target;
            while (curr != entt::null && registry.valid(curr))
            {
                if (curr == potentialAncestor) return true;
                auto* hier = registry.try_get<HierarchyComponent>(curr);
                if (!hier) break;
                curr = hier->parent;
            }
            return false;
        }

        // 親子関係の設定
        static bool SetParent(entt::registry& registry, entt::entity child, entt::entity newParent)
        {
            if (child == newParent) return false;
            if (newParent != entt::null && IsAncestorOf(registry, child, newParent))
            {
                // 循環参照となるため失敗
                return false;
            }

            // 1. 既存の親から child を切り離す（Unlink）
            Unparent(registry, child);

            if (newParent == entt::null)
            {
                return true;
            }

            // 2. 親・子の HierarchyComponent を取得・追加
            auto& childHier = registry.get_or_emplace<HierarchyComponent>(child);
            auto& parentHier = registry.get_or_emplace<HierarchyComponent>(newParent);

            childHier.parent = newParent;
            childHier.depth = parentHier.depth + 1;

            // 3. newParent の firstChild リストの先頭に挿入
            childHier.nextSibling = parentHier.firstChild;
            childHier.prevSibling = entt::null;

            if (parentHier.firstChild != entt::null)
            {
                auto& nextSibHier = registry.get<HierarchyComponent>(parentHier.firstChild);
                nextSibHier.prevSibling = child;
            }
            parentHier.firstChild = child;

            // 4. Dirty フラグを立てる
            if (auto* tc = registry.try_get<TransformComponent>(child))
            {
                tc->isDirty = true;
            }

            return true;
        }

        // 親子関係の解除
        static void Unparent(entt::registry& registry, entt::entity child)
        {
            auto* childHier = registry.try_get<HierarchyComponent>(child);
            if (!childHier || childHier->parent == entt::null) return;

            entt::entity parent = childHier->parent;
            auto* parentHier = registry.try_get<HierarchyComponent>(parent);

            if (parentHier)
            {
                if (parentHier->firstChild == child)
                {
                    parentHier->firstChild = childHier->nextSibling;
                }
            }

            if (childHier->prevSibling != entt::null)
            {
                if (auto* prev = registry.try_get<HierarchyComponent>(childHier->prevSibling))
                {
                    prev->nextSibling = childHier->nextSibling;
                }
            }
            if (childHier->nextSibling != entt::null)
            {
                if (auto* next = registry.try_get<HierarchyComponent>(childHier->nextSibling))
                {
                    next->prevSibling = childHier->prevSibling;
                }
            }

            childHier->parent = entt::null;
            childHier->prevSibling = entt::null;
            childHier->nextSibling = entt::null;
            childHier->depth = 0;
        }
    };
}