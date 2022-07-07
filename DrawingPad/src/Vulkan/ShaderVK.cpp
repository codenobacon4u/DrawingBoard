#include "pwpch.h"
#include "ShaderVK.h"

#include "GraphicsDeviceVK.h"

#include <shaderc/shaderc.hpp>
#ifdef _DEBUG
#undef calloc
#undef free
#undef malloc
#undef realloc
#endif
#include <spirv_cross/spirv_cross.hpp>
#ifdef _DEBUG
#define calloc(c, s)       _calloc_dbg(c, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define free(p)            _free_dbg(p, _NORMAL_BLOCK)
#define malloc(s)          _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define realloc(p, s)      _realloc_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace Vulkan {

	template<ResourceBindingType T>
	inline void ReadResource(const spirv_cross::Compiler &compiler, ShaderType stage, std::map<uint32_t, std::map<uint32_t, ShaderResourceBinding>> &resources, uint32_t* maxSet) {
		
	}

	template<ResourceBindingType T>
	inline void ReadResource(const spirv_cross::Compiler& compiler, ShaderType stage, std::vector<ShaderResourceBinding>& resources, uint32_t* maxSet) {

	}

	template <>
	inline void ReadResource<ResourceBindingType::Input>(const spirv_cross::Compiler& compiler, ShaderType stage, std::vector<ShaderResourceBinding>& resources, uint32_t* maxSet) {
		auto& cResources = compiler.get_shader_resources().stage_inputs;
		for (auto& resource : cResources)
		{
			const auto& type = compiler.get_type_from_variable(resource.id);

			ShaderResourceBinding res = {};
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
	inline void ReadResource<ResourceBindingType::InputAttachment>(const spirv_cross::Compiler& compiler, ShaderType stage, std::map<uint32_t, std::map<uint32_t, ShaderResourceBinding>>& resources, uint32_t* maxSet) {
		auto& cResources = compiler.get_shader_resources().subpass_inputs;
		for (auto& resource : cResources)
		{
			const auto& type = compiler.get_type_from_variable(resource.id);

			ShaderResourceBinding res = {};
			res.Type = ResourceBindingType::InputAttachment;
			res.Stages = ShaderType::Fragment;
			res.Name = resource.name;

			// ArraySize
			res.ArraySize = type.array.size() ? type.array[0] : 1;
			res.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			res.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			res.InputAttachIndex = compiler.get_decoration(resource.id, spv::DecorationInputAttachmentIndex);

			*maxSet = res.Set > *maxSet ? res.Set : *maxSet;

			resources[res.Set].emplace(res.Binding, res);
		}
	}

	template <>
	inline void ReadResource<ResourceBindingType::Output>(const spirv_cross::Compiler& compiler, ShaderType stage, std::vector<ShaderResourceBinding>& resources, uint32_t* maxSet) {
		auto& cResources = compiler.get_shader_resources().stage_outputs;
		for (auto& resource : cResources)
		{
			const auto& type = compiler.get_type_from_variable(resource.id);

			ShaderResourceBinding res = {};
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
	inline void ReadResource<ResourceBindingType::Image>(const spirv_cross::Compiler& compiler, ShaderType stage, std::map<uint32_t, std::map<uint32_t, ShaderResourceBinding>>& resources, uint32_t* maxSet) {
		auto& cResources = compiler.get_shader_resources().separate_images;
		for (auto& resource : cResources)
		{
			const auto& type = compiler.get_type_from_variable(resource.id);

			ShaderResourceBinding res = {};
			res.Type = ResourceBindingType::Image;
			res.Stages = stage;
			res.Name = resource.name;

			// ArraySize
			res.ArraySize = type.array.size() ? type.array[0] : 1;
			res.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			res.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

			*maxSet = res.Set > *maxSet ? res.Set : *maxSet;

			resources[res.Set].emplace(res.Binding, res);
		}
	}

	template <>
	inline void ReadResource<ResourceBindingType::ImageSampler>(const spirv_cross::Compiler& compiler, ShaderType stage, std::map<uint32_t, std::map<uint32_t, ShaderResourceBinding>>& resources, uint32_t* maxSet) {
		auto& cResources = compiler.get_shader_resources().sampled_images;
		for (auto& resource : cResources)
		{
			const auto& type = compiler.get_type_from_variable(resource.id);

			ShaderResourceBinding res = {};
			res.Type = ResourceBindingType::ImageSampler;
			res.Stages = stage;
			res.Name = resource.name;

			// ArraySize
			res.ArraySize = type.array.size() ? type.array[0] : 1;
			res.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			res.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

			*maxSet = res.Set > *maxSet ? res.Set : *maxSet;

			resources[res.Set].emplace(res.Binding, res);
		}
	}

	template <>
	inline void ReadResource<ResourceBindingType::ImageStorage>(const spirv_cross::Compiler& compiler, ShaderType stage, std::map<uint32_t, std::map<uint32_t, ShaderResourceBinding>>& resources, uint32_t* maxSet) {
		auto& cResources = compiler.get_shader_resources().storage_images;
		for (auto& resource : cResources)
		{
			const auto& type = compiler.get_type_from_variable(resource.id);

			ShaderResourceBinding res = {};
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

			resources[res.Set].emplace(res.Binding, res);
		}
	}

	template <>
	inline void ReadResource<ResourceBindingType::Sampler>(const spirv_cross::Compiler& compiler, ShaderType stage, std::map<uint32_t, std::map<uint32_t, ShaderResourceBinding>>& resources, uint32_t* maxSet) {
		auto& cResources = compiler.get_shader_resources().separate_samplers;
		for (auto& resource : cResources)
		{
			const auto& type = compiler.get_type_from_variable(resource.id);

			ShaderResourceBinding res = {};
			res.Type = ResourceBindingType::Sampler;
			res.Stages = stage;
			res.Name = resource.name;

			// ArraySize
			res.ArraySize = type.array.size() ? type.array[0] : 1;
			res.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			res.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

			*maxSet = res.Set > *maxSet ? res.Set : *maxSet;

			resources[res.Set].emplace(res.Binding, res);
		}
	}

	template <>
	inline void ReadResource<ResourceBindingType::UniformBuffer>(const spirv_cross::Compiler& compiler, ShaderType stage, std::map<uint32_t, std::map<uint32_t, ShaderResourceBinding>>& resources, uint32_t* maxSet) {
		auto& cResources = compiler.get_shader_resources().uniform_buffers;
		for (auto& resource : cResources)
		{
			const auto& type = compiler.get_type_from_variable(resource.id);

			ShaderResourceBinding res = {};
			res.Type = ResourceBindingType::UniformBuffer;
			res.Stages = stage;
			res.Name = resource.name;

			res.Size = static_cast<uint32_t>(compiler.get_declared_struct_size_runtime_array(type, 0));
			// ArraySize
			res.ArraySize = type.array.size() ? type.array[0] : 1;
			res.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			res.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

			*maxSet = res.Set > *maxSet ? res.Set : *maxSet;

			resources[res.Set].emplace(res.Binding, res);
		}
	}

	template <>
	inline void ReadResource<ResourceBindingType::StorageBuffer>(const spirv_cross::Compiler& compiler, ShaderType stage, std::map<uint32_t, std::map<uint32_t, ShaderResourceBinding>>& resources, uint32_t* maxSet) {
		auto& cResources = compiler.get_shader_resources().storage_buffers;
		for (auto& resource : cResources)
		{
			const auto& type = compiler.get_type_from_variable(resource.id);

			ShaderResourceBinding res = {};
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

			resources[res.Set].emplace(res.Binding, res);
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

		hash_combine(m_Hash, m_Desc.Type);
		hash_combine(m_Hash, m_Desc.Name);
		hash_combine(m_Hash, m_Desc.EntryPoint);
		hash_combine(m_Hash, m_Desc.Path);
		hash_combine(m_Hash, m_Desc.Src);
	}

	ShaderVK::~ShaderVK()
	{
		vkDestroyShaderModule(m_Device->Get(), m_Module, nullptr);
	}

	bool ShaderVK::LoadShaderFromFile(const std::string& path)
	{
		std::string cachedPath = path + ".cache";
		std::vector<uint32_t> spirv = {};
		std::ifstream in(cachedPath.c_str(), std::ios::in | std::ios::binary);
		if (in.is_open()) {
			in.seekg(0, std::ios::end);
			auto size = in.tellg();
			in.seekg(0, std::ios::beg);

			spirv.resize(size / sizeof(uint32_t));
			in.read((char*)spirv.data(), size);
		} else {
			// Load shader as string from file
			std::ifstream file(path.c_str());
			if (!file.is_open())
				return false;
			std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

			// Compile the file data into SPIR-V
			spirv = Compile(source);

			std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
			if (out.is_open()) {
				out.write((char*)spirv.data(), spirv.size() * sizeof(uint32_t));
				out.flush();
				out.close();
			}
		}

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
		std::vector<uint32_t> spirv = Compile(src);

		Reflect(spirv);

		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = spirv.size() * sizeof(uint32_t);
		createInfo.pCode = spirv.data();

		if (vkCreateShaderModule(m_Device->Get(), &createInfo, nullptr, &m_Module))
			return false;

		return true;
	}

	VkPipelineShaderStageCreateInfo ShaderVK::GetStage()
	{
		VkPipelineShaderStageCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		if (m_Desc.Type == ShaderType::Fragment)
			info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		else if (m_Desc.Type == ShaderType::Vertex)
			info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		info.module = m_Module;
		info.pName = m_Desc.EntryPoint.c_str();
		return info;
	}

	std::vector<uint32_t> ShaderVK::Compile(std::string code)
	{
		shaderc_shader_kind type = shaderc_vertex_shader;
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
			throw std::runtime_error("Unkown shader stage");
			break;
		}

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);
		options.SetOptimizationLevel(shaderc_optimization_level_performance);
		shaderc::CompilationResult result = compiler.CompileGlslToSpv(code, type, m_Desc.EntryPoint.c_str(), options);
		if (result.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			std::cerr << result.GetErrorMessage() << std::endl;
			return {};
		}
		return std::vector<uint32_t>(result.cbegin(), result.cend());
	}

	void ShaderVK::Reflect(std::vector<uint32_t> spirv)
	{
		// Get shader resources for reflection
		spirv_cross::Compiler comp(spirv);
		uint32_t sets = 0;
		ReadResource<ResourceBindingType::Input>(comp, m_Desc.Type, m_Layout.Inputs, &sets);
		ReadResource<ResourceBindingType::Output>(comp, m_Desc.Type, m_Layout.Outputs, &sets);
		ReadResource<ResourceBindingType::InputAttachment>(comp, m_Desc.Type, m_Layout.Resources, &sets);
		ReadResource<ResourceBindingType::Image>(comp, m_Desc.Type, m_Layout.Resources, &sets);
		ReadResource<ResourceBindingType::ImageSampler>(comp, m_Desc.Type, m_Layout.Resources, &sets);
		ReadResource<ResourceBindingType::ImageStorage>(comp, m_Desc.Type, m_Layout.Resources, &sets);
		ReadResource<ResourceBindingType::Sampler>(comp, m_Desc.Type, m_Layout.Resources, &sets);
		ReadResource<ResourceBindingType::UniformBuffer>(comp, m_Desc.Type, m_Layout.Resources, &sets);
		ReadResource<ResourceBindingType::StorageBuffer>(comp, m_Desc.Type, m_Layout.Resources, &sets);
		
		auto& push_constants = comp.get_shader_resources().push_constant_buffers;

		for (auto &resource : push_constants)
		{
			const auto& type = comp.get_type_from_variable(resource.id);
			uint32_t offset = UINT32_MAX;
			for (auto i = 0; i < type.member_types.size(); i++)
				offset = std::min(offset, comp.get_member_decoration(type.self, i, spv::DecorationOffset));

			ShaderResourceBinding res = {};
			res.Type = ResourceBindingType::PushConstant;
			res.Stages = m_Desc.Type;
			res.Name = resource.name;
			res.Offset = offset;

			res.Size = static_cast<uint32_t>(comp.get_declared_struct_size_runtime_array(type, 0));

			res.Size -= res.Offset;

			m_Layout.Constants.push_back(res);
		}

		auto special_constants = comp.get_specialization_constants();

		for (auto& resource : special_constants)
		{
			auto& value = comp.get_constant(resource.id);
			auto& type = comp.get_type(value.constant_type);

			ShaderResourceBinding res = {};
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

			m_Layout.Constants.push_back(res);
		}
		m_Layout.SetCount = sets;
	}
	
	ShaderProgramVK::ShaderProgramVK(GraphicsDeviceVK* device, ShaderVK* vertShader, ShaderVK* fragShader)
		: ShaderProgram(vertShader, fragShader), m_Device(device)
	{
		m_DescCache = DBG_NEW DescriptorSetLayoutCacheVK(m_Device);
		Build();
	}

	ShaderProgramVK::~ShaderProgramVK()
	{
		for (auto& temp : m_UpdateTemplates)
			vkDestroyDescriptorUpdateTemplate(m_Device->Get(), temp, nullptr);
		delete m_DescCache;
		vkDestroyPipelineLayout(m_Device->Get(), m_PipeLayout, nullptr);
	}

	void ShaderProgramVK::AddShader(Shader* shader)
	{
		m_Shaders[shader->GetDesc().Type] = shader;
	}

	void ShaderProgramVK::Build()
	{
		auto merge = [](std::vector<ShaderResourceBinding>& src, std::vector<ShaderResourceBinding>& dst) {
			for (auto& binding : src) {
				auto& it = std::find(dst.begin(), dst.end(), binding);
				if (it != dst.end())
					it->Stages |= binding.Stages;
				else
					dst.push_back(binding);
			}
		};

		uint32_t maxSets = 1;
		for (auto const& [type, shader] : m_Shaders)
		{
			auto& layout = shader->GetLayout();
			maxSets = std::max(maxSets, layout.SetCount);

			for (auto& [set, bindings] : layout.Resources)
				for (auto& [binding, resource] : bindings)
					if (resource == m_Layout.Resources[set][binding])
						m_Layout.Resources[set][binding].Stages |= resource.Stages;
					else
						m_Layout.Resources[set][binding] = resource;

			merge(layout.Inputs, m_Layout.Inputs);
			merge(layout.Outputs, m_Layout.Outputs);
			merge(layout.Constants, m_Layout.Constants);
		}

		std::vector<std::vector<ShaderResourceBinding>> resourceSets(maxSets);
		m_SetLayouts.resize(maxSets);

		//resourceSets[0].insert(resourceSets[0].end(), m_Layout.Inputs.begin(), m_Layout.Inputs.end());
		//resourceSets[0].insert(resourceSets[0].end(), m_Layout.Outputs.begin(), m_Layout.Outputs.end());
		//resourceSets[0].insert(resourceSets[0].end(), m_Layout.Constants.begin(), m_Layout.Constants.end());

		for (auto& [set, bindings] : m_Layout.Resources)
			for (auto& [binding, resource] : bindings)
				resourceSets[set].push_back(resource);

		for (uint32_t i = 0; i < maxSets; i++)
			m_SetLayouts[i] = m_DescCache->GetLayout(resourceSets[i]);

		std::vector<VkPushConstantRange> ranges;
		for (uint32_t i = 0; i < m_Layout.Constants.size(); i++)
			if (m_Layout.Constants[i].Type == ResourceBindingType::PushConstant)
			{
				VkPushConstantRange range = {};
				range.stageFlags = (uint32_t)m_Layout.Constants[i].Stages;
				range.offset = m_Layout.Constants[i].Offset;
				range.size = m_Layout.Constants[i].Size;
				ranges.push_back(range);
			}

		VkPipelineLayoutCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		createInfo.setLayoutCount = maxSets;
		createInfo.pSetLayouts = m_SetLayouts.data();
		createInfo.pushConstantRangeCount = (uint32_t)ranges.size();
		createInfo.pPushConstantRanges = ranges.data();
		vkCreatePipelineLayout(m_Device->Get(), &createInfo, nullptr, &m_PipeLayout);

		CreateUpdateTemplate();
	}

	size_t ShaderProgramVK::GetHash()
	{
		if (m_Hash == 0) 
			for (auto& it : m_Shaders)
				hash_combine(m_Hash, it.second->GetHash());
		return m_Hash;
	}

	void ShaderProgramVK::CreateUpdateTemplate()
	{
		m_UpdateTemplates.resize(m_Layout.SetCount + 1);
		for (auto& [set, bindings] : m_Layout.Resources) {
			std::vector<VkDescriptorUpdateTemplateEntry> entries = {};

			for (auto& [binding, resource] : bindings) {
				VkDescriptorType type;
				size_t offset = 0;
				switch (resource.Type)
				{
				case ResourceBindingType::ImageSampler:
					type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					offset = offsetof(BindingInfoVK, imageInfo) + sizeof(BindingInfoVK) * binding;
					break;
				case ResourceBindingType::ImageStorage:
					type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
					offset = offsetof(BindingInfoVK, imageInfo) + sizeof(BindingInfoVK) * binding;
					break;
				case ResourceBindingType::UniformBuffer:
					type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					offset = offsetof(BindingInfoVK, bufferInfo) + sizeof(BindingInfoVK) * binding;
					break;
				case ResourceBindingType::StorageBuffer:
					type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					offset = offsetof(BindingInfoVK, bufferInfo) + sizeof(BindingInfoVK) * binding;
					break;
				case ResourceBindingType::InputAttachment:
					type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
					offset = offsetof(BindingInfoVK, imageInfo) + sizeof(BindingInfoVK) * binding;
					break;
				case ResourceBindingType::Sampler:
					type = VK_DESCRIPTOR_TYPE_SAMPLER;
					offset = offsetof(BindingInfoVK, imageInfo) + sizeof(BindingInfoVK) * binding;
					break;
				case ResourceBindingType::Image:
					type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
					offset = offsetof(BindingInfoVK, imageInfo) + sizeof(BindingInfoVK) * binding;
					break;
				default:
					throw std::runtime_error("Unknown Resource Binding Type");
					break;
				}
				VkDescriptorUpdateTemplateEntry entry = {};
				entry.descriptorType = type;
				entry.dstBinding = binding;
				entry.dstArrayElement = 0;
				entry.descriptorCount = resource.ArraySize;
				entry.offset = offset;
				entry.stride = sizeof(BindingInfoVK);

				entries.push_back(entry);
			}

			VkDescriptorUpdateTemplateCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO;
			createInfo.descriptorUpdateEntryCount = static_cast<uint32_t>(entries.size());
			createInfo.pDescriptorUpdateEntries = entries.data();
			createInfo.templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET;
			createInfo.descriptorSetLayout = m_SetLayouts[set];
			createInfo.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			createInfo.pipelineLayout = m_PipeLayout;
			createInfo.set = set;

			vkCreateDescriptorUpdateTemplate(m_Device->Get(), &createInfo, nullptr, &m_UpdateTemplates[set]);\
		}
	}
}
