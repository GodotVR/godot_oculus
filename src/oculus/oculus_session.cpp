////////////////////////////////////////////////////////////////////////////////////////////////
// object to encapsulate oculus' session object

#include "oculus/oculus_session.h"

OculusSession *OculusSession::singleton = NULL;

OculusSession *OculusSession::get_singleton() {
	if (singleton == NULL) {
		singleton = new OculusSession();
	}
	return singleton;
}

void OculusSession::cleanup_singleton() {
	if (singleton != NULL) {
		delete singleton;
		singleton = NULL;
	}
}

ovrGraphicsLuid OculusSession::get_default_adapter_luid() {
	ovrGraphicsLuid luid = ovrGraphicsLuid();

#if defined(_WIN32)
	IDXGIFactory *factory = nullptr;

	if (SUCCEEDED(CreateDXGIFactory(IID_PPV_ARGS(&factory)))) {
		IDXGIAdapter *adapter = nullptr;

		if (SUCCEEDED(factory->EnumAdapters(0, &adapter))) {
			DXGI_ADAPTER_DESC desc;

			adapter->GetDesc(&desc);
			memcpy(&luid, &desc.AdapterLuid, sizeof(luid));
			adapter->Release();
		}

		factory->Release();
	}
#endif

	return luid;
};

int OculusSession::compare_luid(const ovrGraphicsLuid &lhs,
		const ovrGraphicsLuid &rhs) {
	return memcmp(&lhs, &rhs, sizeof(ovrGraphicsLuid));
}

void OculusSession::transform_from_pose(godot_transform *p_dest, ovrPosef *p_pose,
		float p_world_scale) {
	godot_quat q;
	godot_basis basis;
	godot_vector3 origin;

	api->godot_quat_new(&q, p_pose->Orientation.x, p_pose->Orientation.y,
			p_pose->Orientation.z, p_pose->Orientation.w);
	api->godot_basis_new_with_euler_quat(&basis, &q);

	api->godot_vector3_new(&origin, p_pose->Position.x * p_world_scale,
			p_pose->Position.y * p_world_scale,
			p_pose->Position.z * p_world_scale);
	api->godot_transform_new(p_dest, &basis, &origin);
}

void OculusSession::transform_from_poses(godot_transform *p_dest, ovrPosef *p_pose_a, ovrPosef *p_pose_b, float p_world_scale) {
	godot_quat q;
	godot_basis basis;
	godot_vector3 origin;

	// assume both poses are oriented the same
	api->godot_quat_new(&q, p_pose_a->Orientation.x, p_pose_a->Orientation.y,
			p_pose_a->Orientation.z, p_pose_a->Orientation.w);
	api->godot_basis_new_with_euler_quat(&basis, &q);

	// find center
	api->godot_vector3_new(&origin, 0.5 * (p_pose_a->Position.x + p_pose_b->Position.x) * p_world_scale,
			0.5 * (p_pose_a->Position.y + p_pose_b->Position.y) * p_world_scale,
			0.5 * (p_pose_a->Position.z + p_pose_b->Position.z) * p_world_scale);
	api->godot_transform_new(p_dest, &basis, &origin);
}

bool OculusSession::is_initialised() {
	return init_status == OCULUS_INIT_SUCCESS;
}

OculusSession::OculusSession() {
	init_status = OCULUS_NOT_INIT;
};

OculusSession::~OculusSession() {
	if (is_initialised()) {
		uninitialise();
	}
}

bool OculusSession::initialise() {
	if (init_status == OCULUS_INIT_FAILED) {
		// failed before? don't try again
		return false;
	} else if (init_status == OCULUS_NOT_INIT) {
		ovrResult result = ovr_Create(&session, &luid);
		if (!OVR_SUCCESS(result)) {
			printf("Oculus - Couldn''t initialize Oculus SDK\n");
			init_status = OCULUS_INIT_FAILED;
			return false;
		}

		if (compare_luid(luid, get_default_adapter_luid())) {
			// If luid that the Rift is on is not the default adapter LUID...
			printf("Oculus - Oculus SDK is on a different adapter. If you have "
				   "multiple graphics cards make sure Godot and the Oculus client are "
				   "set to use the same GPU!!\n");

			ovr_Destroy(session);
			init_status = OCULUS_INIT_FAILED;
			return false;
		}

		// get our HMD handle
		hmdDesc = ovr_GetHmdDesc(session);

		// Assuming standing, Godot can handle sitting
		ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel);

		frameIndex = 0;
		init_status = OCULUS_INIT_SUCCESS;
	}

	return true;
}

