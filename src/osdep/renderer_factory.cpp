/*
 * renderer_factory.cpp - Renderer creation factory
 *
 * Copyright 2026 Dimitris Panokostas
 */

#include "sysdeps.h"
#include "renderer_factory.h"

#ifdef USE_VULKAN
#include "vulkan_renderer.h"
#elif defined(USE_OPENGL)
#include "opengl_renderer.h"
#endif
#include "sdl_renderer.h"

std::unique_ptr<IRenderer> create_sdl_renderer()
{
	return std::make_unique<SDLRenderer>();
}

std::unique_ptr<IRenderer> create_renderer()
{
#ifdef USE_VULKAN
	return std::make_unique<VulkanRenderer>();
#elif defined(USE_OPENGL)
	return std::make_unique<OpenGLRenderer>();
#else
	return create_sdl_renderer();
#endif
}
