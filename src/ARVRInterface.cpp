////////////////////////////////////////////////////////////////////////////////////////////////
// Our main ARVRInterface code for our Oculus GDNative module

#include "ARVRInterface.h"

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

void godot_arvr_set_anchor_detection_is_enabled(void *p_data, bool p_enable) {
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

	ret = arvr_data->state == OVR_INITIALISED;

	return ret;
}

godot_bool godot_arvr_initialize(void *p_data) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	if (arvr_data->state == OVR_INITIALISED) {
		return true;
	} else if (arvr_data->state == OVR_NOT_FOUND) {
		return false;
	} else {
		// initialise this interface, so initialize any 3rd party libraries, open up
		// HMD window if required, etc.
		printf("Oculus - initializing...\n");

		// first time? check if we can initialise our OVR module
		if (arvr_data->state == OVR_NOT_CHECKED) {
			// Initializes LibOVR, and the Rift
			ovrInitParams initParams = { ovrInit_RequestVersion, OVR_MINOR_VERSION, NULL, 0, 0 };
			ovrResult result = ovr_Initialize(&initParams);
			if (!OVR_SUCCESS(result)) {
				arvr_data->state = OVR_NOT_FOUND;

				printf("Failed to initialize libOVR.\n");
				return false;
			}
		}

		arvr_data->state = OVR_INITIALISED;

		// we delay setting up our OVR session until we're called from our render
		// thread

		printf("Oculus - successfully initialized\n");
	}

	// and return our result
	return arvr_data->state == OVR_INITIALISED;
}

void godot_arvr_uninitialize(void *p_data) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	if (arvr_data->state == OVR_INITIALISED) {
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

		if (arvr_data->session != NULL) {
			delete arvr_data->session;
			arvr_data->session = NULL;
		}

		arvr_data->state = OVR_NOT_INITIALISED;
	}
}

godot_vector2 godot_arvr_get_render_targetsize(const void *p_data) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;
	OVRSession *session = arvr_data->session;
	godot_vector2 size;

	if ((arvr_data->state == OVR_INITIALISED) && (session != NULL)) {
		api->godot_vector2_new(&size, arvr_data->width, arvr_data->height);
	} else {
		api->godot_vector2_new(&size, 500, 500);
	}

	return size;
}

godot_transform godot_arvr_get_transform_for_eye(void *p_data, godot_int p_eye, godot_transform *p_cam_transform) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;
	OVRSession *session = arvr_data->session;

	godot_transform transform_for_eye;
	godot_transform reference_frame = arvr_api->godot_arvr_get_reference_frame();
	godot_transform ret;
	godot_vector3 offset;
	godot_real world_scale = arvr_api->godot_arvr_get_worldscale();

	if ((arvr_data->state == OVR_INITIALISED) && (session != NULL)) {
		if (p_eye == 0) {
			// Mono is requested when we want to update our refence frame or position our camera in the scene
			arvr_data->session->hmd_transform(&transform_for_eye, world_scale);
		} else {
			arvr_data->session->eye_transform(p_eye == 2 ? 1 : 0, &transform_for_eye, world_scale);
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
	//	ret = api->godot_transform_operator_multiply(&ret,
	//&arvr_data->hmd_transform);
	ret = api->godot_transform_operator_multiply(&ret, &transform_for_eye);

	return ret;
}

void godot_arvr_fill_projection_for_eye(void *p_data, godot_real *p_projection, godot_int p_eye, godot_real p_aspect, godot_real p_z_near, godot_real p_z_far) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;
	OVRSession *session = arvr_data->session;

	if (arvr_data->state == OVR_INITIALISED && session != NULL) {
		session->projection_matrix(p_eye == 2 ? 1 : 0, p_projection, p_z_near, p_z_far);
	} else {
		// uhm, should do something here really..
	}
}

