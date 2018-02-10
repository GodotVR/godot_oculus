/**
  simple blit shader
**/

#ifndef BLIT_SHADER_H
#define BLIT_SHADER_H

#include <glad/glad.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct blit_shader {
	GLuint program = 0;
	GLuint vao = 0;
	GLuint vbo = 0;
} blit_shader;

#ifdef __cplusplus
extern "C" {
#endif

void blit_render(blit_shader *p_blit_shader, GLuint p_texture_id);
blit_shader blit_shader_init();
void blit_shader_cleanup(blit_shader *p_blit_shader);

#ifdef __cplusplus
}
#endif

#endif /* BLIT_SHADER_H */