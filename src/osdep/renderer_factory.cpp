/*
 * renderer_factory.cpp - Renderer creation factory
 *
 * Copyright 2026 Dimitris Panokostas
 */

#include "sysdeps.h"
#include "renderer_factory.h"

#ifdef USE_OPENGL
#include "opengl_renderer.h"
#else
#include "sdl_renderer.h"
#endif

std::unique_ptr<IRenderer> create_renderer()
{
#ifdef USE_OPENGL
	return std::make_unique<OpenGLRenderer>();
#else
	return std::make_unique<SDLRenderer>();
#endif
}
