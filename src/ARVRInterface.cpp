////////////////////////////////////////////////////////////////////////////////////////////////
// Our main ARVRInterface code for our Oculus GDNative module

// Note, even though this is pure C code, we're using the C++ compiler as
// Microsoft never updated their C compiler to understand more modern dialects
// and openvr uses pesky things such as namespaces

#include "ARVRInterface.h"

///////////////////////////////////////////////////////////////////////////////////////////
// first some stuff from the oculus SDK samples... :)
static ovrGraphicsLuid GetDefaultAdapterLuid() {
	ovrGraphicsLuid luid = ovrGraphicsLuid();

	#if defined(_WIN32)
		IDXGIFactory* factory = nullptr;

		if (SUCCEEDED(CreateDXGIFactory(IID_PPV_ARGS(&factory)))) {
			IDXGIAdapter* adapter = nullptr;

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

static int Compare(const ovrGraphicsLuid& lhs, const ovrGraphicsLuid& rhs) {
	return memcmp(&lhs, &rhs, sizeof(ovrGraphicsLuid));
}

TextureBuffer::TextureBuffer(ovrSession p_session, int p_width, int p_height) :
	Session(p_session),
	TextureChain(nullptr),
	texId(0),
	fboId(0)
{
	width = p_width;
	height = p_height;

	ovrTextureSwapChainDesc desc = {};
	desc.Type = ovrTexture_2D;
	desc.ArraySize = 1;
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.SampleCount = 1;
	desc.StaticImage = ovrFalse;

	ovrResult result = ovr_CreateTextureSwapChainGL(Session, &desc, &TextureChain);

	int length = 0;
	ovr_GetTextureSwapChainLength(Session, TextureChain, &length);

	if(OVR_SUCCESS(result)) {
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

	glGenFramebuffers(1, &fboId);
}

TextureBuffer::~TextureBuffer() {
	if (TextureChain) {
		ovr_DestroyTextureSwapChain(Session, TextureChain);
		TextureChain = nullptr;
	}
	if (texId) {
		glDeleteTextures(1, &texId);
		texId = 0;
	}
	if (fboId) {
		glDeleteFramebuffers(1, &fboId);
		fboId = 0;
	}
}

ovrRecti TextureBuffer::GetViewport() {
	ovrRecti ret;

	ret.Pos.x = 0;
	ret.Pos.y = 0;
	ret.Size.w = width;
	ret.Size.h = height;

	return ret;
}

void TextureBuffer::SetRenderSurface() {
	GLuint curTexId;

	if (TextureChain) {
		int curIndex;
		ovr_GetTextureSwapChainCurrentIndex(Session, TextureChain, &curIndex);
		ovr_GetTextureSwapChainBufferGL(Session, TextureChain, curIndex, &curTexId);
	} else {
		curTexId = texId;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curTexId, 0);

	glViewport(0, 0, width, height);
//	glClear(GL_COLOR_BUFFER_BIT); // don't waste your time, we're overwriting the entire buffer

	// Enabling SRGB ensures we get a conversion from linear colour space to standard RGB colour space
	// Looks like Godot already renders using standard RGB colour space (or atleast when HDR is used) so lets not do this.
//	glEnable(GL_FRAMEBUFFER_SRGB);
}

void TextureBuffer::UnsetRenderSurface() {
	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void TextureBuffer::Commit() {
	if (TextureChain) {
		ovr_CommitTextureSwapChain(Session, TextureChain);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
// our ARVR interface
godot_string godot_arvr_get_name(const void *p_data) {
	godot_string ret;

	char name[] = "Oculus";
	api->godot_string_new(&ret);
	api->godot_string_parse_utf8(&ret, name);

	return ret;
}

godot_int godot_arvr_get_capabilities(const void *p_data) {
	godot_int ret;

	ret = 2 + 8; // 2 = ARVR_STEREO, 8 = ARVR_EXTERNAL

	return ret;
}

godot_bool godot_arvr_get_anchor_detection_is_enabled(const void *p_data) {
	godot_bool ret;

	ret = false; // does not apply here

	return ret;
}

void godot_arvr_set_anchor_detection_is_enabled(void *p_data,
		bool p_enable){
	// we ignore this, not supported in this interface!
}

godot_bool godot_arvr_is_stereo(const void *p_data) {
	godot_bool ret;

	ret = true;

	return ret;
}

godot_bool godot_arvr_is_initialized(const void *p_data) {
	godot_bool ret;
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	ret = arvr_data->oculus_is_initialized;

	return ret;
}

godot_bool godot_arvr_initialize(void *p_data) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	if (!arvr_data->oculus_is_initialized) {
		// initialise this interface, so initialize any 3rd party libraries, open up
		// HMD window if required, etc.
		printf("Oculus - initializing...\n");

		ovrResult result = ovr_Create(&arvr_data->session, &arvr_data->luid);
		if (!OVR_SUCCESS(result)) {
			printf("Oculus - Couldn''t initialize Oculus SDK\n");
			return false;
		}

		if (Compare(arvr_data->luid, GetDefaultAdapterLuid())) {
			// If luid that the Rift is on is not the default adapter LUID...
			printf("Oculus - Oculus SDK is on a different adapter. If you have multiple graphics cards make sure Godot and the Oculus client are set to use the same GPU!!\n");
			return false;
		}

		arvr_data->hmdDesc = ovr_GetHmdDesc(arvr_data->session);

		// Make eye render buffers
		bool success = true;
		for (int eye = 0; eye < 2 && success; ++eye) {
			ovrSizei idealTextureSize = ovr_GetFovTextureSize(arvr_data->session, ovrEyeType(eye), arvr_data->hmdDesc.DefaultEyeFov[eye], 1);
			arvr_data->eyeRenderTexture[eye] = new TextureBuffer(arvr_data->session, idealTextureSize.w, idealTextureSize.h);

			if (!arvr_data->eyeRenderTexture[eye]->TextureChain) {
				printf("Oculus - couldn''t create render texture for eye %i\n", eye+1);
				success = false;
			} else {
				// Could these textures possibly have different sizes?!?! We assume not or else we use the size of our right eye...
				arvr_data->width = idealTextureSize.w;
				arvr_data->height = idealTextureSize.h;

				printf("Oculus - created buffer for eye %i (%i,%i)\n", eye+1, arvr_data->width, arvr_data->height);
			}
		}

		if (success) {
			// Assuming standing, Godot can handle sitting
			ovr_SetTrackingOriginType(arvr_data->session, ovrTrackingOrigin_FloorLevel);

			arvr_data->frameIndex = 0;
			arvr_data->oculus_is_initialized = true;

			printf("Oculus - successfully initialized\n");
		} else {
			// cleanup...
			for (int eye = 0; eye < 2; ++eye) {
				if (arvr_data->eyeRenderTexture[eye] != NULL) {
					delete arvr_data->eyeRenderTexture[eye];
					arvr_data->eyeRenderTexture[eye] = NULL;
				}
			}
			ovr_Destroy(arvr_data->session);
		}
	}

	// and return our result
	return arvr_data->oculus_is_initialized;
}

void godot_arvr_uninitialize(void *p_data) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	if (arvr_data->oculus_is_initialized != NULL) {
		// note, this will already be removed as the primary interface by
		// ARVRInterfaceGDNative

		for (int tracker = 0; tracker < MAX_TRACKERS; tracker++) {
			if (arvr_data->trackers[tracker] != 0) {
				// if we previously had our left touch controller, clean up
				arvr_api->godot_arvr_remove_controller(arvr_data->trackers[tracker]);
				arvr_data->trackers[tracker] = 0;
			}			
		}

		for (int eye = 0; eye < 2; ++eye) {
			if (arvr_data->eyeRenderTexture[eye] != NULL) {
				delete arvr_data->eyeRenderTexture[eye];
				arvr_data->eyeRenderTexture[eye] = NULL;
			}
		}
		ovr_Destroy(arvr_data->session);

		arvr_data->oculus_is_initialized = false;
	};
};

godot_vector2 godot_arvr_get_render_targetsize(const void *p_data) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;
	godot_vector2 size;

	if (arvr_data->oculus_is_initialized) {
		api->godot_vector2_new(&size, arvr_data->width, arvr_data->height);
	} else {
		api->godot_vector2_new(&size, 500, 500);
	}

	return size;
};

void oculus_transform_from_pose(godot_transform *p_dest, ovrPosef *p_pose , float p_world_scale) {
	godot_quat q;
	godot_basis basis;
	godot_vector3 origin;

	api->godot_quat_new(&q, p_pose->Orientation.x, p_pose->Orientation.y, p_pose->Orientation.z, p_pose->Orientation.w);
	api->godot_basis_new_with_euler_quat(&basis, &q);

	api->godot_vector3_new(&origin, p_pose->Position.x * p_world_scale, p_pose->Position.y * p_world_scale, p_pose->Position.z * p_world_scale);
	api->godot_transform_new(p_dest, &basis, &origin);
}

void oculus_transform_from_poses(godot_transform *p_dest, ovrPosef *p_pose_a, ovrPosef *p_pose_b , float p_world_scale) {
	godot_quat q;
	godot_basis basis;
	godot_vector3 origin;

	// assume both poses are oriented the same
	api->godot_quat_new(&q, p_pose_a->Orientation.x, p_pose_a->Orientation.y, p_pose_a->Orientation.z, p_pose_a->Orientation.w);
	api->godot_basis_new_with_euler_quat(&basis, &q);

	// find center
	api->godot_vector3_new(&origin
		, 0.5 * (p_pose_a->Position.x + p_pose_b->Position.x) * p_world_scale
		, 0.5 * (p_pose_a->Position.y + p_pose_b->Position.y) * p_world_scale
		, 0.5 * (p_pose_a->Position.z + p_pose_b->Position.z) * p_world_scale);
	api->godot_transform_new(p_dest, &basis, &origin);
}

godot_transform godot_arvr_get_transform_for_eye(void *p_data, godot_int p_eye, godot_transform *p_cam_transform) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;
	godot_transform transform_for_eye;
	godot_transform reference_frame = arvr_api->godot_arvr_get_reference_frame();
	godot_transform ret;
	godot_vector3 offset;
	godot_real world_scale = arvr_api->godot_arvr_get_worldscale();

	if (p_eye == 0) {
		// ok, we actually get our left and right eye position from tracking data, not our head center with eye offsets
		// so lets find the center :)
		oculus_transform_from_poses(&transform_for_eye, &arvr_data->EyeRenderPose[0], &arvr_data->EyeRenderPose[1], world_scale);		
	} else if (arvr_data->oculus_is_initialized) {
		oculus_transform_from_pose(&transform_for_eye, &arvr_data->EyeRenderPose[p_eye == 2 ? 1 : 0], world_scale);
	} else {
		// really not needed, just being paranoid..
		godot_vector3 offset;
		api->godot_transform_new_identity(&transform_for_eye);
		if (p_eye == 1) {
			api->godot_vector3_new(&offset, -0.035 * world_scale, 0.0, 0.0);
		} else {
			api->godot_vector3_new(&offset, 0.035 * world_scale, 0.0, 0.0);
		};
		api->godot_transform_translated(&transform_for_eye, &offset);
	};

	// Now construct our full transform, the order may be in reverse, have to test
	// :)
	ret = *p_cam_transform;
	ret = api->godot_transform_operator_multiply(&ret, &reference_frame);
//	ret = api->godot_transform_operator_multiply(&ret, &arvr_data->hmd_transform);
	ret = api->godot_transform_operator_multiply(&ret, &transform_for_eye);

	return ret;
};

void godot_arvr_fill_projection_for_eye(void *p_data, godot_real *p_projection, godot_int p_eye, godot_real p_aspect, godot_real p_z_near, godot_real p_z_far) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	if (arvr_data->oculus_is_initialized) {
		// Based on code from OVR_StereoProjection.cpp
		ovrFovPort tanHalfFov = arvr_data->hmdDesc.DefaultEyeFov[p_eye == 2 ? 1 : 0];

		float projXScale = 2.0f / (tanHalfFov.LeftTan + tanHalfFov.RightTan);
		float projXOffset = (tanHalfFov.LeftTan - tanHalfFov.RightTan) * projXScale * 0.5f;
		float projYScale = 2.0f / (tanHalfFov.UpTan + tanHalfFov.DownTan);
		float projYOffset = (tanHalfFov.UpTan - tanHalfFov.DownTan) * projYScale * 0.5f;

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

		// Produces Z-buffer result - app needs to fill this in with whatever Z range it wants.
		// We'll just use some defaults for now.
		p_projection[2] = 0.0f;
		p_projection[6] = 0.0f;
		p_projection[10] = -(p_z_near + p_z_far) / (p_z_far - p_z_near);
		p_projection[14] = -2.0f * p_z_near * p_z_far / (p_z_far - p_z_near);

		// Produces W result (= Z in)
		p_projection[3] = 0.0f;
		p_projection[7] = 0.0f;
		p_projection[11] = -1.0;
		p_projection[15] = 0.0f;
	} else {
		// uhm, should do something here really..
	};
};

void godot_arvr_commit_for_eye(void *p_data, godot_int p_eye, godot_rid *p_render_target, godot_rect2 *p_screen_rect) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	// This function is responsible for outputting the final render buffer for
	// each eye.
	// p_screen_rect will only have a value when we're outputting to the main
	// viewport.

	// For an interface that must output to the main viewport (such as with mobile
	// VR) we should give an error when p_screen_rect is not set
	// For an interface that outputs to an external device we should render a copy
	// of one of the eyes to the main viewport if p_screen_rect is set, and only
	// output to the external device if not.

	godot_rect2 screen_rect = *p_screen_rect;
	godot_vector2 render_size = godot_arvr_get_render_targetsize(p_data);

	if (p_eye == 1 && !api->godot_rect2_has_no_area(&screen_rect)) {
		// blit as mono, attempt to keep our aspect ratio and center our render buffer
		float new_height = screen_rect.size.x * (render_size.y / render_size.x);
		if (new_height > screen_rect.size.y) {
			screen_rect.position.y = (0.5 * screen_rect.size.y) - (0.5 * new_height);
			screen_rect.size.y = new_height;
		} else {
			float new_width = screen_rect.size.y * (render_size.x / render_size.y);

			screen_rect.position.x = (0.5 * screen_rect.size.x) - (0.5 * new_width);
			screen_rect.size.x = new_width;
		};

		// printf("Blit: %0.2f, %0.2f - %0.2f, %0.2f\n",screen_rect.position.x,screen_rect.position.y,screen_rect.size.x,screen_rect.size.y);

		arvr_api->godot_arvr_blit(0, p_render_target, &screen_rect);
	};

	if (arvr_data->oculus_is_initialized) {
		uint32_t texid = arvr_api->godot_arvr_get_texid(p_render_target);
		int eye = p_eye == 2 ? 1 : 0;

		// blit to OVRs buffers			
		// Switch to eye render target
		arvr_data->eyeRenderTexture[eye]->SetRenderSurface();

		// copy our buffer...Unfortunately, at this time Godot can't render directly into Oculus'
		// buffers. Something to discuss with Juan some day but I think this is posing serious 
		// problem with the way our forward renderer handles several effects...
		// Worth further investigation though as this is wasteful...
		blit_render(&arvr_data->shader, texid);

		// Avoids an error when calling SetAndClearRenderSurface during next iteration.
		// Without this, during the next while loop iteration SetAndClearRenderSurface
		// would bind a framebuffer with an invalid COLOR_ATTACHMENT0 because the texture ID
		// associated with COLOR_ATTACHMENT0 had been unlocked by calling wglDXUnlockObjectsNV.
		arvr_data->eyeRenderTexture[eye]->UnsetRenderSurface();

		// Commit changes to the textures so they get picked up frame
		arvr_data->eyeRenderTexture[eye]->Commit();

		if (p_eye == 2) {
			// both eyes are rendered, time to output...

			ovrLayerEyeFov ld;
			ld.Header.Type  = ovrLayerType_EyeFov;
			ld.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;   // Because OpenGL.

			for (int eye = 0; eye < 2; ++eye) {
				ld.ColorTexture[eye] = arvr_data->eyeRenderTexture[eye]->TextureChain;
				ld.Viewport[eye]     = arvr_data->eyeRenderTexture[eye]->GetViewport();
				ld.Fov[eye]          = arvr_data->hmdDesc.DefaultEyeFov[eye];
				ld.RenderPose[eye]   = arvr_data->EyeRenderPose[eye];
				ld.SensorSampleTime  = arvr_data->sensorSampleTime;
			}

			ovrLayerHeader* layers = &ld.Header;
			ovrResult result = ovr_SubmitFrame(arvr_data->session, arvr_data->frameIndex, nullptr, &layers, 1);

			// exit the rendering loop if submit returns an error, will retry on ovrError_DisplayLost
			if (!OVR_SUCCESS(result)) {
				// need to do something here...
			}

			arvr_data->frameIndex++;
		}
	}
}

