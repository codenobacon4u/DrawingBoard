#include "pwpch.h"

#include "ShaderVK.h"
#include "GraphicsDeviceVK.h"

#include <shaderc/shaderc.hpp>

namespace VkAPI {

	template<ResourceBindingType T>
	inline void ReadResource(const spirv_cross::Compiler &compiler, ShaderType stage, std::vector<ResourceBinding> &resources, uint32_t* maxSet) {
		
	}

	template <>
	inline void ReadResource<ResourceBindingType::Input>(const spirv_cross::Compiler& compiler, ShaderType stage, std::vector<ResourceBinding>& resources, uint32_t* maxSet) {
		auto cResources = compiler.get_shader_resources().stage_inputs;
		for (auto& resource : cResources)
		{
			const auto& type = compiler.get_type_from_variable(resource.id);

			ResourceBinding res = {};
			res.Type = ResourceBindingType::Input;
			res.Stages = stage;
			res.Name = resource.name;

			// VecSize
			res.VecSize = type.vecsize;
			res.Columns = type.columns;
			// ArraySize
			res.ArraySize = type.array.size() ? type.array[0] : 1;
			res.Location = compiler.get_decoration(resource.id, spv::DecorationLocation);

			resources.push_back(res);
		}
	}

	template <>
	inline void ReadResource<ResourceBindingType::InputAttachment>(const spirv_cross::Compiler& compiler, ShaderType stage, std::vector<ResourceBinding>& resources, uint32_t* maxSet) {
		auto cResources = compiler.get_shader_resources().subpass_inputs;
		for (auto& resource : cResources)
		{
			const auto& type = compiler.get_type_from_variable(resource.id);

			ResourceBinding res = {};
			res.Type = ResourceBindingType::InputAttachment;
			res.Stages = ShaderType::Fragment;
			res.Name = resource.name;

			// ArraySize
			res.ArraySize = type.array.size() ? type.array[0] : 1;
			res.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			res.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			res.InputAttachIndex = compiler.get_decoration(resource.id, spv::DecorationInputAttachmentIndex);

			*maxSet = res.Set > *maxSet ? res.Set : *maxSet;

			resources.push_back(res);
		}
	}

	template <>
	inline void ReadResource<ResourceBindingType::Output>(const spirv_cross::Compiler& compiler, ShaderType stage, std::vector<ResourceBinding>& resources, uint32_t* maxSet) {
		auto cResources = compiler.get_shader_resources().stage_outputs;
		for (auto& resource : cResources)
		{
			const auto& type = compiler.get_type_from_variable(resource.id);

			ResourceBinding res = {};
			res.Type = ResourceBindingType::Output;
			res.Stages = stage;
			res.Name = resource.name;

			// VecSize
			res.VecSize = type.vecsize;
			res.Columns = type.columns;
			// ArraySize
			res.ArraySize = type.array.size() ? type.array[0] : 1;
			res.Location = compiler.get_decoration(resource.id, spv::DecorationLocation);

			resources.push_back(res);
		}
	}

	template <>
	inline void ReadResource<ResourceBindingType::Image>(const spirv_cross::Compiler& compiler, ShaderType stage, std::vector<ResourceBinding>& resources, uint32_t* maxSet) {
		auto cResources = compiler.get_shader_resources().separate_images;
		for (auto& resource : cResources)
		{
			const auto& type = compiler.get_type_from_variable(resource.id);

			ResourceBinding res = {};
			res.Type = ResourceBindingType::Image;
			res.Stages = stage;
			res.Name = resource.name;

			// ArraySize
			res.ArraySize = type.array.size() ? type.array[0] : 1;
			res.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			res.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

			*maxSet = res.Set > *maxSet ? res.Set : *maxSet;

			resources.push_back(res);
		}
	}

	template <>
	inline void ReadResource<ResourceBindingType::ImageSampler>(const spirv_cross::Compiler& compiler, ShaderType stage, std::vector<ResourceBinding>& resources, uint32_t* maxSet) {
		auto cResources = compiler.get_shader_resources().sampled_images;
		for (auto& resource : cResources)
		{
			const auto& type = compiler.get_type_from_variable(resource.id);

			ResourceBinding res = {};
			res.Type = ResourceBindingType::ImageSampler;
			res.Stages = stage;
			res.Name = resource.name;

			// ArraySize
			res.ArraySize = type.array.size() ? type.array[0] : 1;
			res.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			res.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

			*maxSet = res.Set > *maxSet ? res.Set : *maxSet;

			resources.push_back(res);
		}
	}

