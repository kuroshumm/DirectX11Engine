#include <cstdint>

namespace Engine
{
	class TyoeID
	{
	public:
		template<typename T>
		static uint32_t Get()
		{
			static const uint32_t id = GetNextID();
			return id;
		}

	private:
		static uint32_t GetNextID()
		{
			static uint32_t lastID = 0;
			return lastID++;
		}
	};
}