void oculus_update_touch_controller(arvr_data_struct *p_arvr_data, int p_which) {
	int hand = p_which == TRACKER_LEFT_TOUCH ? 0 : 1;
	if (p_arvr_data->trackers[p_which] == 0) {
		// need to init a new tracker
		if (p_which == TRACKER_LEFT_TOUCH) {
			p_arvr_data->trackers[p_which] = arvr_api->godot_arvr_add_controller("Left Oculus Touch Controller", 1, true, true);
		} else {
			p_arvr_data->trackers[p_which] = arvr_api->godot_arvr_add_controller("Right Oculus Touch Controller", 2, true, true);
		}

		p_arvr_data->handTriggerPressed[hand] = false;
		p_arvr_data->indexTriggerPressed[hand] = false;
	}

	// note that I'm keeping the button assignments the same as we're currently using in OpenVR

	// update button and touch states, note that godot will ignore buttons that didn't change
	if (p_which == TRACKER_LEFT_TOUCH) {
		arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 1, p_arvr_data->inputState.Buttons & ovrButton_Y);
		arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 3, p_arvr_data->inputState.Buttons & ovrButton_Enter); // menu button
		arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 5, p_arvr_data->inputState.Touches & ovrTouch_X);
		arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 6, p_arvr_data->inputState.Touches & ovrTouch_Y);
		arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 7, p_arvr_data->inputState.Buttons & ovrButton_X);

		arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 9, p_arvr_data->inputState.Touches & ovrTouch_LThumbRest);
		arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 10, p_arvr_data->inputState.Touches & ovrTouch_LThumbUp);

		arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 11, p_arvr_data->inputState.Touches & ovrTouch_LIndexTrigger);
		arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 12, p_arvr_data->inputState.Touches & ovrTouch_LIndexPointing);

		arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 14, p_arvr_data->inputState.Buttons & ovrButton_LThumb);
	} else {
		arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 1, p_arvr_data->inputState.Buttons & ovrButton_B);
		arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 3, p_arvr_data->inputState.Buttons & ovrButton_Home); // oculus button
		arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 5, p_arvr_data->inputState.Touches & ovrTouch_A);
		arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 6, p_arvr_data->inputState.Touches & ovrTouch_Y);
		arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 7, p_arvr_data->inputState.Buttons & ovrButton_A);

		arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 9, p_arvr_data->inputState.Touches & ovrTouch_RThumbRest);
		arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 10, p_arvr_data->inputState.Touches & ovrTouch_RThumbUp);

		arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 11, p_arvr_data->inputState.Touches & ovrTouch_RIndexTrigger);
		arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 12, p_arvr_data->inputState.Touches & ovrTouch_RIndexPointing);

		arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 14, p_arvr_data->inputState.Buttons & ovrButton_RThumb);
	}

	if (p_arvr_data->handTriggerPressed[hand] && p_arvr_data->inputState.HandTrigger[hand] < 0.4) {
		p_arvr_data->handTriggerPressed[hand] = false;
	} else if (!p_arvr_data->handTriggerPressed[hand] && p_arvr_data->inputState.HandTrigger[hand] > 0.6) {
		p_arvr_data->handTriggerPressed[hand] = true;
	}
	arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 2, p_arvr_data->handTriggerPressed[hand]);

	if (p_arvr_data->indexTriggerPressed[hand] && p_arvr_data->inputState.IndexTrigger[hand] < 0.4) {
		p_arvr_data->indexTriggerPressed[hand] = false;
	} else if (!p_arvr_data->indexTriggerPressed[hand] && p_arvr_data->inputState.IndexTrigger[hand] > 0.6) {
		p_arvr_data->indexTriggerPressed[hand] = true;
	}
	arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 15, p_arvr_data->indexTriggerPressed[hand]);

	// update axis states
	arvr_api->godot_arvr_set_controller_axis(p_arvr_data->trackers[p_which], 0, p_arvr_data->inputState.Thumbstick[hand].x, true);
	arvr_api->godot_arvr_set_controller_axis(p_arvr_data->trackers[p_which], 1, p_arvr_data->inputState.Thumbstick[hand].y, true);
	arvr_api->godot_arvr_set_controller_axis(p_arvr_data->trackers[p_which], 2, p_arvr_data->inputState.IndexTrigger[hand], true);
	arvr_api->godot_arvr_set_controller_axis(p_arvr_data->trackers[p_which], 3, p_arvr_data->inputState.HandTrigger[hand], true);

	// update orientation and position
	godot_transform transform;
	oculus_transform_from_pose(&transform, &p_arvr_data->trackState.HandPoses[hand].ThePose , 1.0);
	arvr_api->godot_arvr_set_controller_transform(p_arvr_data->trackers[p_which], &transform, true, true);
}

