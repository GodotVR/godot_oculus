////////////////////////////////////////////////////////////////////////////
// Oculus GDNative module for Godot
//
// Written by Bastiaan "Mux213" Olij,
// with loads of help from Thomas "Karroffel" Herzog

#include "godot_oculus.h"

void GDN_EXPORT godot_oculus_gdnative_singleton() {
	if (xr_api != NULL) {
		xr_api->godot_arvr_register_interface(&interface_struct);
	}
}

void GDN_EXPORT godot_oculus_nativescript_init(void *p_handle) {
	if (nativescript_api == NULL) {
		return;
	}

	register_gdnative_config(p_handle);
}
