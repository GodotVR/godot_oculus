////////////////////////////////////////////////////////////////////////////////////////////////
// Our main ARVRInterface code for our Oculus GDNative module

#ifndef OVR_ARVR_INTERFACE_H
#define OVR_ARVR_INTERFACE_H

#include "GodotCalls.h"
#include "OS.h"
#include <glad/glad.h>

// needed for Godot 3.1 support
#include "blit_shader.h"

// Include the Oculus SDK
#include "OVR_CAPI_GL.h"
#include "ovr_session.h"
#include "texture_buffer.h"

extern const godot_arvr_interface_gdnative interface_struct;

enum trackers { TRACKER_LEFT_TOUCH,
	TRACKER_RIGHT_TOUCH,
	MAX_TRACKERS };

enum init_state {
	OVR_NOT_CHECKED,
	OVR_NOT_FOUND,
	OVR_NOT_INITIALISED,
	OVR_INITIALISED
};

typedef struct arvr_data_struct {
	blit_shader *shader;

	init_state state;
	bool has_external_texture_support;

	OVRSession *session;

	uint32_t width;
	uint32_t height;

	TextureBuffer *eyeRenderTexture[2];

	bool handTriggerPressed[2];
	bool indexTriggerPressed[2];

	uint32_t trackers[MAX_TRACKERS];
} arvr_data_struct;

#endif /* !OVR_ARVR_INTERFACE_H */
