////////////////////////////////////////////////////////////////////////////////////////////////
// object to encapsulate oculus' session object

#ifndef OCULUS_SESSION_H
#define OCULUS_SESSION_H

#include "support/godot_calls.h"

// Include the Oculus SDK
#include "OVR_CAPI_GL.h"

#if defined(_WIN32)
#include <dxgi.h> // for GetDefaultAdapterLuid
#pragma comment(lib, "dxgi.lib")
#endif

#include "texture_buffer.h"

class OculusSession {
private:
	static OculusSession *singleton;

	enum InitStatus {
		OCULUS_NOT_INIT,
		OCULUS_INIT_SUCCESS,
		OCULUS_INIT_FAILED
	};

	InitStatus init_status;

	ovrSession session;
	ovrGraphicsLuid luid;
	ovrHmdDesc hmdDesc;

	ovrEyeRenderDesc eyeRenderDesc[2];
	ovrPosef EyeRenderPose[2];
	ovrPosef HmdToEyePose[2];

	ovrTrackingState trackState;
	ovrInputState inputState;

	uint64_t frameIndex;
	double sensorSampleTime;

	ovrGraphicsLuid get_default_adapter_luid();
	int compare_luid(const ovrGraphicsLuid &lhs, const ovrGraphicsLuid &rhs);

	void transform_from_pose(godot_transform *p_dest, ovrPosef *p_pose, float p_world_scale);
	void transform_from_poses(godot_transform *p_dest, ovrPosef *p_pose_a, ovrPosef *p_pose_b, float p_world_scale);

public:
	static OculusSession *get_singleton();
	static void cleanup_singleton();

	bool is_initialised();

	OculusSession();
	~OculusSession();

	bool initialise();
	void uninitialise();

	// texture chains
	ovrSizei get_texture_size(int p_eye);
	TextureBuffer *make_texture_buffer(int p_eye, bool p_need_fbo = true, int p_type = 0);

	// session state
	void session_status(ovrSessionStatus *p_session_status);
	void recenter_tracking_origin();
	void update_eye_poses();
	void update_states();
	unsigned int get_connected_controller_types();

	// controller info
	bool is_button_pressed(unsigned int p_button);
	bool is_button_touched(unsigned int p_button);
	float hand_trigger(unsigned int p_hand);
	float index_trigger(unsigned int p_hand);
	ovrVector2f thumb_stick(unsigned int p_hand);
	void hand_transform(unsigned int p_hand, godot_transform *p_transform);
	void rumble(enum ovrControllerType_ controller, float value);

	// hmd
	float get_refresh_rate();
	const godot_vector3 *get_play_area();
	void hmd_transform(godot_transform *p_transform, float p_world_scale);
	void eye_transform(int p_eye, godot_transform *p_transform, float p_world_scale);
	void projection_matrix(int p_eye, godot_real *p_projection, float p_z_near, float p_z_far);

	// submit frames
	bool submit_frame(TextureBuffer **p_buffers, int p_num_buffers);
};

#endif /* !OCULUS_SESSION_H */