void godot_arvr_commit_for_eye(void *p_data, godot_int p_eye, godot_rid *p_render_target, godot_rect2 *p_screen_rect) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;
	OVRSession *session = arvr_data->session;

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
		// blit as mono, attempt to keep our aspect ratio and center our render
		// buffer
		float new_height = screen_rect.size.x * (render_size.y / render_size.x);
		if (new_height > screen_rect.size.y) {
			screen_rect.position.y = (0.5 * screen_rect.size.y) - (0.5 * new_height);
			screen_rect.size.y = new_height;
		} else {
			float new_width = screen_rect.size.y * (render_size.x / render_size.y);

			screen_rect.position.x = (0.5 * screen_rect.size.x) - (0.5 * new_width);
			screen_rect.size.x = new_width;
		};

		// printf("Blit: %0.2f, %0.2f - %0.2f,
		// %0.2f\n",screen_rect.position.x,screen_rect.position.y,screen_rect.size.x,screen_rect.size.y);

		arvr_api->godot_arvr_blit(0, p_render_target, &screen_rect);
	}

	if (arvr_data->state == OVR_INITIALISED && session != NULL) {
		int eye = p_eye == 2 ? 1 : 0;

		if (!arvr_data->has_external_texture_support) {
			uint32_t texid = arvr_api->godot_arvr_get_texid(p_render_target);

			// make sure we have a buffer
			if (arvr_data->eyeRenderTexture[eye] == NULL) {
				arvr_data->eyeRenderTexture[eye] = session->make_texture_buffer(eye, true, OS_get_current_video_driver());
			}

			if (arvr_data->eyeRenderTexture[eye]->TextureChain) {
				// blit to OVRs buffers
				// Switch to eye render target
				arvr_data->eyeRenderTexture[eye]->set_render_surface();

				// copy our buffer...
				if (arvr_data->shader == NULL) {
					arvr_data->shader = new blit_shader();
				}
				if (arvr_data->shader != NULL) {
					arvr_data->shader->render(texid);
				}

				// Avoids an error when calling SetAndClearRenderSurface during next
				// iteration. Without this, during the next while loop iteration
				// SetAndClearRenderSurface would bind a framebuffer with an invalid
				// COLOR_ATTACHMENT0 because the texture ID associated with
				// COLOR_ATTACHMENT0 had been unlocked by calling wglDXUnlockObjectsNV.
				arvr_data->eyeRenderTexture[eye]->unset_render_surface();

				// Commit changes to the textures so they get picked up frame
				arvr_data->eyeRenderTexture[eye]->commit();
			}
		} else if (arvr_data->eyeRenderTexture[eye]->TextureChain) {
			// Commit changes to the textures so they get picked up frame
			arvr_data->eyeRenderTexture[eye]->commit();
		}

		if (p_eye == 2) {
			// both eyes are rendered, time to output...
			if (!session->submit_frame(arvr_data->eyeRenderTexture, 2)) {
				// do something here?
			}
		}
	}
}

