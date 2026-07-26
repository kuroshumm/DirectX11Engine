#include "Registry.h"

namespace Engine
{
	Entity Registry::Create()
	{
		return m_nextEntity++;
	}

	void Registry::Destroy(Entity entity)
	{

	}
}