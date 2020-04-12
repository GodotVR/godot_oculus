////////////////////////////////////////////////////////////////////////////////////////////////
// Our main ARVRInterface code for our Oculus GDNative module

#ifndef OVR_XR_INTERFACE_H
#define OVR_XR_INTERFACE_H

#include "support/OS.h"
#include "support/godot_calls.h"
#include <glad/glad.h>

// needed for Godot 3.1 support
#include "support/blit_shader.h"

// Include the Oculus SDK
#include "OVR_CAPI_GL.h"
#include "oculus/oculus_session.h"
#include "oculus/texture_buffer.h"

extern const godot_arvr_interface_gdnative interface_struct;

enum trackers {
	TRACKER_LEFT_TOUCH,
	TRACKER_RIGHT_TOUCH,
	MAX_TRACKERS
};

enum init_state {
	OVR_NOT_CHECKED,
	OVR_NOT_FOUND,
	OVR_NOT_INITIALISED,
	OVR_INITIALISED
};

typedef struct xr_data_struct {
	blit_shader *shader;

	init_state state;
	bool has_external_texture_support;

	OculusSession *session;

	uint32_t width;
	uint32_t height;

	TextureBuffer *eyeRenderTexture[2];

	bool handTriggerPressed[2];
	bool indexTriggerPressed[2];

	godot_real rumble[MAX_TRACKERS];

	uint32_t trackers[MAX_TRACKERS];
} xr_data_struct;

#endif /* !OVR_XR_INTERFACE_H */
