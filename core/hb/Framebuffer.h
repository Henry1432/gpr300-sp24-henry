#include <stdio.h>
#include <math.h>

#include "../ew/external/glad.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "../ew/shader.h"
#include "../ew/model.h"
#include "../ew/camera.h"
#include "../ew/transform.h"
#include "../ew/cameraController.h"
#include "../ew/texture.h"


namespace hb 
{
	struct Framebuffer 
	{
		unsigned int fbo;
		unsigned int rbo;
		unsigned int colorBuffer[8];
		unsigned int depthBuffer;
		unsigned int width;
		unsigned int height;
	};


	Framebuffer createFramebuffer(unsigned int width, unsigned int height, int colorFormat);
}
