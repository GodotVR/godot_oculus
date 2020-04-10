#include "oculus_config.h"

static const char *kClassName = "OculusConfig";

GDCALLINGCONV void *oculus_config_constructor(godot_object *instance, void *method_data) {
	config_data_struct *data = (config_data_struct *)api->godot_alloc(sizeof(config_data_struct));

	// we don't have any data to store yet but allocate the buffer so we have a unique pointer for Godot to latch onto

	return data;
}

GDCALLINGCONV void oculus_config_destructor(godot_object *instance, void *method_data, void *user_data) {
	if (user_data) {
		// free it
		api->godot_free(user_data);
	}
}

GDCALLINGCONV godot_variant oculus_config_get_refresh_rate(godot_object *, void *, void *user_data, int num_args, godot_variant **args) {
	godot_variant ret;

	float refresh_rate = OculusSession::get_singleton()->get_refresh_rate();
	api->godot_variant_new_real(&ret, refresh_rate);

	return ret;
}

GDCALLINGCONV godot_variant oculus_config_play_area_available(godot_object *, void *, void *user_data, int num_args, godot_variant **args) {
	godot_variant ret;

	bool is_available = OculusSession::get_singleton()->is_initialised();
	api->godot_variant_new_bool(&ret, is_available);

	return ret;
}

GDCALLINGCONV godot_variant oculus_config_get_play_area(godot_object *, void *, void *user_data, int num_args, godot_variant **args) {
	godot_variant ret;
	godot_pool_vector3_array array;

	api->godot_pool_vector3_array_new(&array);

	const godot_vector3 *play_area = OculusSession::get_singleton()->get_play_area();
	godot_transform reference_frame = xr_api->godot_arvr_get_reference_frame();
	godot_real world_scale = xr_api->godot_arvr_get_worldscale();

	for (int i = 0; i < 4; i++) {
		godot_vector3 v = api->godot_transform_xform_inv_vector3(&reference_frame, &play_area[i]);
		v.x *= world_scale;
		v.y *= world_scale;
		v.z *= world_scale;
		api->godot_pool_vector3_array_push_back(&array, &v);
	}

	api->godot_variant_new_pool_vector3_array(&ret, &array);
	api->godot_pool_vector3_array_destroy(&array);

	return ret;
}

void register_gdnative_config(void *handle) {
	// register the constructor and destructor of the OvrInitConfig class for use in GDScript
	godot_instance_create_func create = { nullptr, nullptr, nullptr };
	create.create_func = &oculus_config_constructor;

	godot_instance_destroy_func destroy = { nullptr, nullptr, nullptr };
	destroy.destroy_func = &oculus_config_destructor;

	nativescript_api->godot_nativescript_register_class(handle, kClassName, "Reference", create, destroy);

	// register all the functions that we want to expose via the OculusConfig class in GDScript
	godot_instance_method method = { nullptr, nullptr, nullptr };
	godot_method_attributes attributes = { GODOT_METHOD_RPC_MODE_DISABLED };

	method.method = &oculus_config_get_refresh_rate;
	nativescript_api->godot_nativescript_register_method(handle, kClassName, "get_refresh_rate", attributes, method);

	method.method = &oculus_config_play_area_available;
	nativescript_api->godot_nativescript_register_method(handle, kClassName, "play_area_available", attributes, method);

	method.method = &oculus_config_get_play_area;
	nativescript_api->godot_nativescript_register_method(handle, kClassName, "get_play_area", attributes, method);
}
