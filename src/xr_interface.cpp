////////////////////////////////////////////////////////////////////////////////////////////////
// Our main ARVRInterface code for our Oculus GDNative module

#include "xr_interface.h"

///////////////////////////////////////////////////////////////////////////////////////////
// our XR interface
godot_string ovr_get_name(const void *p_data) {
	godot_string ret;

	char name[] = "Oculus";
	api->godot_string_new(&ret);
	api->godot_string_parse_utf8(&ret, name);

	return ret;
}

godot_int ovr_get_capabilities(const void *p_data) {
	godot_int ret;

	ret = 2 + 8; // 2 = XR_STEREO, 8 = XR_EXTERNAL

	return ret;
}

godot_bool ovr_get_anchor_detection_is_enabled(const void *p_data) {
	godot_bool ret;

	ret = false; // does not apply here

	return ret;
}

void ovr_set_anchor_detection_is_enabled(void *p_data, bool p_enable) {
	// we ignore this, not supported in this interface!
}

godot_bool ovr_is_stereo(const void *p_data) {
	godot_bool ret;

	ret = true;

	return ret;
}

godot_bool ovr_is_initialized(const void *p_data) {
	godot_bool ret;
	xr_data_struct *xr_data = (xr_data_struct *)p_data;

	ret = xr_data->state == OVR_INITIALISED;

	return ret;
}

godot_bool ovr_initialize(void *p_data) {
	xr_data_struct *xr_data = (xr_data_struct *)p_data;

	if (xr_data->state == OVR_INITIALISED) {
		return true;
	} else if (xr_data->state == OVR_NOT_FOUND) {
		return false;
	} else {
		// initialise this interface, so initialize any 3rd party libraries, open up
		// HMD window if required, etc.
		printf("Oculus - initializing...\n");

		// first time? check if we can initialise our OVR module
		if (xr_data->state == OVR_NOT_CHECKED) {
			// Initializes LibOVR, and the Rift
			ovrInitParams initParams = { ovrInit_RequestVersion, OVR_MINOR_VERSION, NULL, 0, 0 };
			ovrResult result = ovr_Initialize(&initParams);
			if (!OVR_SUCCESS(result)) {
				xr_data->state = OVR_NOT_FOUND;

				printf("Failed to initialize libOVR.\n");
				return false;
			}
		}

		xr_data->state = OVR_INITIALISED;

		// we delay setting up our OVR session until we're called from our render
		// thread

		printf("Oculus - successfully initialized\n");
	}

	// and return our result
	return xr_data->state == OVR_INITIALISED;
}

void ovr_uninitialize(void *p_data) {
	xr_data_struct *xr_data = (xr_data_struct *)p_data;

	if (xr_data->state == OVR_INITIALISED) {
		// note, this will already be removed as the primary interface by
		// ARVRInterfaceGDNative

		for (int tracker = 0; tracker < MAX_TRACKERS; tracker++) {
			if (xr_data->trackers[tracker] != 0) {
				// if we previously had our left touch controller, clean up
				xr_api->godot_arvr_remove_controller(xr_data->trackers[tracker]);
				xr_data->trackers[tracker] = 0;
			}
		}

		for (int eye = 0; eye < 2; ++eye) {
			if (xr_data->eyeRenderTexture[eye] != NULL) {
				delete xr_data->eyeRenderTexture[eye];
				xr_data->eyeRenderTexture[eye] = NULL;
			}
		}

		if (xr_data->session != NULL) {
			xr_data->session->uninitialise();
		}

		xr_data->state = OVR_NOT_INITIALISED;
	}
}

godot_vector2 ovr_get_render_targetsize(const void *p_data) {
	xr_data_struct *xr_data = (xr_data_struct *)p_data;
	godot_vector2 size;

	if ((xr_data->state == OVR_INITIALISED) && (xr_data->session != NULL)) {
		api->godot_vector2_new(&size, (godot_real)xr_data->width, (godot_real)xr_data->height);
	} else {
		api->godot_vector2_new(&size, 500.0f, 500.0f);
	}

	return size;
}

