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