	template <>
	inline void ReadResource<ResourceBindingType::ImageStorage>(const spirv_cross::Compiler& compiler, ShaderType stage, std::vector<ResourceBinding>& resources, uint32_t* maxSet) {
		auto cResources = compiler.get_shader_resources().storage_images;
		for (auto& resource : cResources)
		{
			const auto& type = compiler.get_type_from_variable(resource.id);

			ResourceBinding res = {};
			res.Type = ResourceBindingType::ImageStorage;
			res.Stages = stage;
			res.Name = resource.name;

			// ArraySize
			res.ArraySize = type.array.size() ? type.array[0] : 1;
			res.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			res.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			// NonRead/Write
			res.Qualifiers = 1 | 2;

			*maxSet = res.Set > *maxSet ? res.Set : *maxSet;

			resources.push_back(res);
		}
	}

	template <>
	inline void ReadResource<ResourceBindingType::Sampler>(const spirv_cross::Compiler& compiler, ShaderType stage, std::vector<ResourceBinding>& resources, uint32_t* maxSet) {
		auto cResources = compiler.get_shader_resources().separate_samplers;
		for (auto& resource : cResources)
		{
			const auto& type = compiler.get_type_from_variable(resource.id);

			ResourceBinding res = {};
			res.Type = ResourceBindingType::Sampler;
			res.Stages = stage;
			res.Name = resource.name;

			// ArraySize
			res.ArraySize = type.array.size() ? type.array[0] : 1;
			res.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			res.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

			*maxSet = res.Set > *maxSet ? res.Set : *maxSet;

			resources.push_back(res);
		}
	}

	template <>
	inline void ReadResource<ResourceBindingType::UniformBuffer>(const spirv_cross::Compiler& compiler, ShaderType stage, std::vector<ResourceBinding>& resources, uint32_t* maxSet) {
		auto cResources = compiler.get_shader_resources().uniform_buffers;
		for (auto& resource : cResources)
		{
			const auto& type = compiler.get_type_from_variable(resource.id);

			ResourceBinding res = {};
			res.Type = ResourceBindingType::UniformBuffer;
			res.Stages = stage;
			res.Name = resource.name;

			res.Size = static_cast<uint32_t>(compiler.get_declared_struct_size_runtime_array(type, 0));
			// ArraySize
			res.ArraySize = type.array.size() ? type.array[0] : 1;
			res.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			res.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

			*maxSet = res.Set > *maxSet ? res.Set : *maxSet;

			resources.push_back(res);
		}
	}

	template <>
	inline void ReadResource<ResourceBindingType::StorageBuffer>(const spirv_cross::Compiler& compiler, ShaderType stage, std::vector<ResourceBinding>& resources, uint32_t* maxSet) {
		auto cResources = compiler.get_shader_resources().storage_buffers;
		for (auto& resource : cResources)
		{
			const auto& type = compiler.get_type_from_variable(resource.id);

			ResourceBinding res = {};
			res.Type = ResourceBindingType::StorageBuffer;
			res.Stages = stage;
			res.Name = resource.name;

			res.Size = static_cast<uint32_t>(compiler.get_declared_struct_size_runtime_array(type, 0));
			// ArraySize
			res.ArraySize = type.array.size() ? type.array[0] : 1;
			res.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			res.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			res.Qualifiers = 1 | 2;

			*maxSet = res.Set > *maxSet ? res.Set : *maxSet;

			resources.push_back(res);
		}
	}

	ShaderVK::ShaderVK(GraphicsDeviceVK* device, const ShaderDesc& desc)
		: Shader(desc), m_Device(device)
	{
		if (!desc.Path.empty() && desc.Src.empty())
		{
			if (!LoadShaderFromFile(desc.Path))
				throw std::runtime_error("Failed to load shader file");
		}
		else if (desc.Path.empty() && !desc.Src.empty())
			if (!LoadShaderFromSrc(desc.Src))
				throw std::runtime_error("Failed to load shader from source");

		m_Stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		if (desc.Type == ShaderType::Fragment)
			m_Stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		else if (desc.Type == ShaderType::Vertex)
			m_Stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
		m_Stage.module = m_Module;
		m_Stage.pName = desc.EntryPoint.c_str();
	}