godot_transform ovr_get_transform_for_eye(void *p_data, godot_int p_eye, godot_transform *p_cam_transform) {
	xr_data_struct *xr_data = (xr_data_struct *)p_data;

	godot_transform transform_for_eye;
	godot_transform reference_frame = xr_api->godot_arvr_get_reference_frame();
	godot_transform ret;
	godot_real world_scale = xr_api->godot_arvr_get_worldscale();

	if ((xr_data->state == OVR_INITIALISED) && (xr_data->session != NULL)) {
		if (p_eye == 0) {
			// Mono is requested when we want to update our refence frame or position our camera in the scene
			xr_data->session->hmd_transform(&transform_for_eye, world_scale);
		} else {
			xr_data->session->eye_transform(p_eye == 2 ? 1 : 0, &transform_for_eye, world_scale);
		}
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
	}

	// Now construct our full transform
	ret = *p_cam_transform;
	ret = api->godot_transform_operator_multiply(&ret, &reference_frame);
	ret = api->godot_transform_operator_multiply(&ret, &transform_for_eye);

	return ret;
}

void ovr_fill_projection_for_eye(void *p_data, godot_real *p_projection, godot_int p_eye, godot_real p_aspect, godot_real p_z_near, godot_real p_z_far) {
	xr_data_struct *xr_data = (xr_data_struct *)p_data;

	if (xr_data->state == OVR_INITIALISED && xr_data->session != NULL) {
		xr_data->session->projection_matrix(p_eye == 2 ? 1 : 0, p_projection, p_z_near, p_z_far);
	} else {
		// uhm, should do something here really..
	}
}

void ovr_commit_for_eye(void *p_data, godot_int p_eye, godot_rid *p_render_target, godot_rect2 *p_screen_rect) {
	xr_data_struct *xr_data = (xr_data_struct *)p_data;

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
	godot_vector2 render_size = ovr_get_render_targetsize(p_data);

	if (p_eye == 1 && !api->godot_rect2_has_no_area(&screen_rect)) {
		// blit as mono, attempt to keep our aspect ratio and center our render
		// buffer
		float new_height = screen_rect.size.x * (render_size.y / render_size.x);
		if (new_height > screen_rect.size.y) {
			screen_rect.position.y = (0.5f * screen_rect.size.y) - (0.5f * new_height);
			screen_rect.size.y = new_height;
		} else {
			float new_width = screen_rect.size.y * (render_size.x / render_size.y);

			screen_rect.position.x = (0.5f * screen_rect.size.x) - (0.5f * new_width);
			screen_rect.size.x = new_width;
		};

		// printf("Blit: %0.2f, %0.2f - %0.2f,
		// %0.2f\n",screen_rect.position.x,screen_rect.position.y,screen_rect.size.x,screen_rect.size.y);

		xr_api->godot_arvr_blit(0, p_render_target, &screen_rect);
	}

	if (xr_data->state == OVR_INITIALISED && xr_data->session != NULL) {
		int eye = p_eye == 2 ? 1 : 0;

		if (!xr_data->has_external_texture_support) {
			uint32_t texid = xr_api->godot_arvr_get_texid(p_render_target);

			// make sure we have a buffer
			if (xr_data->eyeRenderTexture[eye] == NULL) {
				xr_data->eyeRenderTexture[eye] = xr_data->session->make_texture_buffer(eye, true, OS::get_singleton()->get_current_video_driver());
			}

			if (xr_data->eyeRenderTexture[eye]->TextureChain) {
				// blit to OVRs buffers
				// Switch to eye render target
				xr_data->eyeRenderTexture[eye]->set_render_surface();

				// copy our buffer...
				if (xr_data->shader == NULL) {
					xr_data->shader = new blit_shader();
				}
				if (xr_data->shader != NULL) {
					xr_data->shader->render(texid);
				}

				// Avoids an error when calling SetAndClearRenderSurface during next
				// iteration. Without this, during the next while loop iteration
				// SetAndClearRenderSurface would bind a framebuffer with an invalid
				// COLOR_ATTACHMENT0 because the texture ID associated with
				// COLOR_ATTACHMENT0 had been unlocked by calling wglDXUnlockObjectsNV.
				xr_data->eyeRenderTexture[eye]->unset_render_surface();

				// Commit changes to the textures so they get picked up frame
				xr_data->eyeRenderTexture[eye]->commit();
			}
		} else if (xr_data->eyeRenderTexture[eye]->TextureChain) {
			// Commit changes to the textures so they get picked up frame
			xr_data->eyeRenderTexture[eye]->commit();
		}

		if (p_eye == 2) {
			// both eyes are rendered, time to output...
			if (!xr_data->session->submit_frame(xr_data->eyeRenderTexture, 2)) {
				// do something here?
			}
		}
	}
}