void OculusSession::uninitialise() {
	if (is_initialised()) {
		ovr_Destroy(session);
		init_status = OCULUS_NOT_INIT;
	}
}

ovrSizei OculusSession::get_texture_size(int p_eye) {
	if (is_initialised()) {
		return ovr_GetFovTextureSize(session, ovrEyeType(p_eye),
				hmdDesc.DefaultEyeFov[p_eye], 1);
	} else {
		ovrSizei size;
		size.w = 0;
		size.h = 0;
		return size;
	}
}

TextureBuffer *OculusSession::make_texture_buffer(int p_eye, bool p_need_fbo, int p_type) {
	if (is_initialised()) {
		ovrSizei idealTextureSize = ovr_GetFovTextureSize(session, ovrEyeType(p_eye), hmdDesc.DefaultEyeFov[p_eye], 1);
		return new TextureBuffer(session, idealTextureSize.w, idealTextureSize.h, p_need_fbo, p_type);
	} else {
		return NULL;
	}
}

void OculusSession::session_status(ovrSessionStatus *p_session_status) {
	if (is_initialised()) {
		ovr_GetSessionStatus(session, p_session_status);
	}
}

void OculusSession::recenter_tracking_origin() {
	if (is_initialised()) {
		ovr_RecenterTrackingOrigin(session);
	}
}

void OculusSession::update_eye_poses() {
	if (is_initialised()) {
		// Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the
		// returned values (e.g. HmdToEyePose) may change at runtime.
		eyeRenderDesc[0] =
				ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
		eyeRenderDesc[1] =
				ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);

		// Get eye poses, feeding in correct IPD offset
		HmdToEyePose[0] = eyeRenderDesc[0].HmdToEyePose;
		HmdToEyePose[1] = eyeRenderDesc[1].HmdToEyePose;

		ovr_GetEyePoses(session, frameIndex, ovrTrue, HmdToEyePose, EyeRenderPose,
				&sensorSampleTime);
	}
}

void OculusSession::update_states() {
	if (is_initialised()) {
		// update our controller state
		double frame_timing = 1.0; // need to do something with this..
		trackState = ovr_GetTrackingState(session, frame_timing, ovrFalse);
		ovr_GetInputState(session, ovrControllerType_Active, &inputState);
	}
}

unsigned int OculusSession::get_connected_controller_types() {
	if (is_initialised()) {
		return ovr_GetConnectedControllerTypes(session);
	} else {
		return 0;
	}
}

bool OculusSession::is_button_pressed(unsigned int p_button) {
	if (is_initialised()) {
		return (inputState.Buttons & p_button);
	} else {
		return false;
	}
}

bool OculusSession::is_button_touched(unsigned int p_button) {
	if (is_initialised()) {
		return (inputState.Touches & p_button);
	} else {
		return false;
	}
}

float OculusSession::hand_trigger(unsigned int p_hand) {
	if (is_initialised()) {
		return inputState.HandTrigger[p_hand];
	} else {
		return 0.0;
	}
}

float OculusSession::index_trigger(unsigned int p_hand) {
	if (is_initialised()) {
		return inputState.IndexTrigger[p_hand];
	} else {
		return 0.0;
	}
}

ovrVector2f OculusSession::thumb_stick(unsigned int p_hand) {
	if (is_initialised()) {
		return inputState.Thumbstick[p_hand];
	} else {
		ovrVector2f v;
		v.x = 0.0;
		v.y = 0.0;
		return v;
	}
}

float OculusSession::get_refresh_rate() {
	if (is_initialised()) {
		return hmdDesc.DisplayRefreshRate;
	} else {
		return 0.0f;
	}
}

const godot_vector3 *OculusSession::get_play_area() {
	static godot_vector3 play_area[4];

	if (is_initialised()) {
		// retrieving just the play area should return 4 points
		ovrVector3f oculus_play_area[4];
		int points = 4;
		ovr_GetBoundaryGeometry(session, ovrBoundary_PlayArea, oculus_play_area, &points);

		for (int i = 0; i < 4; i++) {
			play_area[i].x = oculus_play_area[i].x;
			play_area[i].y = oculus_play_area[i].y;
			play_area[i].z = oculus_play_area[i].z;
		}
	} else {
		for (int i = 0; i < 4; i++) {
			play_area[i].x = 0.0f;
			play_area[i].y = 0.0f;
			play_area[i].z = 0.0f;
		}
	}

	return play_area;
}

