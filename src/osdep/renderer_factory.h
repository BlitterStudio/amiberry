#pragma once

/*
 * renderer_factory.h - Renderer creation factory
 *
 * Creates the appropriate IRenderer implementation based on build configuration.
 *
 * Copyright 2026 Dimitris Panokostas
 */

#include <memory>
#include "irenderer.h"

// Creates the renderer matching the current build configuration:
// - USE_VULKAN defined: returns VulkanRenderer
// - USE_OPENGL defined: returns OpenGLRenderer
// - Otherwise: returns SDLRenderer
std::unique_ptr<IRenderer> create_renderer();

// Creates the SDL software renderer backend. Used as the primary renderer in
// no-GL builds and as the runtime demotion target when GL context creation
// fails in USE_OPENGL builds (see gfx_window.cpp).
std::unique_ptr<IRenderer> create_sdl_renderer();
