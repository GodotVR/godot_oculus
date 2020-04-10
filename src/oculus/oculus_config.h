////////////////////////////////////////////////////////////////////////////////////////////////
// gdnative script to access configuration info

#ifndef OCULUS_CONFIG_H
#define OCULUS_CONFIG_H

#include "oculus/oculus_session.h"
#include "support/godot_calls.h"

typedef struct config_data_struct {
	int dummy;
} config_data_struct;

#ifdef __cplusplus
extern "C" {
#endif

// Registers the OculusConfig class and functions to GDNative.
// This methhod should be called from godot_oculus_nativescript_init.
void register_gdnative_config(void *handle);

#ifdef __cplusplus
}
#endif

#endif /* !OCULUS_CONFIG_H */