void oculus_update_touch_controller(xr_data_struct *p_xr_data, int p_which) {
	int hand = p_which == TRACKER_LEFT_TOUCH ? 0 : 1;
	if (p_xr_data->trackers[p_which] == 0) {
		// need to init a new tracker
		if (p_which == TRACKER_LEFT_TOUCH) {
			p_xr_data->trackers[p_which] = xr_api->godot_arvr_add_controller(
					"Left Oculus Touch Controller", 1, true, true);
		} else {
			p_xr_data->trackers[p_which] = xr_api->godot_arvr_add_controller(
					"Right Oculus Touch Controller", 2, true, true);
		}

		p_xr_data->handTriggerPressed[hand] = false;
		p_xr_data->indexTriggerPressed[hand] = false;
	}

	// note that I'm keeping the button assignments the same as we're currently
	// using in OpenVR

	// update button and touch states, note that godot will ignore buttons that
	// didn't change
	if (p_which == TRACKER_LEFT_TOUCH) {
		xr_api->godot_arvr_set_controller_button(
				p_xr_data->trackers[p_which], 1,
				p_xr_data->session->is_button_pressed(ovrButton_Y));
		xr_api->godot_arvr_set_controller_button(
				p_xr_data->trackers[p_which], 3,
				p_xr_data->session->is_button_pressed(ovrButton_Enter)); // menu button
		xr_api->godot_arvr_set_controller_button(
				p_xr_data->trackers[p_which], 5,
				p_xr_data->session->is_button_touched(ovrTouch_X));
		xr_api->godot_arvr_set_controller_button(
				p_xr_data->trackers[p_which], 6,
				p_xr_data->session->is_button_touched(ovrTouch_Y));
		xr_api->godot_arvr_set_controller_button(
				p_xr_data->trackers[p_which], 7,
				p_xr_data->session->is_button_pressed(ovrButton_X));

		xr_api->godot_arvr_set_controller_button(
				p_xr_data->trackers[p_which], 9,
				p_xr_data->session->is_button_touched(ovrTouch_LThumbRest));
		xr_api->godot_arvr_set_controller_button(
				p_xr_data->trackers[p_which], 10,
				p_xr_data->session->is_button_touched(ovrTouch_LThumbUp));

		xr_api->godot_arvr_set_controller_button(
				p_xr_data->trackers[p_which], 11,
				p_xr_data->session->is_button_touched(ovrTouch_LIndexTrigger));
		xr_api->godot_arvr_set_controller_button(
				p_xr_data->trackers[p_which], 12,
				p_xr_data->session->is_button_touched(ovrTouch_LIndexPointing));

		xr_api->godot_arvr_set_controller_button(
				p_xr_data->trackers[p_which], 14,
				p_xr_data->session->is_button_pressed(ovrButton_LThumb));
	} else {
		xr_api->godot_arvr_set_controller_button(
				p_xr_data->trackers[p_which], 1,
				p_xr_data->session->is_button_pressed(ovrButton_B));
		xr_api->godot_arvr_set_controller_button(
				p_xr_data->trackers[p_which], 3,
				p_xr_data->session->is_button_pressed(ovrButton_Home)); // oculus button
		xr_api->godot_arvr_set_controller_button(
				p_xr_data->trackers[p_which], 5,
				p_xr_data->session->is_button_touched(ovrTouch_A));
		xr_api->godot_arvr_set_controller_button(
				p_xr_data->trackers[p_which], 6,
				p_xr_data->session->is_button_touched(ovrTouch_B));
		xr_api->godot_arvr_set_controller_button(
				p_xr_data->trackers[p_which], 7,
				p_xr_data->session->is_button_pressed(ovrButton_A));

		xr_api->godot_arvr_set_controller_button(
				p_xr_data->trackers[p_which], 9,
				p_xr_data->session->is_button_pressed(ovrTouch_RThumbRest));
		xr_api->godot_arvr_set_controller_button(
				p_xr_data->trackers[p_which], 10,
				p_xr_data->session->is_button_pressed(ovrTouch_RThumbUp));

		xr_api->godot_arvr_set_controller_button(
				p_xr_data->trackers[p_which], 11,
				p_xr_data->session->is_button_touched(ovrTouch_RIndexTrigger));
		xr_api->godot_arvr_set_controller_button(
				p_xr_data->trackers[p_which], 12,
				p_xr_data->session->is_button_touched(ovrTouch_RIndexPointing));

		xr_api->godot_arvr_set_controller_button(
				p_xr_data->trackers[p_which], 14,
				p_xr_data->session->is_button_pressed(ovrButton_RThumb));
	}

	float hand_trigger = p_xr_data->session->hand_trigger(hand);
	if (p_xr_data->handTriggerPressed[hand] && hand_trigger < 0.4) {
		p_xr_data->handTriggerPressed[hand] = false;
	} else if (!p_xr_data->handTriggerPressed[hand] && hand_trigger > 0.6) {
		p_xr_data->handTriggerPressed[hand] = true;
	}
	xr_api->godot_arvr_set_controller_button(p_xr_data->trackers[p_which], 2, p_xr_data->handTriggerPressed[hand]);

	float index_trigger = p_xr_data->session->index_trigger(hand);
	if (p_xr_data->indexTriggerPressed[hand] && index_trigger < 0.4) {
		p_xr_data->indexTriggerPressed[hand] = false;
	} else if (!p_xr_data->indexTriggerPressed[hand] && index_trigger > 0.6) {
		p_xr_data->indexTriggerPressed[hand] = true;
	}
	xr_api->godot_arvr_set_controller_button(p_xr_data->trackers[p_which], 15, p_xr_data->indexTriggerPressed[hand]);

	// update axis states
	ovrVector2f thumb_stick = p_xr_data->session->thumb_stick(hand);
	xr_api->godot_arvr_set_controller_axis(p_xr_data->trackers[p_which], 0, thumb_stick.x, true);
	xr_api->godot_arvr_set_controller_axis(p_xr_data->trackers[p_which], 1, thumb_stick.y, true);
	xr_api->godot_arvr_set_controller_axis(p_xr_data->trackers[p_which], 2, index_trigger, true);
	xr_api->godot_arvr_set_controller_axis(p_xr_data->trackers[p_which], 3, hand_trigger, true);

	// update orientation and position
	godot_transform transform;
	p_xr_data->session->hand_transform(hand, &transform);
	xr_api->godot_arvr_set_controller_transform(p_xr_data->trackers[p_which], &transform, true, true);

	godot_real rumble = xr_api->godot_arvr_get_controller_rumble(p_xr_data->trackers[p_which]);
	if (rumble != p_xr_data->rumble[p_which]) {
		p_xr_data->session->rumble(p_which == TRACKER_LEFT_TOUCH ? ovrControllerType_LTouch : ovrControllerType_RTouch, rumble);
		p_xr_data->rumble[p_which] = rumble;
	}
}

