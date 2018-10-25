/**
  simple blit shader
**/

#ifndef BLIT_SHADER_H
#define BLIT_SHADER_H

#include <glad/glad.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

class blit_shader {
private:
	GLuint program = 0;
	GLuint vao = 0;
	GLuint vbo = 0;

	static float vertice_data[12];

	void compile_shader(GLuint shader, const char *src);
	bool link_shader();
public:
	blit_shader();
	~blit_shader();

	void render(GLuint p_texture_id);
};

#endif /* BLIT_SHADER_H */