void godot_arvr_process(void *p_data) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	// this method gets called before every frame is rendered, here is where you
	// should update tracking data, update controllers, etc.

	// first check if Oculus wants us to react to something
	if (arvr_data->oculus_is_initialized) {
		ovrSessionStatus sessionStatus;
		ovr_GetSessionStatus(arvr_data->session, &sessionStatus);
		if (sessionStatus.ShouldQuit) {
			// bye bye oculus.. We possibly need to signal Godot that its time to exit completely..
			godot_arvr_uninitialize(p_data);
		} else {
			if (sessionStatus.ShouldRecenter)
				ovr_RecenterTrackingOrigin(arvr_data->session);

			// Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. HmdToEyePose) may change at runtime.
			arvr_data->eyeRenderDesc[0] = ovr_GetRenderDesc(arvr_data->session, ovrEye_Left, arvr_data->hmdDesc.DefaultEyeFov[0]);
			arvr_data->eyeRenderDesc[1] = ovr_GetRenderDesc(arvr_data->session, ovrEye_Right, arvr_data->hmdDesc.DefaultEyeFov[1]);

			// Get eye poses, feeding in correct IPD offset
			arvr_data->HmdToEyePose[0] = arvr_data->eyeRenderDesc[0].HmdToEyePose;
			arvr_data->HmdToEyePose[1] = arvr_data->eyeRenderDesc[1].HmdToEyePose;

			ovr_GetEyePoses(arvr_data->session, arvr_data->frameIndex, ovrTrue, arvr_data->HmdToEyePose, arvr_data->EyeRenderPose, &arvr_data->sensorSampleTime);

			// update our controller state
			double frame_timing = 1.0; // need to do something with this..
			arvr_data->trackState = ovr_GetTrackingState(arvr_data->session, frame_timing, ovrFalse);
			ovr_GetInputState(arvr_data->session, ovrControllerType_Active, &arvr_data->inputState);

			// and now handle our controllers, not that Godot is perfectly capable of handling the XBox controller, no need to add double support
			unsigned int which_controllers_do_we_have = ovr_GetConnectedControllerTypes(arvr_data->session);

			if (which_controllers_do_we_have & ovrControllerType_LTouch) {
				oculus_update_touch_controller(arvr_data, TRACKER_LEFT_TOUCH);
			} else if (arvr_data->trackers[TRACKER_LEFT_TOUCH] != 0) {
				// if we previously had our left touch controller, clean up
				arvr_api->godot_arvr_remove_controller(arvr_data->trackers[TRACKER_LEFT_TOUCH]);
				arvr_data->trackers[TRACKER_LEFT_TOUCH] = 0;
			}

			if (which_controllers_do_we_have & ovrControllerType_RTouch) {
				oculus_update_touch_controller(arvr_data, TRACKER_RIGHT_TOUCH);
			} else if (arvr_data->trackers[TRACKER_RIGHT_TOUCH] != 0) {
				// if we previously had our right touch controller, clean up
				arvr_api->godot_arvr_remove_controller(arvr_data->trackers[TRACKER_RIGHT_TOUCH]);
				arvr_data->trackers[TRACKER_RIGHT_TOUCH] = 0;
			}

			if (which_controllers_do_we_have & ovrControllerType_Remote) {
				// should add support for our remote... 
			} else {
				// if we previously had our remote, clean up
			}
		}
	}
}

