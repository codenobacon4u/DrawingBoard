#include "pwpch.h"

#include "ShaderVK.h"
#include "GraphicsDeviceVK.h"

#include <shaderc/shaderc.hpp>

namespace VkAPI {
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
		//CreatePipelineLayout();
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
	void ShaderVK::Reflect(std::vector<uint32_t> spirv)
	{
		// Get shader resources for reflection
		spirv_cross::Compiler comp(spirv);
		spirv_cross::ShaderResources resources = comp.get_shader_resources();

		for (auto& res : resources.sampled_images)
		{
			uint32_t set = comp.get_decoration(res.id, spv::DecorationDescriptorSet);
			uint32_t binding = comp.get_decoration(res.id, spv::DecorationBinding);
			auto& type = comp.get_type(res.type_id);
			DescriptorBinding descBind = {};
			descBind.Binding = binding;
			descBind.DescriptorType = DescType::SampledImage;
			descBind.DescriptorCount = 1;
			descBind.IsFloat = comp.get_type(type.image.type).basetype == spirv_cross::SPIRType::BaseType::Float;
			if (type.image.dim == spv::DimBuffer)
			{
				m_Layout.Sets[set].SampledBuffers.emplace_back(std::move(descBind));
				m_Layout.Sets[set].SampledBufferCount += 1;
			}
			else
			{
				m_Layout.Sets[set].SampledImages.emplace_back(std::move(descBind));
				m_Layout.Sets[set].SampledImageCount += 1;
			}

			//if (comp.get_type(type.image.type).basetype == spirv_cross::SPIRType::BaseType::Float)
			//	m_Layout.Sets[set].Floats |= 1 << binding;
			test(type, set, binding);

		}

		for (auto& res : resources.subpass_inputs)
		{
			uint32_t set = comp.get_decoration(res.id, spv::DecorationDescriptorSet);
			uint32_t binding = comp.get_decoration(res.id, spv::DecorationBinding);
			auto& type = comp.get_type(res.type_id);
			DescriptorBinding descBind = {};
			descBind.Binding = binding;
			descBind.DescriptorCount = 1;
			descBind.DescriptorType = DescType::InputAttachment;
			descBind.IsFloat = comp.get_type(type.image.type).basetype == spirv_cross::SPIRType::BaseType::Float;
			m_Layout.Sets[set].InputAttachments.emplace_back(std::move(descBind));
			m_Layout.Sets[set].InputAttachmentCount += 1;
			//if (comp.get_type(type.image.type).basetype == spirv_cross::SPIRType::BaseType::Float)
			//	m_Layout.Sets[set].Floats |= 1 << binding;
			test(type, set, binding);
		}

		for (auto& res : resources.separate_images)
		{
			uint32_t set = comp.get_decoration(res.id, spv::DecorationDescriptorSet);
			uint32_t binding = comp.get_decoration(res.id, spv::DecorationBinding);
			auto& type = comp.get_type(res.type_id);
			DescriptorBinding descBind = {};
			descBind.Binding = binding;
			descBind.DescriptorCount = 1;
			descBind.DescriptorType = DescType::SampledImage;
			descBind.IsFloat = comp.get_type(type.image.type).basetype == spirv_cross::SPIRType::BaseType::Float;
			if (type.image.dim == spv::DimBuffer)
			{
				m_Layout.Sets[set].SampledBuffers.emplace_back(std::move(descBind));
				m_Layout.Sets[set].SampledBufferCount += 1;
			}
			else
			{
				m_Layout.Sets[set].SampledImages.emplace_back(std::move(descBind));
				m_Layout.Sets[set].SampledImageCount += 1;
			}

			//if (comp.get_type(type.image.type).basetype == spirv_cross::SPIRType::BaseType::Float)
			//	m_Layout.Sets[set].Floats |= 1 << binding;
			test(type, set, binding);
		}

		for (auto& res : resources.separate_samplers)
		{
			uint32_t set = comp.get_decoration(res.id, spv::DecorationDescriptorSet);
			uint32_t binding = comp.get_decoration(res.id, spv::DecorationBinding);
			auto& type = comp.get_type(res.type_id);
			DescriptorBinding descBind = {};
			descBind.Binding = binding;
			descBind.DescriptorCount = 1;
			descBind.DescriptorType = DescType::Sampler;
			descBind.IsFloat = comp.get_type(type.image.type).basetype == spirv_cross::SPIRType::BaseType::Float;
			m_Layout.Sets[set].Samplers.emplace_back(std::move(descBind));
			m_Layout.Sets[set].SamplerCount += 1;
			test(type, set, binding);
		}

		for (auto& res : resources.storage_images)
		{
			uint32_t set = comp.get_decoration(res.id, spv::DecorationDescriptorSet);
			uint32_t binding = comp.get_decoration(res.id, spv::DecorationBinding);
			auto& type = comp.get_type(res.type_id);

			DescriptorBinding descBind = {};
			descBind.Binding = binding;
			descBind.DescriptorCount = 1;
			descBind.DescriptorType = DescType::StorageImage;
			descBind.IsFloat = comp.get_type(type.image.type).basetype == spirv_cross::SPIRType::BaseType::Float;
			m_Layout.Sets[set].StorageImages.emplace_back(std::move(descBind));
			m_Layout.Sets[set].StorageImageCount += 1;
			//if (comp.get_type(type.image.type).basetype == spirv_cross::SPIRType::BaseType::Float)
			//	m_Layout.Sets[set].Floats |= 1 << binding;
			test(type, set, binding);
		}

		for (auto& res : resources.uniform_buffers)
		{
			uint32_t set = comp.get_decoration(res.id, spv::DecorationDescriptorSet);
			uint32_t binding = comp.get_decoration(res.id, spv::DecorationBinding);
			auto& type = comp.get_type(res.type_id);
			DescriptorBinding descBind = {};
			descBind.Binding = binding;
			descBind.DescriptorCount = 1;
			descBind.DescriptorType = DescType::UniformBuffer;
			descBind.IsFloat = true;//comp.get_type(type.image.type).basetype == spirv_cross::SPIRType::BaseType::Float;
			m_Layout.Sets[set].UniformBuffers.emplace_back(std::move(descBind));
			m_Layout.Sets[set].UniformBufferCount += 1;
			test(type, set, binding);
		}

		for (auto& res : resources.storage_buffers)
		{
			uint32_t set = comp.get_decoration(res.id, spv::DecorationDescriptorSet);
			uint32_t binding = comp.get_decoration(res.id, spv::DecorationBinding);
			auto& type = comp.get_type(res.type_id);
			DescriptorBinding descBind = {};
			descBind.Binding = binding;
			descBind.DescriptorCount = 1;
			descBind.DescriptorType = DescType::StorageBuffer;
			descBind.IsFloat = comp.get_type(type.image.type).basetype == spirv_cross::SPIRType::BaseType::Float;
			m_Layout.Sets[set].StorageBuffers.emplace_back(std::move(descBind));
			m_Layout.Sets[set].StorageBufferCount += 1;
			test(type, set, binding);
		}

		for (auto& res : resources.stage_inputs)
		{
			uint32_t location = comp.get_decoration(res.id, spv::DecorationLocation);
			m_Layout.Inputs |= 1 << location;
		}

		for (auto& res : resources.stage_outputs)
		{
			uint32_t location = comp.get_decoration(res.id, spv::DecorationLocation);
			m_Layout.Outputs |= 1 << location;
		}

		if (!resources.push_constant_buffers.empty())
		{
			m_Layout.PushConstantSize = static_cast<uint32_t>(comp.get_declared_struct_size(comp.get_type(resources.push_constant_buffers.front().base_type_id)));
		}

		for (auto& res : comp.get_specialization_constants())
			m_Layout.SpecConstants |= 1 << res.constant_id;
	}

