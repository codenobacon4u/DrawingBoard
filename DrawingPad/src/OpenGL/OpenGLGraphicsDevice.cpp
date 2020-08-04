#include "pwpch.h"
#include <glad/glad.h>

#include "OpenGLGraphicsDevice.h"
#include "OpenGLCommandBuffer.h"

OpenGLGraphicsDevice::OpenGLGraphicsDevice(GLFWwindow* window)
{
	OpenGLDeviceInfo info;
	info.SwapBuffers = std::function([=]() { glfwSwapBuffers(window); });

	Init(info);
}

OpenGLGraphicsDevice::OpenGLGraphicsDevice(OpenGLDeviceInfo info)
{
	Init(info);
}

void OpenGLGraphicsDevice::Init(OpenGLDeviceInfo info)
{
	m_SwapBuffers = info.SwapBuffers;
	int major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);

	m_Major = major;
	m_Minor = minor;

	EnumerateExtensions();

	int uboAlignment;
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uboAlignment);
	
	if (m_Supported.StorageBuffers) {
		int ssboAlignment;
		glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &ssboAlignment);
	}
}

void OpenGLGraphicsDevice::SubmitCmdBuffer(OpenGLCmdEntryList* list)
{
	// Should be the same as ManagedCommandEntryList functionalty
	// Iterate through each cammand in the command buffer
	//	 Switch on the command type
	//	   Execute the OpenGL functions
	//	   Executed in CommandBuffer
	//     Default: throw error
	for (auto cmd : list->Get())
	{
		switch (cmd.CmdType)
		{
		case OpenGLCmdEntry::Type::Begin:
		case OpenGLCmdEntry::Type::End:
		{
			break;
		}
		case OpenGLCmdEntry::Type::ClearColor:
		{
			float* color = static_cast<float*>(cmd.CmdData);
			glClearColor(color[0], color[1], color[2], color[3]);
			glClear(GL_COLOR_BUFFER_BIT);
			break;
		}
		case OpenGLCmdEntry::Type::DrawIndexed:
		{
			PreDraw();
			uint32_t indexCount = *static_cast<uint32_t*>(cmd.CmdData);
			glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
			break;
		}
		case OpenGLCmdEntry::Type::SetFramebuffer:
		{
			// Replace with code to check if the given framebuffer
			// target is swapchain or texture
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			break;
		}
		case OpenGLCmdEntry::Type::SetIndexBuffer:
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0/*bufferID*/);
			break;
		}
		case OpenGLCmdEntry::Type::SetVertexBuffer:
		{
			break;
		}
		case OpenGLCmdEntry::Type::SetPipeline:
		{
			break;
		}
		}
	}
}

void OpenGLGraphicsDevice::PreDraw()
{

}

void OpenGLGraphicsDevice::Submit(CmdList* cb)
{
	//Should be the same for Vulkan
	OpenGLCmdList* glcq = static_cast<OpenGLCmdList*>(cb); // CommandList
	OpenGLCmdEntryList* list = glcq->GetCommands(); // CommandEntryList
	
	SubmitCmdBuffer(list);
}

void OpenGLGraphicsDevice::SwapBuffers() const
{
	m_SwapBuffers();
}

bool OpenGLGraphicsDevice::IsExtensionSupported(std::string extension)
{
	return m_Extensions->find(extension) != m_Extensions->end();
}

bool OpenGLGraphicsDevice::GLVersion(int major, int minor)
{
	return m_Major >= major && m_Minor >= minor;
}

void OpenGLGraphicsDevice::EnumerateExtensions()
{
	int extensionCount;
	glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
	m_Extensions = new std::unordered_set<std::string>();
	for (uint32_t i = 0; i < extensionCount; i++) {
		const char* name = (char*)glGetStringi(GL_EXTENSIONS, i);
		m_Extensions->insert(name);
	}
	m_Supported.TextureStorage = IsExtensionSupported("GL_ARB_texture_storage");
	m_Supported.TextureStorageMultisample = IsExtensionSupported("GL_ARB_texture_storage_multisample");
	m_Supported.DirectStateAccess = IsExtensionSupported("GL_ARB_direct_state_access");
	m_Supported.MultiBind = IsExtensionSupported("GL_ARB_multi_bind");
	m_Supported.TextureView = GLVersion(4, 3) || IsExtensionSupported("GL_ARB_texture_view");
	m_Supported.CopyImage = IsExtensionSupported("GL_ARB_copy_image") || IsExtensionSupported("GL_EXT_copy_image");
	m_Supported.DebugOutput = IsExtensionSupported("GL_ARB_debug_output");
	m_Supported.Debug = IsExtensionSupported("GL_KHR_debug");
	m_Supported.ComputeShaders = IsExtensionSupported("GL_ARB_compute_shader");
	m_Supported.ViewportArray = GLVersion(4, 1) || IsExtensionSupported("GL_ARB_viewport_array");
	m_Supported.TessellationShader = GLVersion(4, 0) || IsExtensionSupported("GL_ARB_tessellation_shader");
	m_Supported.GeometryShader = GLVersion(3, 2) || IsExtensionSupported("GL_ARB_geometry_shader4");
	m_Supported.DrawElementsBaseVertex = GLVersion(3, 2) || IsExtensionSupported("GL_ARB_draw_elements_base_vertex");
	m_Supported.IndependentBlend = GLVersion(4, 0);
	m_Supported.DrawIndirect = GLVersion(4, 0) || IsExtensionSupported("GL_ARB_draw_indirect");
	m_Supported.MultiDrawIndirect = GLVersion(4, 3) || IsExtensionSupported("GL_ARB_multi_draw_indirect") || IsExtensionSupported("GL_EXT_multi_draw_indirect");
	m_Supported.StorageBuffers = GLVersion(4, 3) || IsExtensionSupported("GL_ARB_shader_storage_buffer_object");
	m_Supported.ClipControl = GLVersion(4, 5) || IsExtensionSupported("GL_ARB_clip_control");
}