int ovr_glad_status = 0;

void ovr_process(void *p_data) {
	xr_data_struct *xr_data = (xr_data_struct *)p_data;

	// this method gets called before every frame is rendered, here is where you
	// should update tracking data, update controllers, etc.

	// first check if Oculus wants us to react to something
	if (xr_data->state == OVR_INITIALISED) {
		// only attempt to initialie glad once
		if (ovr_glad_status == 0) {
			if (gladLoadGL()) {
				ovr_glad_status = 1;
			} else {
				ovr_glad_status = 2;
				printf("Error initializing GLAD\n");
			}
		}

		if (ovr_glad_status == 1) {
			// we wait with setting up our session until we are here so we are in the
			// render thread...
			if (xr_data->session == NULL) {
				xr_data->session = OculusSession::get_singleton();

				// this should be the only place we call initialise on our session!
				if (xr_data->session->initialise()) {
					// Get the size of our eye buffer, as the right eye is what we'll make
					// available to Godot to display we're using it's size here. It is a
					// fair assumption it will be the same for both eyes though.
					ovrSizei idealTextureSize = xr_data->session->get_texture_size(1);
					xr_data->width = idealTextureSize.w;
					xr_data->height = idealTextureSize.h;
				}
			}

			if (xr_data->session->is_initialised()) {
				ovrSessionStatus sessionStatus;
				xr_data->session->session_status(&sessionStatus);
				if (sessionStatus.ShouldQuit) {
					// bye bye oculus.. We possibly need to signal Godot that its time to
					// exit completely..
					ovr_uninitialize(p_data);
				} else {
					if (sessionStatus.ShouldRecenter) {
						xr_data->session->recenter_tracking_origin();
					}

					xr_data->session->update_eye_poses();
					xr_data->session->update_states();

					// and now handle our controllers, note that Godot is perfectly
					// capable of handling the XBox controller, no need to add double
					// support
					unsigned int which_controllers_do_we_have = xr_data->session->get_connected_controller_types();

					if (which_controllers_do_we_have & ovrControllerType_LTouch) {
						oculus_update_touch_controller(xr_data, TRACKER_LEFT_TOUCH);
					} else if (xr_data->trackers[TRACKER_LEFT_TOUCH] != 0) {
						// if we previously had our left touch controller, clean up
						xr_api->godot_arvr_remove_controller(xr_data->trackers[TRACKER_LEFT_TOUCH]);
						xr_data->trackers[TRACKER_LEFT_TOUCH] = 0;
					}

					if (which_controllers_do_we_have & ovrControllerType_RTouch) {
						oculus_update_touch_controller(xr_data, TRACKER_RIGHT_TOUCH);
					} else if (xr_data->trackers[TRACKER_RIGHT_TOUCH] != 0) {
						// if we previously had our right touch controller, clean up
						xr_api->godot_arvr_remove_controller(
								xr_data->trackers[TRACKER_RIGHT_TOUCH]);
						xr_data->trackers[TRACKER_RIGHT_TOUCH] = 0;
					}

					if (which_controllers_do_we_have & ovrControllerType_Remote) {
						// should add support for our remote...
					} else {
						// if we previously had our remote, clean up
					}
				}
			}
		}
	}
}

