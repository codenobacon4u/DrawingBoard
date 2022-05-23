#pragma once
#include <memory>
#include <string>
#include <stdexcept>

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

template<typename ... Args>
std::string string_format(const std::string& format, Args ... args)
{
	int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
	if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
	auto size = static_cast<size_t>(size_s);
	std::unique_ptr<char[]> buf(new char[size]);
	std::snprintf(buf.get(), size, format.c_str(), args ...);
	return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}