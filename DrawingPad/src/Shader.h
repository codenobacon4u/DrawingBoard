#pragma once

#include <map>
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "Utils.h"

enum class ShaderType : uint32_t {
	Vertex = 0x0001,
	TessControl = 0x0002,
	TessEval = 0x0004,
	Geometry = 0x0008,
	Fragment = 0x0010,
	Compute = 0x0020,
	AllGraphics = 0x001F,
	RayGen = 0x0100,
	AnyHit = 0x0200,
	ClosestHit = 0x0400,
	Miss = 0x0800,
	Intersection = 0x1000,
	Callable = 0x2000,
	Task = 0x0040,
	Mesh = 0x0080,
	Count = 15
};

inline constexpr ShaderType operator&(ShaderType x, ShaderType y) {
	return static_cast<ShaderType>(static_cast<uint32_t>(x) & static_cast<uint32_t>(y));
}

inline constexpr ShaderType operator|(ShaderType x, ShaderType y) {
	return static_cast<ShaderType>(static_cast<uint32_t>(x) | static_cast<uint32_t>(y));
}

inline constexpr ShaderType& operator&=(ShaderType& x, ShaderType y) {
	x = x & y;
	return x;
}

inline constexpr ShaderType& operator|=(ShaderType& x, ShaderType y) {
	x = x | y;
	return x;
}

struct ShaderDesc {
	ShaderType Type = ShaderType::Fragment;
	std::string Name;
	std::string EntryPoint = "main";
	std::string Path;
	std::string Src;
};

enum class ResourceBindingType
{
	Input,
	InputAttachment,
	Output,
	Image,
	ImageSampler,
	ImageStorage,
	Sampler,
	UniformBuffer,
	StorageBuffer,
	SampledBuffer,
	PushConstant,
	SpecialConstant,
	All
};

enum class ResourceBindingMode
{
	Static,
	Dynamic,
	UpdateAfterBind
};

struct ShaderResourceBinding
{
	ShaderType Stages;
	ResourceBindingType Type;
	ResourceBindingMode Mode;
	uint32_t Set = 0;
	uint32_t Binding = 0;
	uint32_t Location = 0;
	uint32_t InputAttachIndex = 0;
	uint32_t VecSize = 0;
	uint32_t Columns = 0;
	uint32_t ArraySize = 0;
	uint32_t Offset = 0;
	uint32_t Size = 0;
	uint32_t ConstantId = 0;
	uint32_t Qualifiers = 0;
	std::string Name = "";

	bool operator==(const ShaderResourceBinding& rhs) const {
		return Stages == rhs.Stages &&
			Type == rhs.Type &&
			Mode == rhs.Mode &&
			Set == rhs.Set &&
			Binding == rhs.Binding &&
			Location == rhs.Location &&
			InputAttachIndex == rhs.InputAttachIndex &&
			VecSize == rhs.VecSize &&
			Columns == rhs.Columns &&
			ArraySize == rhs.ArraySize &&
			Offset == rhs.Offset &&
			Size == rhs.Size &&
			ConstantId == rhs.ConstantId &&
			Qualifiers == rhs.Qualifiers &&
			Name == rhs.Name;
	}
};

struct ShaderLayout
{
	std::map<uint32_t, std::map<uint32_t, ShaderResourceBinding>> Resources;
	std::vector<ShaderResourceBinding> Inputs;
	std::vector<ShaderResourceBinding> Outputs;
	std::vector<ShaderResourceBinding> Constants;
	uint32_t SetCount = 0;
};

class Shader {
public:
	Shader(const ShaderDesc& desc)
		: m_Desc(desc) {}

	virtual ~Shader() {}

	const ShaderDesc& GetDesc() { return m_Desc; }

	ShaderLayout GetLayout() { return m_Layout; }
	size_t GetHash() { return m_Hash; }

protected:
	ShaderDesc m_Desc;
	ShaderLayout m_Layout;
	size_t m_Hash = 0;
};

class ShaderProgram {
public:
	ShaderProgram(Shader* vertShader, Shader* fragShader)
	{
		m_Shaders[ShaderType::Vertex] = vertShader;
		m_Shaders[ShaderType::Fragment] = fragShader;
	}

	virtual ~ShaderProgram() {}

	virtual void AddShader(Shader* shader) = 0;
	virtual void Build() = 0;

	virtual size_t GetHash() = 0;

protected:
	std::map<ShaderType, Shader*> m_Shaders;
};
