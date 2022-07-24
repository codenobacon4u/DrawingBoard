#pragma once

enum class BufferBindFlags {
	None,
	Vertex,
	Index,
	Staging,
	Uniform,
	ShaderResource,
	StreamOutput,
	Unordered,
	IndirectDraw,
	RayTracing
};

enum class BufferUsageFlags {
	Immutable,
	Default,
	Dynamic,
	Staging,
	Unified
};

enum class AccessFlags {
	None,
	Read,
	Write
};

enum class BufferModeFlags {
	Undefined,
	Formatted,
	Structured,
	Raw
};

typedef struct BufferDesc {
	uint32_t Size = 0;
	BufferBindFlags BindFlags = BufferBindFlags::None;
	BufferUsageFlags Usage = BufferUsageFlags::Default;
	AccessFlags CPUAccess = AccessFlags::None;
	BufferModeFlags Mode = BufferModeFlags::Undefined;
	uint32_t Stride = 0;
	bool Buffered = false;

	BufferDesc() = default;
} BufferDesc;

class Buffer
{
public:
	Buffer(const BufferDesc& desc)
		: m_Desc(desc), m_Data(nullptr)
	{}
	Buffer(const BufferDesc& desc, uint8_t* data)
		: m_Desc(desc), m_Data(data)
	{}

	virtual ~Buffer() {}

	virtual void* MapMemory() = 0;
	virtual void UnmapMemory() = 0;
	virtual void FlushMemory() = 0;

	virtual void Update(uint64_t offset, uint64_t size, const void* data) = 0;

	BufferDesc GetDesc() const { return m_Desc; }

	uint32_t GetStride() { return m_Desc.Stride; }
	uint32_t GetSize() { return m_Desc.Size; }
	uint32_t GetNumElements() { return m_Desc.Size / m_Desc.Stride; }

protected:
	void* m_Data;
	BufferDesc m_Desc;
};
