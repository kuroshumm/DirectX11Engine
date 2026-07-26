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
		Entity Create();
		void Destroy(Entity entity);

		template <typename T>
		T& Emplace(Entity entity, T&& component)
		{

		}

		template <typename T>
		T& Get(Entity entity)
		{

		}

	private:
		/// <summary>
		/// 型 T に対応するコンポーネントプール（SparceSet<T>）への共有ポインタをオプションで取得します。
		/// 対応するプールが存在しない場合は空の optional を返します。
		/// </summary>
		/// <typeparam name="T">取得するコンポーネントの型。TypeID::Get<T>() によってプールのインデックスを決定します。</typeparam>
		/// <returns>プールが存在する場合は std::optional に包まれた std::shared_ptr<SparceSet<T>> を返します。存在しない場合は std::nullopt（空の optional）を返します。</returns>
		template <typename T>
		std::optional<std::shared_ptr<SparceSet<T>>> GetPool()
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