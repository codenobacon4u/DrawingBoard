#pragma once

class Hasher {
public:
	explicit Hasher(uint64_t hash)
		: m_Hash(hash)
	{}

	Hasher() = default;

	template <typename T>
	inline void AddHash(const T& v)
	{
		std::hash<T> h;
		m_Hash ^= h(v) + 0x9e3779ba + (m_Hash << 6) + (m_Hash >> 2);
	}

	inline uint64_t Get() const
	{
		return m_Hash;
	}

private:
	uint64_t m_Hash = 0x5bd1e995;
};