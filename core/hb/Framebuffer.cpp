#include "Framebuffer.h"
#include <stdio.h>
#include <math.h>
#include <GLFW/glfw3.h>
#include "../ew/external/glad.h"



hb::Framebuffer hb::createFramebuffer(unsigned int width, unsigned int height, int colorFormat)
{
	Framebuffer fb;
	glCreateFramebuffers(1, &fb.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);

	//color buffer
	glGenTextures(1, fb.colorBuffer);
	glBindTexture(GL_TEXTURE_2D, (GLuint) fb.colorBuffer);
	//hdr here, RGB 16/32 f or something
	glTexStorage2D(GL_TEXTURE_2D, 1, colorFormat, width, height)

		//unbind textures at end of function, same with frame buffer
}