////////////////////////////////////////////////////////////////////////////////////////////////
// Texture buffer object to handle our texture swap chain

#ifndef TEXTURE_BUFFER_H
#define TEXTURE_BUFFER_H

#include <glad/glad.h>

// Include the Oculus SDK
#include "OVR_CAPI_GL.h"

// based on Oculus' example Win32_GLAppUtil.h
class TextureBuffer {
public:
	ovrSession Session;
	ovrTextureSwapChain TextureChain;
	GLuint fboId;
	int width;
	int height;

	TextureBuffer(ovrSession p_session, int p_width, int p_height, bool p_need_fbo = true, int p_type = 0);
	~TextureBuffer();

	ovrRecti get_viewport();
	void set_render_surface();
	void unset_render_surface();
	GLuint get_next_texture();
	void commit();
};

#endif /* !TEXTURE_BUFFER_H */
