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
	glGenTextures(1, &fb.colorBuffer[0]);
	glBindTexture(GL_TEXTURE_2D, fb.colorBuffer[0]);
	//hdr here, RGB 16/32 f or something
	glTexStorage2D(GL_TEXTURE_2D, 1, colorFormat, width, height);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fb.colorBuffer[0], 0);

	//depth buffer
	glGenTextures(1, &fb.depthBuffer);
	glBindTexture(GL_TEXTURE_2D, fb.depthBuffer);
	//Create 16 bit depth buffer - must be same width/height of color buffer
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, width, height);
	//Attach to framebuffer (assuming FBO is bound)
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fb.depthBuffer, 0);

	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("Framebuffer incomplete: %d", fboStatus);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return fb;
}

hb::Framebuffer hb::createDepthMap(unsigned int width, unsigned int height)
{
	Framebuffer depthMapFb;
	depthMapFb.width = width;
	depthMapFb.height = height;

	glCreateFramebuffers(1, &depthMapFb.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFb.fbo);

	//depth buffer
	glGenTextures(1, &depthMapFb.depthBuffer);
	glBindTexture(GL_TEXTURE_2D, depthMapFb.depthBuffer);
	//Create 16 bit depth buffer - must be same width/height of color buffer
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, depthMapFb.width, depthMapFb.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapFb.depthBuffer, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	float borderColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("Framebuffer incomplete: %d", fboStatus);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return depthMapFb;
}

hb::Framebuffer hb::createGBuffer(unsigned int width, unsigned int height)
{
	hb::Framebuffer framebuffer;
	framebuffer.width = width;
	framebuffer.height = height;

	glCreateFramebuffers(1, &framebuffer.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);

	int formats[3] = {
		GL_RGB32F, //0 = World Position 
		GL_RGB16F, //1 = World Normal
		GL_RGB16F  //2 = Albedo
	};
	//Create 3 color textures
	for (size_t i = 0; i < 3; i++)
	{
		glGenTextures(1, &framebuffer.colorBuffer[i]);
		glBindTexture(GL_TEXTURE_2D, framebuffer.colorBuffer[i]);
		glTexStorage2D(GL_TEXTURE_2D, 1, formats[i], width, height);
		//Clamp to border so we don't wrap when sampling for post processing
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		//Attach each texture to a different slot.
	//GL_COLOR_ATTACHMENT0 + 1 = GL_COLOR_ATTACHMENT1, etc
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, framebuffer.colorBuffer[i], 0);
	}
	//Explicitly tell OpenGL which color attachments we will draw to
	const GLenum drawBuffers[3] = {
			GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2
	};
	glDrawBuffers(3, drawBuffers);

	//depth buffer
	glGenTextures(1, &framebuffer.depthBuffer);
	glBindTexture(GL_TEXTURE_2D, framebuffer.depthBuffer);
	//Create 16 bit depth buffer - must be same width/height of color buffer
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, framebuffer.width, framebuffer.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, framebuffer.depthBuffer, 0);

	float borderColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("Framebuffer incomplete: %d", fboStatus);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//Clean up global state
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return framebuffer;
}