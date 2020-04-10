////////////////////////////////////////////////////////////////////////////////////////////////
// Just exposing an interface to the ImageTexture functions reachable in GDNative
// Basically just ported some of the functions in the cpp_bindings for use in C

#ifndef OS_H
#define OS_H

#include "support/godot_calls.h"

class OS {
private:
	static OS *singleton;
	godot_object *_os_singleton;

	godot_method_bind *mb_get_ticks_msec;
	godot_method_bind *mb_get_screen_size;
	godot_method_bind *mb_get_current_video_driver;

public:
	static OS *get_singleton();
	static void cleanup_singleton();

	OS();
	~OS();

	int64_t get_ticks_msec();
	godot_vector2 get_screen_size(const int64_t screen = -1);
	godot_int get_current_video_driver();
};

#endif /* !OS_H */