void oculus_update_touch_controller(arvr_data_struct *p_arvr_data, int p_which) {
	OVRSession *session = p_arvr_data->session;

	int hand = p_which == TRACKER_LEFT_TOUCH ? 0 : 1;
	if (p_arvr_data->trackers[p_which] == 0) {
		// need to init a new tracker
		if (p_which == TRACKER_LEFT_TOUCH) {
			p_arvr_data->trackers[p_which] = arvr_api->godot_arvr_add_controller(
					"Left Oculus Touch Controller", 1, true, true);
		} else {
			p_arvr_data->trackers[p_which] = arvr_api->godot_arvr_add_controller(
					"Right Oculus Touch Controller", 2, true, true);
		}

		p_arvr_data->handTriggerPressed[hand] = false;
		p_arvr_data->indexTriggerPressed[hand] = false;
	}

	// note that I'm keeping the button assignments the same as we're currently
	// using in OpenVR

	// update button and touch states, note that godot will ignore buttons that
	// didn't change
	if (p_which == TRACKER_LEFT_TOUCH) {
		arvr_api->godot_arvr_set_controller_button(
				p_arvr_data->trackers[p_which], 1,
				session->is_button_pressed(ovrButton_Y));
		arvr_api->godot_arvr_set_controller_button(
				p_arvr_data->trackers[p_which], 3,
				session->is_button_pressed(ovrButton_Enter)); // menu button
		arvr_api->godot_arvr_set_controller_button(
				p_arvr_data->trackers[p_which], 5,
				session->is_button_touched(ovrTouch_X));
		arvr_api->godot_arvr_set_controller_button(
				p_arvr_data->trackers[p_which], 6,
				session->is_button_touched(ovrTouch_Y));
		arvr_api->godot_arvr_set_controller_button(
				p_arvr_data->trackers[p_which], 7,
				session->is_button_pressed(ovrButton_X));

		arvr_api->godot_arvr_set_controller_button(
				p_arvr_data->trackers[p_which], 9,
				session->is_button_touched(ovrTouch_LThumbRest));
		arvr_api->godot_arvr_set_controller_button(
				p_arvr_data->trackers[p_which], 10,
				session->is_button_touched(ovrTouch_LThumbUp));

		arvr_api->godot_arvr_set_controller_button(
				p_arvr_data->trackers[p_which], 11,
				session->is_button_touched(ovrTouch_LIndexTrigger));
		arvr_api->godot_arvr_set_controller_button(
				p_arvr_data->trackers[p_which], 12,
				session->is_button_touched(ovrTouch_LIndexPointing));

		arvr_api->godot_arvr_set_controller_button(
				p_arvr_data->trackers[p_which], 14,
				session->is_button_pressed(ovrButton_LThumb));
	} else {
		arvr_api->godot_arvr_set_controller_button(
				p_arvr_data->trackers[p_which], 1,
				session->is_button_pressed(ovrButton_B));
		arvr_api->godot_arvr_set_controller_button(
				p_arvr_data->trackers[p_which], 3,
				session->is_button_pressed(ovrButton_Home)); // oculus button
		arvr_api->godot_arvr_set_controller_button(
				p_arvr_data->trackers[p_which], 5,
				session->is_button_touched(ovrTouch_A));
		arvr_api->godot_arvr_set_controller_button(
				p_arvr_data->trackers[p_which], 6,
				session->is_button_touched(ovrTouch_B));
		arvr_api->godot_arvr_set_controller_button(
				p_arvr_data->trackers[p_which], 7,
				session->is_button_pressed(ovrButton_A));

		arvr_api->godot_arvr_set_controller_button(
				p_arvr_data->trackers[p_which], 9,
				session->is_button_pressed(ovrTouch_RThumbRest));
		arvr_api->godot_arvr_set_controller_button(
				p_arvr_data->trackers[p_which], 10,
				session->is_button_pressed(ovrTouch_RThumbUp));

		arvr_api->godot_arvr_set_controller_button(
				p_arvr_data->trackers[p_which], 11,
				session->is_button_touched(ovrTouch_RIndexTrigger));
		arvr_api->godot_arvr_set_controller_button(
				p_arvr_data->trackers[p_which], 12,
				session->is_button_touched(ovrTouch_RIndexPointing));

		arvr_api->godot_arvr_set_controller_button(
				p_arvr_data->trackers[p_which], 14,
				session->is_button_pressed(ovrButton_RThumb));
	}

	float hand_trigger = session->hand_trigger(hand);
	if (p_arvr_data->handTriggerPressed[hand] && hand_trigger < 0.4) {
		p_arvr_data->handTriggerPressed[hand] = false;
	} else if (!p_arvr_data->handTriggerPressed[hand] && hand_trigger > 0.6) {
		p_arvr_data->handTriggerPressed[hand] = true;
	}
	arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 2, p_arvr_data->handTriggerPressed[hand]);

	float index_trigger = session->index_trigger(hand);
	if (p_arvr_data->indexTriggerPressed[hand] && index_trigger < 0.4) {
		p_arvr_data->indexTriggerPressed[hand] = false;
	} else if (!p_arvr_data->indexTriggerPressed[hand] && index_trigger > 0.6) {
		p_arvr_data->indexTriggerPressed[hand] = true;
	}
	arvr_api->godot_arvr_set_controller_button(p_arvr_data->trackers[p_which], 15, p_arvr_data->indexTriggerPressed[hand]);

	// update axis states
	ovrVector2f thumb_stick = session->thumb_stick(hand);
	arvr_api->godot_arvr_set_controller_axis(p_arvr_data->trackers[p_which], 0, thumb_stick.x, true);
	arvr_api->godot_arvr_set_controller_axis(p_arvr_data->trackers[p_which], 1, thumb_stick.y, true);
	arvr_api->godot_arvr_set_controller_axis(p_arvr_data->trackers[p_which], 2, index_trigger, true);
	arvr_api->godot_arvr_set_controller_axis(p_arvr_data->trackers[p_which], 3, hand_trigger, true);

	// update orientation and position
	godot_transform transform;
	session->hand_transform(hand, &transform);
	arvr_api->godot_arvr_set_controller_transform(p_arvr_data->trackers[p_which], &transform, true, true);
}

int godot_arvr_glad_status = 0;