void *godot_arvr_constructor(godot_object *p_instance) {
	godot_string ret;

	arvr_data_struct *arvr_data = (arvr_data_struct *)api->godot_alloc(sizeof(arvr_data_struct));

	arvr_data->oculus_is_initialized = false;
	arvr_data->eyeRenderTexture[0] = NULL;
	arvr_data->eyeRenderTexture[1] = NULL;
	for (int tracker = 0; tracker < MAX_TRACKERS; tracker++) {
		arvr_data->trackers[tracker] = 0;
	}

    // Initializes LibOVR, and the Rift
	ovrInitParams initParams = { ovrInit_RequestVersion, OVR_MINOR_VERSION, NULL, 0, 0 };
	ovrResult result = ovr_Initialize(&initParams);
    if (!OVR_SUCCESS(result)) {
    	printf("Failed to initialize libOVR.\n");
    }

	// we should have only one so should be pretty safe
	arvr_data->shader = blit_shader_init();

	return arvr_data;
};

void godot_arvr_destructor(void *p_data) {
	if (p_data != NULL) {
		arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

		if (arvr_data->oculus_is_initialized) {
			// this should have already been called... But just in case...
			godot_arvr_uninitialize(p_data);
		}

		blit_shader_cleanup(&arvr_data->shader);

		api->godot_free(p_data);
	};
};

const godot_arvr_interface_gdnative interface_struct = {
	GODOTVR_API_MAJOR, GODOTVR_API_MINOR,
	godot_arvr_constructor,
	godot_arvr_destructor,
	godot_arvr_get_name,
	godot_arvr_get_capabilities,
	godot_arvr_get_anchor_detection_is_enabled,
	godot_arvr_set_anchor_detection_is_enabled,
	godot_arvr_is_stereo,
	godot_arvr_is_initialized,
	godot_arvr_initialize,
	godot_arvr_uninitialize,
	godot_arvr_get_render_targetsize,
	godot_arvr_get_transform_for_eye,
	godot_arvr_fill_projection_for_eye,
	godot_arvr_commit_for_eye,
	godot_arvr_process
};
