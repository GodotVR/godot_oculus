////////////////////////////////////////////////////////////////////////////
// Oculus GDNative module for Godot
//
// Written by Bastiaan "Mux213" Olij, 
// with loads of help from Thomas "Karroffel" Herzog

#ifndef GODOT_OCULUS_H
#define GODOT_OCULUS_H

#include "GodotCalls.h"
#include "ARVRInterface.h"

// declare our public functions for our ARVR Interface
#ifdef __cplusplus
extern "C" {
#endif

void GDN_EXPORT godot_oculus_gdnative_singleton();
void GDN_EXPORT godot_oculus_nativescript_init(void *p_handle);

#ifdef __cplusplus
}
#endif


#endif /* !GODOT_OCULUS_H */
