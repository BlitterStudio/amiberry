#include <guisan/sdl.hpp>

void androidsdl_event(SDL_Event gui_event, gcn::SDLInput *gui_input)
{
    /*
             * Now that we are done polling and using SDL events we pass
             * the leftovers to the SDLInput object to later be handled by
             * the Gui. (This example doesn't require us to do this 'cause a
             * label doesn't use input. But will do it anyway to show how to
             * set up an SDL application with Guichan.)
             */
    if (gui_event.type == SDL_MOUSEMOTION ||
        gui_event.type == SDL_MOUSEBUTTONDOWN ||
        gui_event.type == SDL_MOUSEBUTTONUP)
    {
        // Filter emulated mouse events for Guichan, we wand absolute input
    }
    else
    {
        // Convert multitouch event to SDL mouse event
        static int x = 0, y = 0, buttons = 0, wx = 0, wy = 0, pr = 0;
        SDL_Event event2;
        memcpy(&event2, &gui_event, sizeof(gui_event));
        if (gui_event.type == SDL_JOYBALLMOTION &&
            gui_event.jball.which == 0 &&
            gui_event.jball.ball == 0)
        {
            event2.type = SDL_MOUSEMOTION;
            event2.motion.which = 0;
            event2.motion.state = buttons;
            event2.motion.xrel = gui_event.jball.xrel - x;
            event2.motion.yrel = gui_event.jball.yrel - y;
            if (gui_event.jball.xrel != 0)
            {
                x = gui_event.jball.xrel;
                y = gui_event.jball.yrel;
            }
            event2.motion.x = x;
            event2.motion.y = y;
            //__android_log_print(ANDROID_LOG_INFO, "GUICHAN","Mouse motion %d %d btns %d", x, y, buttons);
            if (buttons == 0)
            {
                // Push mouse motion event first, then button down event
                gui_input->pushInput(event2);
                buttons = SDL_BUTTON_LMASK;
                event2.type = SDL_MOUSEBUTTONDOWN;
                event2.button.which = 0;
                event2.button.button = SDL_BUTTON_LEFT;
                event2.button.state = SDL_PRESSED;
                event2.button.x = x;
                event2.button.y = y;
                //__android_log_print(ANDROID_LOG_INFO, "GUICHAN","Mouse button %d coords %d %d", buttons, x, y);
            }
        }
        if (gui_event.type == SDL_JOYBUTTONUP &&
            gui_event.jbutton.which == 0 &&
            gui_event.jbutton.button == 0)
        {
            // Do not push button down event here, because we need mouse motion event first
            buttons = 0;
            event2.type = SDL_MOUSEBUTTONUP;
            event2.button.which = 0;
            event2.button.button = SDL_BUTTON_LEFT;
            event2.button.state = SDL_RELEASED;
            event2.button.x = x;
            event2.button.y = y;
            //__android_log_print(ANDROID_LOG_INFO, "GUICHAN","Mouse button %d coords %d %d", buttons, x, y);
        }
        gui_input->pushInput(event2);
    }
}
