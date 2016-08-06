#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <glm/glm.hpp>

#include <functional>
#include <iostream>

struct Viz1 {
	Viz1(glm::uvec2 size);
	~Viz1();

	void run();

	bool stop_flag;

	std::function< void(SDL_Event const &) > handle_event;
	std::function< void(float) > update;
	std::function< void() > draw;

	float aspect;
	glm::uvec2 size;

	SDL_Window *window;
	SDL_GLContext context;
};

inline bool gl_errors(std::string const &where) {
	GLuint err;
	bool had_errors = false;
	while ((err = glGetError()) != GL_NO_ERROR) {
		std::cerr << "(in " << where << ") OpenGL error ";
		if (err == GL_INVALID_ENUM) {
			std::cerr << "INVALID_ENUM";
		} else if (err == GL_INVALID_VALUE) {
			std::cerr << "INVALID_VALUE";
		} else if (err == GL_INVALID_OPERATION) {
			std::cerr << "INVALID_OPERATION";
		} else if (err == GL_INVALID_FRAMEBUFFER_OPERATION) {
			std::cerr << "INVALID_FRAMEBUFFER_OPERATION";
		} else if (err == GL_OUT_OF_MEMORY) {
			std::cerr << "OUT_OF_MEMORY";
#ifndef USE_GLES
		} else if (err == GL_STACK_UNDERFLOW) {
			std::cerr << "STACK_UNDERFLOW";
		} else if (err == GL_STACK_OVERFLOW) {
			std::cerr << "STACK_OVERFLOW";
#endif //USE_GLES
		} else {
			std::cerr << "0x" << std::hex << err;
		}
		std::cerr << "\n";
		had_errors = true;
	}
	if (had_errors) {
		std::cerr.flush();
	}
	return had_errors;
}
