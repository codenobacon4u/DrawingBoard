#pragma once

#include <string>
#include <map>
#include <glm/glm.hpp>

enum class ShaderType : uint32_t {
	Fragment = 0,
	Vertex = 1,
	Geometry = 2,
	TessControl = 3,
	TessEval = 4,
	Mesh = 5,
	RayGen = 6,
	RayMiss = 7,
	RayClosest = 8,
	Compute = 9,
	Count = 10
};

enum class DescType : uint32_t {
	Sampler = 0,
	CombinedImageSampler,
	SampledImage,
	StorageImage,
	UniformTexelBuffer,
	StorageTexelBuffer,
	UniformBuffer,
	StorageBuffer,
	UniformBufferDynamic,
	StorageBufferDynamic,
	InputAttachment,
	Unknown
};

struct ShaderDesc {
	ShaderType Type = ShaderType::Fragment;
	std::string Name;
	std::string EntryPoint = "main";
	std::string Path;
	std::string Src;
};

struct DescriptorBinding {
	uint32_t Binding = 0;
	uint32_t DescriptorCount = 1;
	DescType DescriptorType = DescType::Unknown;
	bool IsFloat = false;

	bool operator==(const DescriptorBinding& r) const
	{
		return Binding == r.Binding &&
			DescriptorCount == r.DescriptorCount &&
			DescriptorType == r.DescriptorType &&
			IsFloat == r.IsFloat;
	}
};

struct DescriptorSetLayout {
	uint32_t SampledImageCount = 0;
	std::vector<DescriptorBinding> SampledImages;
	//uint32_t SampledImages = 0;
	uint32_t StorageImageCount = 0;
	std::vector<DescriptorBinding> StorageImages;
	//uint32_t StorageImages = 0;
	uint32_t UniformBufferCount = 0;
	std::vector<DescriptorBinding> UniformBuffers;
	//uint32_t UniformBuffers = 0;
	uint32_t StorageBufferCount = 0;
	std::vector<DescriptorBinding> StorageBuffers;
	//uint32_t StorageBuffers = 0;
	uint32_t SampledBufferCount = 0;
	std::vector<DescriptorBinding> SampledBuffers;
	//uint32_t SampledBuffers = 0;
	uint32_t InputAttachmentCount = 0;
	std::vector<DescriptorBinding> InputAttachments;
	//uint32_t InputAttachments = 0;
	uint32_t SamplerCount = 0;
	std::vector<DescriptorBinding> Samplers;
	//uint32_t Samplers = 0;
	uint32_t SeparateImageCount = 0;
	std::vector<DescriptorBinding> SeparateImages;
	//uint32_t SeparateImages = 0;

	bool operator==(const DescriptorSetLayout& r) const
	{
		return SampledImages == r.SampledImages &&
			StorageImages == r.StorageImages &&
			UniformBuffers == r.UniformBuffers &&
			StorageBuffers == r.StorageBuffers &&
			SampledBuffers == r.SampledBuffers &&
			InputAttachments == r.InputAttachments &&
			Samplers == r.Samplers &&
			SeparateImages == r.Samplers;
	}

	bool operator!=(const DescriptorSetLayout& r) const {
		return !(*this == r);
	}
};

struct ResourceLayout {
	DescriptorSetLayout Sets[16];
	uint32_t Inputs = 0;
	uint32_t Outputs = 0;
	uint32_t PushConstantSize = 0;
	uint32_t SpecConstants = 0;
	uint32_t BindlessSets = 0;
};

struct CombinedLayout {
	uint32_t Inputs = 0;
	uint32_t Outputs = 0;
	DescriptorSetLayout Sets[16] = {};
	uint32_t BindingStages[16][32] = {};
	uint32_t SetStages[16] = {};
	uint32_t DescriptorSet = 0;
	uint32_t BindlessSets = 0;
	uint32_t SpecConstants[static_cast<uint32_t>(ShaderType::Count)] = {};
	uint32_t CombinedSpecConstants = 0;
};

class Shader {
public:
	Shader(const ShaderDesc& desc)
		: m_Desc(desc) {}

	virtual ~Shader() {}

	static Shader* Create(const std::string& frag, const std::string& vert);
	static Shader* Create(const std::string& path);

	const ShaderDesc& GetDesc() { return m_Desc; }

	ResourceLayout GetLayout() { return m_Layout; }

protected:
	ShaderDesc m_Desc;
	ResourceLayout m_Layout;
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

protected:
	std::map<ShaderType, Shader*> m_Shaders;
};