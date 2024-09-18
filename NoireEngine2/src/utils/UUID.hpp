#include <stdint.h>
#include <cstddef>

class UUID
{
public:
	UUID();
	UUID(uint64_t uuid);
	UUID(const UUID&) = default;

	operator uint64_t() const { return m_UUID; }
private:
	uint64_t m_UUID;
};

template<>
struct std::hash<UUID>
{
	std::size_t operator()(const UUID& uuid) const
	{
		return (uint64_t)uuid;
	}
};