void *ovr_constructor(godot_object *p_instance) {
	xr_data_struct *xr_data = (xr_data_struct *)api->godot_alloc(sizeof(xr_data_struct));

	xr_data->state = OVR_NOT_CHECKED;
	xr_data->has_external_texture_support = false;
	xr_data->session = NULL;
	xr_data->eyeRenderTexture[0] = NULL;
	xr_data->eyeRenderTexture[1] = NULL;
	for (int tracker = 0; tracker < MAX_TRACKERS; tracker++) {
		xr_data->trackers[tracker] = 0;
		xr_data->rumble[tracker] = 0;
	}

	xr_data->shader = NULL;

	return xr_data;
}

void ovr_destructor(void *p_data) {
	if (p_data != NULL) {
		xr_data_struct *xr_data = (xr_data_struct *)p_data;

		if (xr_data->state == OVR_INITIALISED) {
			// this should have already been called... But just in case...
			ovr_uninitialize(p_data);
		}

		if (xr_data->shader != NULL) {
			delete xr_data->shader;
			xr_data->shader = NULL;
		}

		api->godot_free(p_data);
	}

	// further cleanup_singleton
	OculusSession::cleanup_singleton();
	OS::cleanup_singleton();
}

int ovr_get_external_texture_for_eye(void *p_data, int p_eye) {
	xr_data_struct *xr_data = (xr_data_struct *)p_data;

	// this only gets called from Godot 3.2 and newer, allows us to use Oculus
	// texture chain directly.

	// process should be called by now but just in case...
	if (xr_data->state == OVR_INITIALISED && xr_data->session != NULL) {
		int eye = p_eye == 2 ? 1 : 0;

		// make sure we know that we're rendering directly to our texture chain
		xr_data->has_external_texture_support = true;

		if (xr_data->eyeRenderTexture[eye] == NULL) {
			xr_data->eyeRenderTexture[eye] = xr_data->session->make_texture_buffer(eye, false, OS::get_singleton()->get_current_video_driver());
		}

		// get the next texture from our chain
		if (xr_data->eyeRenderTexture[eye]->TextureChain) {
			return xr_data->eyeRenderTexture[eye]->get_next_texture();
		}
	}

	return 0;
}

void ovr_notification(void *p_data, int p_what) {
	// nothing to do here for now but we should implement this.
}

const godot_arvr_interface_gdnative interface_struct = {
	GODOTVR_API_MAJOR, GODOTVR_API_MINOR,
	ovr_constructor,
	ovr_destructor,
	ovr_get_name,
	ovr_get_capabilities,
	ovr_get_anchor_detection_is_enabled,
	ovr_set_anchor_detection_is_enabled,
	ovr_is_stereo,
	ovr_is_initialized,
	ovr_initialize,
	ovr_uninitialize,
	ovr_get_render_targetsize,
	ovr_get_transform_for_eye,
	ovr_fill_projection_for_eye,
	ovr_commit_for_eye,
	ovr_process,
	ovr_get_external_texture_for_eye,
	ovr_notification
};