	void ShaderVK::test(const spirv_cross::SPIRType& type, uint32_t set, uint32_t binding)
	{
		uint32_t size;
		if (!type.array.empty())
		{
			if (type.array.size() != 1)
				std::cerr << "Array dim must be 1" << std::endl;
			else if (!type.array_size_literal.front())
				std::cerr << "Array dim must be literal" << std::endl;
			else
			{
				if (type.array.front() == 0)
				{
					if (binding != 0)
						std::cerr << "Bindless textures can only be used with binding = 0 in a set" << std::endl;
					if (type.basetype != spirv_cross::SPIRType::Image || type.image.dim == spv::DimBuffer)
						std::cerr << "Can only use bindless for sampled images" << std::endl;
					else
						m_Layout.BindlessSets |= 1 << set;
					size = 0xff;
				}
				else
					size = type.array.front();
			}
		}
		else
		{
			size = 1;
		}
	}

	void ShaderVK::CreatePipelineLayout()
	{
		uint32_t setCount = 0;
		std::vector<VkDescriptorSetLayout> layouts;
		for (uint32_t i = 0; i < 16; i++)
		{
			VkDescriptorSetLayoutCreateInfo info = {};
			VkDescriptorSetLayoutBinding binding;
			//binding.
		}

		VkPipelineLayoutCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		createInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
		createInfo.pSetLayouts = layouts.data();

		vkCreatePipelineLayout(m_Device->Get(), &createInfo, nullptr, &m_PipelineLayout);
	}
	
