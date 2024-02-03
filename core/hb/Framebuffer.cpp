#include "Framebuffer.h"
#include <stdio.h>
#include <math.h>
#include <GLFW/glfw3.h>
#include "../ew/external/glad.h"



hb::Framebuffer hb::createFramebuffer(unsigned int width, unsigned int height, int colorFormat)
{
	Framebuffer fb;
	fb.width = width;
	fb.height = height;

	glCreateFramebuffers(1, &fb.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);

	//color buffer
	for (int i = 0; i < sizeof(fb.colorBuffer) / sizeof(int); i++)
	{
		glGenTextures(1, fb.colorBuffer);
		glBindTexture(GL_TEXTURE_2D, fb.colorBuffer[i]);
		//hdr here, RGB 16/32 f or something
		glTexStorage2D(GL_TEXTURE_2D, 1, colorFormat, width, height);

		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fb.colorBuffer[i], 0);
	}

	//depth buffer
	glGenTextures(1, &fb.depthBuffer);
	glBindTexture(GL_TEXTURE_2D, fb.depthBuffer);
	//Create 16 bit depth buffer - must be same width/height of color buffer
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, width, height);
	//Attach to framebuffer (assuming FBO is bound)
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fb.depthBuffer, 0);

	glGenRenderbuffers(1, &fb.rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, fb.rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fb.rbo);

	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("Framebuffer incomplete: %d", fboStatus);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return fb;
}