	ShaderVK::~ShaderVK()
	{
		vkDestroyShaderModule(m_Device->Get(), m_Module, nullptr);
	}

	bool ShaderVK::LoadShaderFromFile(const std::string& path)
	{
		shaderc_shader_kind type;
		switch (m_Desc.Type)
		{
		case ShaderType::Fragment:
			type = shaderc_fragment_shader;
			break;
		case ShaderType::Vertex:
			type = shaderc_vertex_shader;
			break;
		case ShaderType::Geometry:
			type = shaderc_geometry_shader;
			break;
		case ShaderType::TessControl:
			type = shaderc_tess_control_shader;
			break;
		case ShaderType::Mesh:
			type = shaderc_mesh_shader;
			break;
		case ShaderType::RayGen:
			type = shaderc_raygen_shader;
			break;
		case ShaderType::Compute:
			type = shaderc_compute_shader;
			break;
		default:
			//throw std::runtime_error("Unkown shader stage");
			break;
		}

		// Load shader as string from file
		std::ifstream file(path.c_str());
		if (!file.is_open())
			return false;
		std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		// Compile the file data into SPIR-V
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		shaderc::CompilationResult result = compiler.CompileGlslToSpv(source, type, m_Desc.EntryPoint.c_str(), options);
		if (result.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			std::cerr << result.GetErrorMessage() << std::endl;
			return false;
		}

		std::vector<uint32_t> spirv = { result.cbegin(), result.cend() };
		Reflect(spirv);

		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = spirv.size() * sizeof(uint32_t);
		createInfo.pCode = spirv.data();

		if (vkCreateShaderModule(m_Device->Get(), &createInfo, nullptr, &m_Module))
			return false;
		return true;
	}

	bool ShaderVK::LoadShaderFromSrc(const std::string& src)
	{
		shaderc_shader_kind type;
		switch (m_Desc.Type)
		{
		case ShaderType::Fragment:
			type = shaderc_fragment_shader;
			break;
		case ShaderType::Vertex:
			type = shaderc_vertex_shader;
			break;
		case ShaderType::Geometry:
			type = shaderc_geometry_shader;
			break;
		case ShaderType::TessControl:
			type = shaderc_tess_control_shader;
			break;
		case ShaderType::Mesh:
			type = shaderc_mesh_shader;
			break;
		case ShaderType::RayGen:
			type = shaderc_raygen_shader;
			break;
		case ShaderType::Compute:
			type = shaderc_compute_shader;
			break;
		default:
			//throw std::runtime_error("Unkown shader stage");
			break;
		}
		// Since we are getting the shader from a string already, we don't need to read it in.
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		shaderc::CompilationResult result = compiler.CompileGlslToSpv(src, type, m_Desc.EntryPoint.c_str(), options);
		if (result.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			std::cerr << result.GetErrorMessage() << std::endl;
			return false;
		}
		// Get shader resources for reflection
		std::vector<uint32_t> spirv = { result.cbegin(), result.cend() };\
		Reflect(spirv);

		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = spirv.size() * sizeof(uint32_t);
		createInfo.pCode = spirv.data();

		if (vkCreateShaderModule(m_Device->Get(), &createInfo, nullptr, &m_Module))
			return false;

		return true;
	}

