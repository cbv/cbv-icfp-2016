#include "Viz1.hpp"

#include <iostream>

#define LOG_ERROR( X ) std::cerr << X << std::endl
#define LOG_INFO( X ) std::cout << X << std::endl

Viz1::Viz1(glm::uvec2 _size) : stop_flag(false), size(_size) {

	if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) != 0) {
		LOG_ERROR("Could not initialize sdl: " << SDL_GetError());
		exit(1);
	}

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	window = SDL_CreateWindow("holes", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, size.x, size.y, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);

		
	if (!window) {
		LOG_ERROR("Could not create window: " << SDL_GetError());
		exit(1);
	}
	{
		int w = 0, h = 0;
		SDL_GetWindowSize(window, &w, &h);
		if (w != int(size.x) || h != int(size.y)) {
			LOG_ERROR("Error: requested " << size.x << "x" << size.y << " window, got " << w << "x" << h << " window.");
			exit(1);
		}
	}

	context = SDL_GL_CreateContext(window);

	if (!context) {
		LOG_ERROR("Could not create opengl context: " << SDL_GetError());
		exit(1);
	}

	LOG_INFO("OpenGL info:");
	LOG_INFO("  Vendor: " << glGetString(GL_VENDOR));
	LOG_INFO("  Renderer: " << glGetString(GL_RENDERER));
	LOG_INFO("  Version: " << glGetString(GL_VERSION));
	LOG_INFO("  Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION));


	{ //check what we got...
		int major_version, minor_version;
		SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major_version);
		SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor_version);

		LOG_INFO("  SDL says context is version " << major_version << "." << minor_version);
	}
	{ //another way...
		GLint major_version = 0, minor_version = 0;
		glGetIntegerv(GL_MAJOR_VERSION, &major_version);
		glGetIntegerv(GL_MINOR_VERSION, &minor_version);
		LOG_INFO("  glGet says version " << major_version << "." << minor_version);
	}
/*
	{
		int val = -1;
		SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &val);
		//GLint gl_val = -1;
		//glGetIntegerv(GL_STENCIL_BITS, &gl_val);
		LOG_INFO("   Stencil size: " << val << " (says SDL)"); // " << gl_val << " (says GL)");
	}
*/

	if (SDL_GL_SetSwapInterval(1) != 0) {
		LOG_ERROR("Error setting up VSYNC " << SDL_GetError() << "; will continue, but there may be tearing (or excessive resource usage)");
	}

	//------------------------------------------
	//And let's do some GL setup:

	aspect = 1.0f;
	{
		int w = 0, h = 0;
		SDL_GetWindowSize(window, &w, &h);
		glViewport(0,0,w,h);
		size.x = w;
		size.y = h;
		if (h) aspect = w / float(h);
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

Viz1::~Viz1() {

	SDL_DestroyWindow(window);
	SDL_GL_DeleteContext(context);

	SDL_Quit();
}

void Viz1::run() {
	Uint32 before = SDL_GetTicks();
	stop_flag = false;
	while (!stop_flag) {
		{ //events:
			static SDL_Event event;
			while (SDL_PollEvent(&event)) {
				if (handle_event) {
					handle_event(event);
				}
				if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
					stop_flag = true;
				}
				if (event.type == SDL_QUIT) {
					stop_flag = true;
					continue;
				}
				if (event.type == SDL_WINDOWEVENT) {
					if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
						int w = 0, h = 0;
						SDL_GetWindowSize(window, &w, &h);
						glViewport(0,0,w,h);
						size.x = w;
						size.y = h;
						if (h) aspect = w / float(h);
						continue;
					}
					continue;
				}
			}
		}
		if (stop_flag) break;

		if (update) {
			Uint32 after = SDL_GetTicks();
			update((after - before) / 1000.0f);
			before = after;
			if (stop_flag) break;
		}
		if (draw) {
			draw();
		} else {
			glClearColor(0.0f, 1.0f, 1.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);
		}

		SDL_GL_SwapWindow(window);
		
		gl_errors("Viz1::run");
	}
}
