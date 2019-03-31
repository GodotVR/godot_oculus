////////////////////////////////////////////////////////////////////////////////////////////////
// Texture buffer object to handle our texture swap chain

#include "texture_buffer.h"

TextureBuffer::TextureBuffer(ovrSession p_session, int p_width, int p_height, bool p_need_fbo, int p_type) :
		Session(p_session),
		TextureChain(nullptr),
		fboId(0) {
	width = p_width;
	height = p_height;

	ovrTextureSwapChainDesc desc = {};
	desc.Type = ovrTexture_2D;
	desc.ArraySize = 1;
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	if (p_type == 1) { // GLES2 renderer
		desc.Format = OVR_FORMAT_R8G8B8A8_UNORM;
	} else { // else assume GLES3 renderer which works with SRGB
		desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
	}
	desc.SampleCount = 1;
	desc.StaticImage = ovrFalse;

	ovrResult result = ovr_CreateTextureSwapChainGL(Session, &desc, &TextureChain);

	int length = 0;
	ovr_GetTextureSwapChainLength(Session, TextureChain, &length);

	if (OVR_SUCCESS(result)) {
		for (int i = 0; i < length; ++i) {
			GLuint chainTexId;
			ovr_GetTextureSwapChainBufferGL(Session, TextureChain, i, &chainTexId);
			glBindTexture(GL_TEXTURE_2D, chainTexId);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
	}

	if (p_need_fbo) {
		// In Godot 3.2+ Godot will create the framebuffer
		glGenFramebuffers(1, &fboId);
	}
}

TextureBuffer::~TextureBuffer() {
	if (TextureChain) {
		ovr_DestroyTextureSwapChain(Session, TextureChain);
		TextureChain = nullptr;
	}
	if (fboId) {
		glDeleteFramebuffers(1, &fboId);
		fboId = 0;
	}
}

ovrRecti TextureBuffer::get_viewport() {
	ovrRecti ret;

	ret.Pos.x = 0;
	ret.Pos.y = 0;
	ret.Size.w = width;
	ret.Size.h = height;

	return ret;
}

void TextureBuffer::set_render_surface() {
	GLuint curTexId;

	if (TextureChain) {
		int curIndex;
		ovr_GetTextureSwapChainCurrentIndex(Session, TextureChain, &curIndex);
		ovr_GetTextureSwapChainBufferGL(Session, TextureChain, curIndex, &curTexId);

		glBindFramebuffer(GL_FRAMEBUFFER, fboId);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
				curTexId, 0);

		glViewport(0, 0, width, height);
		//	glClear(GL_COLOR_BUFFER_BIT); // don't waste your time, we're
		//overwriting the entire buffer

		// Enabling SRGB ensures we get a conversion from linear colour space to
		// standard RGB colour space Looks like Godot already renders using standard
		// RGB colour space (or atleast when HDR is used) so lets not do this.
		//	glEnable(GL_FRAMEBUFFER_SRGB);
	}
}

void TextureBuffer::unset_render_surface() {
	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0,
			0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint TextureBuffer::get_next_texture() {
	GLuint curTexId = 0;

	if (TextureChain) {
		int curIndex;
		ovr_GetTextureSwapChainCurrentIndex(Session, TextureChain, &curIndex);
		ovr_GetTextureSwapChainBufferGL(Session, TextureChain, curIndex, &curTexId);
	}
	return curTexId;
}

void TextureBuffer::commit() {
	if (TextureChain) {
		ovr_CommitTextureSwapChain(Session, TextureChain);
	}
}
