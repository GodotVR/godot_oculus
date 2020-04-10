////////////////////////////////////////////////////////////////////////////
// Oculus GDNative module for Godot
//
// Written by Bastiaan "Mux213" Olij,
// with loads of help from Thomas "Karroffel" Herzog

#ifndef GODOT_OCULUS_H
#define GODOT_OCULUS_H

#include "oculus/oculus_config.h"
#include "support/godot_calls.h"
#include "xr_interface.h"

// declare our public functions for our XR Interface
#ifdef __cplusplus
extern "C" {
#endif

void GDN_EXPORT godot_oculus_gdnative_singleton();
void GDN_EXPORT godot_oculus_nativescript_init(void *p_handle);

#ifdef __cplusplus
}
#endif

#endif /* !GODOT_OCULUS_H */
