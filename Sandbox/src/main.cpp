#include <stdio.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "DrawingPad.h"
#include "OpenGL/OpenGLGraphicsDevice.h"
#include "CommandBuffer.h"


int main() {
	GraphicsDevice* gd;
	CmdList* cq;
	Buffer* vb;
	Buffer* ib;
	//Shader[] shaders;
	Pipeline* pipeline;
	if (!glfwInit())
		printf("Failed");

	GLFWwindow* window = glfwCreateWindow(1920, 1080, "DrawingPad Test", NULL, NULL);
	if (window == NULL) {
		printf("Window is NULL!\n");
		std::abort();
	}
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	auto glfwVersion = glfwGetVersionString();
	auto glVersion = glGetString(GL_VERSION);
	auto glRenderer = glGetString(GL_RENDERER);
	auto glVendor = glGetString(GL_VENDOR);
	gd = GraphicsDevice::Create(window);

	printf("GLFW Version: %s\nGL Version: %s\nGL Renderer: %s\nGL Vendor: %s\n", glfwVersion, glVersion, glRenderer, glVendor);


	static const float data[] = {
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		0.0f,  1.0f, 0.0f,
	};

	static float clearColor[] = {
		1.0f, 1.0f, 1.0f, 1.0f
	};

	while (!glfwWindowShouldClose(window))
	{
		/*
		cq->Begin();

		cq->SetFramebuffer(gd->GetSwapchainFramebuffer());
		cq->ClearColor(clearColor);

		cq->SetVertexBuffer(vb);
		cq->SetIndexBuffer(ib);
		cq->SetPipeline(pipeline);

		cq->DrawIndexed(6);

		cq->End();

		gd->Submit(cq);
		*/
		glfwWaitEvents();
		gd->SwapBuffers();
	}
	glfwDestroyWindow(window);
	glfwTerminate();
}