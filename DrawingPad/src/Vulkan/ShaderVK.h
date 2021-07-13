#pragma once

#include "Shader.h"

#include <vulkan/vulkan.h>
#include <spirv_cross/spirv_cross.hpp>

namespace VkAPI {
	class GraphicsDeviceVK;
	class ShaderVK : public Shader
	{
	public:
		ShaderVK(GraphicsDeviceVK* device, const ShaderDesc& desc);

		VkPipelineShaderStageCreateInfo& GetStage() { return m_Stage; }

		bool LoadShaderFromFile(const std::string& path);
		bool LoadShaderFromSrc(const std::string& src);

	private:
		void Reflect(std::vector<uint32_t> spirv);
		void test(const spirv_cross::SPIRType& type, uint32_t set, uint32_t binding);
		void CreatePipelineLayout();

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
		
		virtual void AddShader(Shader* shader) override;

		VkPipelineLayout GetLayout() { return m_Layout; }

	private:
		void CombineLayouts();
		void CreatePipelineLayout(const CombinedLayout& layout);

	private:
		GraphicsDeviceVK* m_Device;
		ResourceLayout m_Combined;
		VkPipelineLayout m_Layout = VK_NULL_HANDLE;
	};
}