#include "Registry.h"

namespace Engine
{
    Entity Registry::Create()
    {
        return m_nextEntity++;
    }

    void Registry::Destroy(Entity entity)
    {
        if (entity == NullEntity)
        {
            return;
        }

        for (auto& pool : m_componentPool)
        {
            if (!pool)
            {
                continue;
            }

            pool->Remove(entity);
        }
    }
}