void OculusSession::hand_transform(unsigned int p_hand, godot_transform *p_transform) {
	transform_from_pose(p_transform, &trackState.HandPoses[p_hand].ThePose, 1.0);
}

void OculusSession::rumble(enum ovrControllerType_ controller, float rumble) {
	ovr_SetControllerVibration(session, controller, 1.0, rumble);
}

void OculusSession::hmd_transform(godot_transform *p_transform, float p_world_scale) {
	transform_from_poses(p_transform, &EyeRenderPose[0], &EyeRenderPose[1], p_world_scale);
}

void OculusSession::eye_transform(int p_eye, godot_transform *p_transform,
		float p_world_scale) {
	transform_from_pose(p_transform, &EyeRenderPose[p_eye], p_world_scale);
}

void OculusSession::projection_matrix(int p_eye, godot_real *p_projection,
		float p_z_near, float p_z_far) {
	if (is_initialised()) {
		// Based on code from OVR_StereoProjection.cpp
		ovrFovPort tanHalfFov = hmdDesc.DefaultEyeFov[p_eye];

		float projXScale = 2.0f / (tanHalfFov.LeftTan + tanHalfFov.RightTan);
		float projXOffset =
				(tanHalfFov.LeftTan - tanHalfFov.RightTan) * projXScale * 0.5f;
		float projYScale = 2.0f / (tanHalfFov.UpTan + tanHalfFov.DownTan);
		float projYOffset =
				(tanHalfFov.UpTan - tanHalfFov.DownTan) * projYScale * 0.5f;

		// Produces X result, mapping clip edges to [-w,+w]
		p_projection[0] = projXScale;
		p_projection[4] = 0.0f;
		p_projection[8] = -projXOffset;
		p_projection[12] = 0.0f;

		// Produces Y result, mapping clip edges to [-w,+w]
		// Hey - why is that YOffset negated?
		// It's because a projection matrix transforms from world coords with Y=up,
		// whereas this is derived from an NDC scaling, which is Y=down.
		p_projection[1] = 0.0f;
		p_projection[5] = projYScale;
		p_projection[9] = projYOffset;
		p_projection[13] = 0.0f;

		// Produces Z-buffer result - app needs to fill this in with whatever Z
		// range it wants. We'll just use some defaults for now.
		p_projection[2] = 0.0f;
		p_projection[6] = 0.0f;
		p_projection[10] = -(p_z_near + p_z_far) / (p_z_far - p_z_near);
		p_projection[14] = -2.0f * p_z_near * p_z_far / (p_z_far - p_z_near);

		// Produces W result (= Z in)
		p_projection[3] = 0.0f;
		p_projection[7] = 0.0f;
		p_projection[11] = -1.0;
		p_projection[15] = 0.0f;
	}
}

bool OculusSession::submit_frame(TextureBuffer **p_buffers, int p_num_buffers) {
	if (is_initialised()) {
		ovrLayerEyeFov ld;
		ld.Header.Type = ovrLayerType_EyeFov;
		ld.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft; // Because OpenGL.

		for (int eye = 0; eye < p_num_buffers; ++eye) {
			ld.ColorTexture[eye] = p_buffers[eye]->TextureChain;
			ld.Viewport[eye] = p_buffers[eye]->get_viewport();
			ld.Fov[eye] = hmdDesc.DefaultEyeFov[eye];
			ld.RenderPose[eye] = EyeRenderPose[eye];
			ld.SensorSampleTime = sensorSampleTime;
		}

		ovrLayerHeader *layers = &ld.Header;
		ovrResult result =
				ovr_SubmitFrame(session, frameIndex, nullptr, &layers, 1);

		// exit the rendering loop if submit returns an error, will retry on
		// ovrError_DisplayLost
		if (!OVR_SUCCESS(result)) {
			// need to do something here...
			return false;
		}

		frameIndex++;
		return true;
	} else {
		return false;
	}
}
