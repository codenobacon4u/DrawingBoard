#pragma once

#include "Shader.h"

#include <vulkan/vulkan.h>

#include "DescriptorSetVK.h"

namespace Vulkan {
	class GraphicsDeviceVK;
	class ShaderVK : public Shader
	{
	public:
		ShaderVK(GraphicsDeviceVK* device, const ShaderDesc& desc);

		~ShaderVK();

		bool LoadShaderFromFile(const std::string& path);
		bool LoadShaderFromSrc(const std::string& src);

		VkPipelineShaderStageCreateInfo GetStage();

	private:
		std::vector<uint32_t> Compile(std::string code);
		void Reflect(std::vector<uint32_t> spirv);

	private:
		GraphicsDeviceVK* m_Device;
		VkShaderModule m_Module = VK_NULL_HANDLE;
		VkPipelineShaderStageCreateInfo m_Stage = {};
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	};

	class ShaderProgramVK : public ShaderProgram
	{
	public:
		ShaderProgramVK(GraphicsDeviceVK* device, ShaderVK* vertShader, ShaderVK* fragShader);
		
		~ShaderProgramVK();

		virtual void AddShader(Shader* shader) override;
		virtual void Build() override;


		virtual size_t GetHash() override;

		VkPipelineLayout GetPipelineLayout() { return m_PipeLayout; }
		VkDescriptorSetLayout& GetSetLayout(uint32_t set) { return m_SetLayouts[set]; }
		VkDescriptorUpdateTemplate& GetUpdateTemplate(uint32_t set) { return m_UpdateTemplates[set]; }

	private:
		void CreateUpdateTemplate();

	private:
		GraphicsDeviceVK* m_Device;
		ShaderLayout m_Layout;

		VkPipelineLayout m_PipeLayout = VK_NULL_HANDLE;
		DescriptorSetLayoutCacheVK* m_DescCache;
		std::vector<VkDescriptorSetLayout> m_SetLayouts = {};
		std::vector<VkDescriptorUpdateTemplate> m_UpdateTemplates = {};

		size_t m_Hash = 0;
	};
}