void godot_arvr_process(void *p_data) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	// this method gets called before every frame is rendered, here is where you
	// should update tracking data, update controllers, etc.

	// first check if Oculus wants us to react to something
	if (arvr_data->state == OVR_INITIALISED) {
		// only attempt to initialie glad once
		if (godot_arvr_glad_status == 0) {
			if (gladLoadGL()) {
				godot_arvr_glad_status = 1;
			} else {
				godot_arvr_glad_status = 2;
				printf("Error initializing GLAD\n");
			}
		}

		if (godot_arvr_glad_status == 1) {
			// we wait with setting up our session until we are here so we are in the
			// render thread...
			if (arvr_data->session == NULL) {
				arvr_data->session = new OVRSession();

				if (arvr_data->session->is_initialised()) {
					// Get the size of our eye buffer, as the right eye is what we'll make
					// available to Godot to display we're using it's size here. It is a
					// fair assumption it will be the same for both eyes though.
					ovrSizei idealTextureSize = arvr_data->session->get_texture_size(1);
					arvr_data->width = idealTextureSize.w;
					arvr_data->height = idealTextureSize.h;
				}
			}

			if (arvr_data->session->is_initialised()) {
				ovrSessionStatus sessionStatus;
				arvr_data->session->session_status(&sessionStatus);
				if (sessionStatus.ShouldQuit) {
					// bye bye oculus.. We possibly need to signal Godot that its time to
					// exit completely..
					godot_arvr_uninitialize(p_data);
				} else {
					if (sessionStatus.ShouldRecenter)
						arvr_data->session->recenter_tracking_origin();

					arvr_data->session->update_eye_poses();
					arvr_data->session->update_states();

					// and now handle our controllers, note that Godot is perfectly
					// capable of handling the XBox controller, no need to add double
					// support
					unsigned int which_controllers_do_we_have =
							arvr_data->session->get_connected_controller_types();

					if (which_controllers_do_we_have & ovrControllerType_LTouch) {
						oculus_update_touch_controller(arvr_data, TRACKER_LEFT_TOUCH);
					} else if (arvr_data->trackers[TRACKER_LEFT_TOUCH] != 0) {
						// if we previously had our left touch controller, clean up
						arvr_api->godot_arvr_remove_controller(
								arvr_data->trackers[TRACKER_LEFT_TOUCH]);
						arvr_data->trackers[TRACKER_LEFT_TOUCH] = 0;
					}

					if (which_controllers_do_we_have & ovrControllerType_RTouch) {
						oculus_update_touch_controller(arvr_data, TRACKER_RIGHT_TOUCH);
					} else if (arvr_data->trackers[TRACKER_RIGHT_TOUCH] != 0) {
						// if we previously had our right touch controller, clean up
						arvr_api->godot_arvr_remove_controller(
								arvr_data->trackers[TRACKER_RIGHT_TOUCH]);
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
	}
}

void *godot_arvr_constructor(godot_object *p_instance) {
	godot_string ret;

	arvr_data_struct *arvr_data = (arvr_data_struct *)api->godot_alloc(sizeof(arvr_data_struct));

	arvr_data->state = OVR_NOT_CHECKED;
	arvr_data->has_external_texture_support = false;
	arvr_data->session = NULL;
	arvr_data->eyeRenderTexture[0] = NULL;
	arvr_data->eyeRenderTexture[1] = NULL;
	for (int tracker = 0; tracker < MAX_TRACKERS; tracker++) {
		arvr_data->trackers[tracker] = 0;
	}

	arvr_data->shader = NULL;

	return arvr_data;
}

void godot_arvr_destructor(void *p_data) {
	if (p_data != NULL) {
		arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

		if (arvr_data->state == OVR_INITIALISED) {
			// this should have already been called... But just in case...
			godot_arvr_uninitialize(p_data);
		}

		if (arvr_data->shader != NULL) {
			delete arvr_data->shader;
			arvr_data->shader = NULL;
		}

		api->godot_free(p_data);
	}
}

int godot_arvr_get_external_texture_for_eye(void *p_data, int p_eye) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;
	OVRSession *session = arvr_data->session;

	// this only gets called from Godot 3.2 and newer, allows us to use Oculus
	// texture chain directly.

	// process should be called by now but just in case...
	if (arvr_data->state == OVR_INITIALISED && session != NULL) {
		int eye = p_eye == 2 ? 1 : 0;

		// make sure we know that we're rendering directly to our texture chain
		arvr_data->has_external_texture_support = true;

		if (arvr_data->eyeRenderTexture[eye] == NULL) {
			arvr_data->eyeRenderTexture[eye] = session->make_texture_buffer(eye, false, OS_get_current_video_driver());
		}

		// get the next texture from our chain
		if (arvr_data->eyeRenderTexture[eye]->TextureChain) {
			return arvr_data->eyeRenderTexture[eye]->get_next_texture();
		}
	}

	return 0;
}

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
	godot_arvr_process,
	// only available in Godot 3.2+
	godot_arvr_get_external_texture_for_eye
};
