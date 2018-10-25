////////////////////////////////////////////////////////////////////////////////////////////////
// Our main ARVRInterface code for our Oculus GDNative module

#ifndef OVR_ARVR_INTERFACE_H
#define OVR_ARVR_INTERFACE_H

#include "GodotCalls.h"
#include "OS.h"
#include "blit_shader.h"

// Include the Oculus SDK
#include "OVR_CAPI_GL.h"

#if defined(_WIN32)
	#include <dxgi.h> // for GetDefaultAdapterLuid
	#pragma comment(lib, "dxgi.lib")
#endif

// based on Oculus' example Win32_GLAppUtil.h 
class TextureBuffer {
public:
	ovrSession Session;
	ovrTextureSwapChain TextureChain;
	GLuint texId;
	GLuint fboId;
	int width;
	int height;

	TextureBuffer(ovrSession p_session, int p_width, int p_height);
	~TextureBuffer();

	ovrRecti GetViewport();
	void SetRenderSurface();
	void UnsetRenderSurface();
	void Commit();
};

extern const godot_arvr_interface_gdnative interface_struct;

enum trackers {
	TRACKER_LEFT_TOUCH,
	TRACKER_RIGHT_TOUCH,
	MAX_TRACKERS
};

typedef struct arvr_data_struct {
	blit_shader * shader;

	bool oculus_is_initialized;
	ovrSession session;
	ovrGraphicsLuid luid;
	ovrHmdDesc hmdDesc;
	ovrEyeRenderDesc eyeRenderDesc[2];
	ovrPosef EyeRenderPose[2];
	ovrPosef HmdToEyePose[2];
	uint64_t frameIndex;
	double sensorSampleTime;

	uint32_t width;
	uint32_t height;

	TextureBuffer * eyeRenderTexture[2];

	ovrTrackingState trackState;
	ovrInputState inputState;

	bool handTriggerPressed[2];
	bool indexTriggerPressed[2];

	uint32_t trackers[MAX_TRACKERS];
} arvr_data_struct;

#endif /* !OVR_ARVR_INTERFACE_H */