	void ShaderVK::Reflect(std::vector<uint32_t> spirv)
	{
		// Get shader resources for reflection
		spirv_cross::Compiler comp(spirv);
		uint32_t sets = 0;
		ReadResource<ResourceBindingType::Input>(comp, m_Desc.Type, m_Layout.Resources, &sets);
		ReadResource<ResourceBindingType::Output>(comp, m_Desc.Type, m_Layout.Resources, &sets);
		ReadResource<ResourceBindingType::InputAttachment>(comp, m_Desc.Type, m_Layout.Resources, &sets);
		ReadResource<ResourceBindingType::Image>(comp, m_Desc.Type, m_Layout.Resources, &sets);
		ReadResource<ResourceBindingType::ImageSampler>(comp, m_Desc.Type, m_Layout.Resources, &sets);
		ReadResource<ResourceBindingType::ImageStorage>(comp, m_Desc.Type, m_Layout.Resources, &sets);
		ReadResource<ResourceBindingType::Sampler>(comp, m_Desc.Type, m_Layout.Resources, &sets);
		ReadResource<ResourceBindingType::UniformBuffer>(comp, m_Desc.Type, m_Layout.Resources, &sets);
		ReadResource<ResourceBindingType::StorageBuffer>(comp, m_Desc.Type, m_Layout.Resources, &sets);
		
		auto push_constants = comp.get_shader_resources().push_constant_buffers;

		for (auto &resource : push_constants)
		{
			const auto& type = comp.get_type_from_variable(resource.id);
			uint32_t offset = UINT32_MAX;
			for (auto i = 0; i < type.member_types.size(); i++)
				offset = std::min(offset, comp.get_member_decoration(type.self, i, spv::DecorationOffset));

			ResourceBinding res = {};
			res.Type = ResourceBindingType::PushConstant;
			res.Stages = m_Desc.Type;
			res.Name = resource.name;
			res.Offset = offset;

			res.Size = static_cast<uint32_t>(comp.get_declared_struct_size_runtime_array(type, 0));

			res.Size -= res.Offset;

			m_Layout.Resources.push_back(res);
		}

		auto special_constants = comp.get_specialization_constants();

		for (auto& resource : special_constants)
		{
			auto& value = comp.get_constant(resource.id);
			auto& type = comp.get_type(value.constant_type);

			ResourceBinding res = {};
			res.Type = ResourceBindingType::SpecialConstant;
			res.Stages = m_Desc.Type;
			res.Name = comp.get_name(resource.id);
			res.Offset = 0;
			res.ConstantId = resource.constant_id;

			switch (type.basetype)
			{
			case spirv_cross::SPIRType::BaseType::Boolean:
			case spirv_cross::SPIRType::BaseType::Char:
			case spirv_cross::SPIRType::BaseType::Int:
			case spirv_cross::SPIRType::BaseType::UInt:
			case spirv_cross::SPIRType::BaseType::Float:
				res.Size = 4;
				break;
			case spirv_cross::SPIRType::BaseType::Int64:
			case spirv_cross::SPIRType::BaseType::UInt64:
			case spirv_cross::SPIRType::BaseType::Double:
				res.Size = 8;
				break;
			default:
				res.Size = 0;
				break;
			}

			m_Layout.Resources.push_back(res);
		}
		m_Layout.SetCount = sets;
	}
	
	ShaderProgramVK::ShaderProgramVK(GraphicsDeviceVK* device, ShaderVK* vertShader, ShaderVK* fragShader)
		: ShaderProgram(vertShader, fragShader), m_Device(device)
	{
		m_DescCache = DBG_NEW DescriptorSetLayoutCacheVK(m_Device);
	}

	ShaderProgramVK::~ShaderProgramVK()
	{
		delete m_DescCache;
		vkDestroyPipelineLayout(m_Device->Get(), m_PipeLayout, nullptr);
	}

	void ShaderProgramVK::AddShader(Shader* shader)
	{
		m_Shaders[shader->GetDesc().Type] = shader;
	}

	void ShaderProgramVK::Build()
	{
		uint32_t maxSets = 1;
		for (auto const& [type, shader] : m_Shaders)
		{
			maxSets = std::max(maxSets, shader->GetLayout().SetCount);
			for (ResourceBinding resource : shader->GetLayout().Resources)
				m_Layout.Resources.emplace_back(resource);
		}
		std::vector<std::vector<ResourceBinding>> resourceSets(maxSets);
		std::vector<VkDescriptorSetLayout> layouts(maxSets);
		for (uint32_t i = 0; i < m_Layout.Resources.size(); i++)
		{
			uint32_t binding = m_Layout.Resources[i].Binding;
			uint32_t set = m_Layout.Resources[i].Set;
			m_Layout.BindingStagesMask[set][binding] |= (uint32_t)m_Layout.Resources[i].Stages;
			resourceSets[m_Layout.Resources[i].Set].emplace_back(m_Layout.Resources[i]);
		}
		for (uint32_t i = 0; i < maxSets; i++)
			layouts[i] = m_DescCache->GetLayout(resourceSets[i], m_Layout.BindingStagesMask[i]);

		VkPipelineLayoutCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		createInfo.setLayoutCount = maxSets;
		createInfo.pSetLayouts = layouts.data();
		vkCreatePipelineLayout(m_Device->Get(), &createInfo, nullptr, &m_PipeLayout);
	}
}