	ShaderProgramVK::ShaderProgramVK(GraphicsDeviceVK* device, ShaderVK* vertShader, ShaderVK* fragShader)
		: ShaderProgram(vertShader, fragShader), m_Device(device)
	{
		CombineLayouts();
	}

	void ShaderProgramVK::AddShader(Shader* shader)
	{
		m_Shaders[shader->GetDesc().Type] = shader;
	}

	void ShaderProgramVK::CombineLayouts()
	{
		CombinedLayout combined = {};
		if (m_Shaders[ShaderType::Vertex])
			combined.Inputs = m_Shaders[ShaderType::Vertex]->GetLayout().Inputs;
		if (m_Shaders[ShaderType::Fragment])
			combined.Outputs = m_Shaders[ShaderType::Fragment]->GetLayout().Outputs;

		for (auto const& [type, shader] : m_Shaders)
		{
			uint32_t stageMask = 1 << static_cast<uint32_t>(type);
			auto layout = shader->GetLayout();
			for (uint32_t set = 0; set < 16; set++)
			{
				combined.Sets[set].SampledImages.insert(std::end(combined.Sets[set].SampledImages), std::begin(layout.Sets[set].SampledImages), std::end(layout.Sets[set].SampledImages));
				combined.Sets[set].StorageImages.insert(std::end(combined.Sets[set].StorageImages), std::begin(layout.Sets[set].StorageImages), std::end(layout.Sets[set].StorageImages));
				combined.Sets[set].UniformBuffers.insert(std::end(combined.Sets[set].UniformBuffers), std::begin(layout.Sets[set].UniformBuffers), std::end(layout.Sets[set].UniformBuffers));
				combined.Sets[set].StorageBuffers.insert(std::end(combined.Sets[set].StorageBuffers), std::begin(layout.Sets[set].StorageBuffers), std::end(layout.Sets[set].StorageBuffers));
				combined.Sets[set].SampledBuffers.insert(std::end(combined.Sets[set].SampledBuffers), std::begin(layout.Sets[set].SampledBuffers), std::end(layout.Sets[set].SampledBuffers));
				combined.Sets[set].InputAttachments.insert(std::end(combined.Sets[set].InputAttachments), std::begin(layout.Sets[set].InputAttachments), std::end(layout.Sets[set].InputAttachments));
				combined.Sets[set].Samplers.insert(std::end(combined.Sets[set].Samplers), std::begin(layout.Sets[set].Samplers), std::end(layout.Sets[set].Samplers));
				combined.Sets[set].SeparateImages.insert(std::end(combined.Sets[set].SeparateImages), std::begin(layout.Sets[set].SeparateImages), std::begin(layout.Sets[set].SeparateImages));

				uint32_t active = !layout.Sets[set].SampledImages.empty() | !layout.Sets[set].StorageImages.empty() |
					!layout.Sets[set].UniformBuffers.empty() | !layout.Sets[set].StorageBuffers.empty() |
					!layout.Sets[set].SampledBuffers.empty() | !layout.Sets[set].InputAttachments.empty() |
					!layout.Sets[set].Samplers.empty() | !layout.Sets[set].SeparateImages.empty();

				if (active)
					combined.SetStages[set] |= stageMask;

				for (uint32_t bit = 0; bit < 32; bit++)
					combined.BindingStages[set][bit] |= stageMask;
			}

			if (layout.PushConstantSize != 0)
			{
			}
			combined.SpecConstants[static_cast<uint32_t>(type)] = layout.SpecConstants;
			combined.CombinedSpecConstants |= layout.SpecConstants;
			combined.BindlessSets |= layout.BindlessSets;
		}

		for (uint32_t set = 0; set < 16; set++)
		{
			if (combined.SetStages[set] != 0)
				combined.DescriptorSet |= 1 << set;
		}

		CreatePipelineLayout(combined);
	}

	void ShaderProgramVK::CreatePipelineLayout(const CombinedLayout& layout)
	{
		VkDescriptorSetLayout layouts[16];
		uint32_t sets = 0;
		for (uint32_t i = 0; i < 16; i++)
		{
			layouts[i] = m_Device->GetDescriptorSetPool().GetLayout(layout.Sets[i], layout.BindingStages[i]);
			if (layout.DescriptorSet & (1 << i))
				sets = i + 1;
		}
		VkPipelineLayoutCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		if (sets)
		{
			createInfo.setLayoutCount = sets;
			createInfo.pSetLayouts = layouts;
		}
		vkCreatePipelineLayout(m_Device->Get(), &createInfo, nullptr, &m_Layout);
	}
}