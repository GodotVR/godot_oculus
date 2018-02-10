#include "blit_shader.h"

const char *const blit_vertex_shader =
		"#version 330\n"
		"\n"
		"layout (location=0) in vec2 coords;"
		"out vec2 T;"
		"\n"
		"void main(void)\n"
		"{\n"
		"T = coords;\n"
		"gl_Position = vec4((coords * 2.0) - 1.0, 0.0, 1.0);\n"
		"}";

const char *const blit_fragment_shader =
		"#version 330\n"
		"\n"
		"uniform sampler2D hdr_texture;\n"
		"\n"
		"in vec2 T;\n"
		"out vec4 color;\n"
		"\n"
		"void main()\n"
		"{\n"
		"color = texture(hdr_texture, T);\n"
		"}";

float blit_vertice_data[12] = {
	0.0, 1.0,
	1.0, 0.0,
	0.0, 0.0,
	0.0, 1.0,
	1.0, 1.0,
	1.0, 0.0
};

void blit_shader_compile_shader_src(GLuint shader, const char *src) {
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);

	GLint status;
	GLint length;
	char log[4096] = { 0 };

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	glGetShaderInfoLog(shader, 4096, &length, log);
	if (status == GL_FALSE) {
		printf("Compile failed %s\n", log);
	};
};

bool blit_shader_link_shader(blit_shader *p_blit_shader) {
	printf("Compiling blit shader\n");

	// Create the handels
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	p_blit_shader->program = glCreateProgram();

	// Attach the shaders to a program handel.
	glAttachShader(p_blit_shader->program, vertexShader);
	glAttachShader(p_blit_shader->program, fragmentShader);

	// Load and compile the Vertex Shader
	blit_shader_compile_shader_src(vertexShader, blit_vertex_shader);

	// Load and compile the Fragment Shader
	blit_shader_compile_shader_src(fragmentShader, blit_fragment_shader);

	printf("Linking blit shaders\n");
	glLinkProgram(p_blit_shader->program);

	// The shader objects are not needed any more,
	// the shader_program is the complete shader to be used.
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	GLint status;
	GLint length;
	char log[4096] = { 0 };

	glGetProgramiv(p_blit_shader->program, GL_LINK_STATUS, &status);
	glGetProgramInfoLog(p_blit_shader->program, 4096, &length, log);
	if (status == GL_FALSE) {
		printf("Link failed %s\n", log);

		glDeleteProgram(p_blit_shader->program);
		p_blit_shader->program = 0;
		return false;
	};

	// and set some properties that never change
	glUniform1i(glGetUniformLocation(p_blit_shader->program, "hdr_texture"), 0);
	glUseProgram(0);

	return true;
};

void blit_render(blit_shader *p_blit_shader, GLuint p_texture_id) {
	GLuint was_program;
	glGetIntegerv(GL_CURRENT_PROGRAM, (GLint *)&was_program);

	if (p_blit_shader == NULL) {
		// uuuh?
	} else if (p_blit_shader->program != 0) {
		// set our shader up
		glUseProgram(p_blit_shader->program);

		// set our texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, p_texture_id);

		// bind our vao to restore our state
		glBindVertexArray(p_blit_shader->vao);

		// render our rectangle
		glDrawArrays(GL_TRIANGLES, 0, 3 * 2);

		// and unbind
		glBindVertexArray(0);
		glUseProgram(was_program);
	}
};

blit_shader blit_shader_init() {
	blit_shader new_shader;

	// Create our shader program
	if (blit_shader_link_shader(&new_shader)) {
		// Need a Vertex Array Object
		glGenVertexArrays(1, &new_shader.vao);

		// Bind our VAO, all relevant state changes are bound to our VAO, will be unset when we unbind, and reset when we bind...
		glBindVertexArray(new_shader.vao);

		// Need a Vertex Buffer Object
		glGenBuffers(1, &new_shader.vbo);

		// Now bind our Vertex Buffer Object and load up some data!
		glBindBuffer(GL_ARRAY_BUFFER, new_shader.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, blit_vertice_data, GL_STATIC_DRAW);

		// And setup our attributes
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (GLvoid *)0);

		// and unbind our vao to return back to our old state
		glBindVertexArray(0);
	} else {
		new_shader.vao = 0;
		new_shader.vbo = 0;
	}

	return new_shader;
}

void blit_shader_cleanup(blit_shader *p_blit_shader) {
	if (p_blit_shader != NULL) {
		if (p_blit_shader->program != 0) {
			glDeleteProgram(p_blit_shader->program);
			p_blit_shader->program = 0;
		}

		if (p_blit_shader->vao != 0) {
			glDeleteVertexArrays(1, &p_blit_shader->vao);
			p_blit_shader->vao = 0;
		}

		if (p_blit_shader->vbo != 0) {
			glDeleteBuffers(1, &p_blit_shader->vbo);
			p_blit_shader->vbo = 0;
		}
	}
}

