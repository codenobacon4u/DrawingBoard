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

//TODO: This might change to be similar to Paperworks BufferElements if there are no cases that
//		can't be fit by defined elements
typedef struct BufferData {
	const void* Value = nullptr;
	uint32_t Size = 0;

	BufferData() = default;
} BufferData;

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
	Buffer(const BufferDesc& desc, void* data)
		: m_Desc(desc), m_Data(data)
	{}

	virtual void BeginStaging() = 0;
	virtual void EndStaging() = 0;

	uint32_t GetStride() { return m_Desc.Stride; }
	uint32_t GetSize() { return m_Desc.Size; }
	uint32_t GetNumElements() { return m_Desc.Size / m_Desc.Stride; }

protected:
	void* m_Data;
	BufferDesc m_Desc;
};