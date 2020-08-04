#pragma once
#include <GLFW/glfw3.h>
#include <unordered_set>
#include <functional>

#include "GraphicsDevice.h"
#include "SafeQueue.h"
#include "OpenGLCommandBuffer.h"

struct SupportedExtensions {
	bool TextureStorage;
	bool TextureStorageMultisample;
	bool DirectStateAccess;
	bool MultiBind;
	bool TextureView;
	bool CopyImage;
	bool DebugOutput;
	bool Debug;
	bool ComputeShaders;
	bool ViewportArray;
	bool TessellationShader;
	bool GeometryShader;
	bool DrawElementsBaseVertex;
	bool IndependentBlend;
	bool DrawIndirect;
	bool MultiDrawIndirect;
	bool StorageBuffers;
	bool ClipControl;
};

struct OpenGLDeviceInfo {
	std::function<void()> SwapBuffers;
};

class OpenGLGraphicsDevice : public GraphicsDevice {
public:
	OpenGLGraphicsDevice(GLFWwindow* window);
	OpenGLGraphicsDevice(OpenGLDeviceInfo info);

	virtual void Submit(CmdList* cb) override;
	virtual void SwapBuffers() const override;

	const SupportedExtensions GetExtensions() const { return m_Supported; }

private:
	void Init(OpenGLDeviceInfo info);
	void SubmitCmdBuffer(OpenGLCmdEntryList* list);
	void PreDraw();

	void EnumerateExtensions();
	bool IsExtensionSupported(std::string extension);
	bool GLVersion(int major, int minor);

private:
	OpenGLDeviceInfo m_Info;
	std::unordered_set<std::string>* m_Extensions;
	uint32_t m_Major, m_Minor;
	SupportedExtensions m_Supported;
	std::function<void()> m_SwapBuffers;
};