/*      _______   __   __   __   ______   __   __   _______   __   __
 *     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\
 *    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /
 *   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /
 *  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /
 * /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /
 * \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/
 *
 * Copyright (c) 2004, 2005, 2006, 2007 Olof Naess�n and Per Larsson
 *
 *                                                         Js_./
 * Per Larsson a.k.a finalman                          _RqZ{a<^_aa
 * Olof Naess�n a.k.a jansem/yakslem                _asww7!uY`>  )\a//
 *                                                 _Qhm`] _f "'c  1!5m
 * Visit: http://guichan.darkbits.org             )Qk<P ` _: :+' .'  "{[
 *                                               .)j(] .d_/ '-(  P .   S
 * License: (BSD)                                <Td/Z <fP"5(\"??"\a.  .L
 * Redistribution and use in source and          _dV>ws?a-?'      ._/L  #'
 * binary forms, with or without                 )4d[#7r, .   '     )d`)[
 * modification, are permitted provided         _Q-5'5W..j/?'   -?!\)cam'
 * that the following conditions are met:       j<<WP+k/);.        _W=j f
 * 1. Redistributions of source code must       .$%w\/]Q  . ."'  .  mj$
 *    retain the above copyright notice,        ]E.pYY(Q]>.   a     J@\
 *    this list of conditions and the           j(]1u<sE"L,. .   ./^ ]{a
 *    following disclaimer.                     4'_uomm\.  )L);-4     (3=
 * 2. Redistributions in binary form must        )_]X{Z('a_"a7'<a"a,  ]"[
 *    reproduce the above copyright notice,       #}<]m7`Za??4,P-"'7. ).m
 *    this list of conditions and the            ]d2e)Q(<Q(  ?94   b-  LQ/
 *    following disclaimer in the                <B!</]C)d_, '(<' .f. =C+m
 *    documentation and/or other materials      .Z!=J ]e []('-4f _ ) -.)m]'
 *    provided with the distribution.          .w[5]' _[ /.)_-"+?   _/ <W"
 * 3. Neither the name of Guichan nor the      :$we` _! + _/ .        j?
 *    names of its contributors may be used     =3)= _f  (_yQmWW$#(    "
 *    to endorse or promote products derived     -   W,  sQQQQmZQ#Wwa]..
 *    from this software without specific        (js, \[QQW$QWW#?!V"".
 *    prior written permission.                    ]y:.<\..          .
 *                                                 -]n w/ '         [.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT       )/ )/           !
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY         <  (; sac    ,    '
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING,               ]^ .-  %
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF            c <   r
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR            aga<  <La
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE          5%  )P'-3L
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR        _bQf` y`..)a
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          ,J?4P'.P"_(\?d'.,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES               _Pa,)!f/<[]/  ?"
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT      _2-..:. .r+_,.. .
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     ?a.<%"'  " -'.a_ _,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION)                     ^
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * For comments regarding functions please see the header file.
 */

#include "guisan/sdl/sdlinput.hpp"

#include "guisan/exception.hpp"

namespace gcn
{
    //--------------------------------------------------------------------------
    static Uint32 utf8ToUnicode(const char* text)
    {
        const Uint32 c0 = static_cast<unsigned char>(text[0]);
        if ((c0 & 0xF8) == 0xF0) {
            if (((text[1] & 0xC0) != 0x80) && ((text[2] & 0xC0) != 0x80)
                && ((text[3] & 0xC0) != 0x80)) {
                throw GCN_EXCEPTION("invalid utf8");
            }
            const unsigned char c1 = text[1] & 0x3F;
            const unsigned char c2 = text[2] & 0x3F;
            const unsigned char c3 = text[3] & 0x3F;

            return ((c0 & 0x07) << 18) | (c1 << 12) | (c2 << 6) | c3;
        } else if ((c0 & 0xF0) == 0xE0) {
            if (((text[1] & 0xC0) != 0x80) && ((text[2] & 0xC0) != 0x80)) {
                throw GCN_EXCEPTION("invalid utf8");
            }
            const unsigned char c1 = text[1] & 0x3F;
            const unsigned char c2 = text[2] & 0x3F;

            return ((c0 & 0x0F) << 12) | (c1 << 6) | c2;
        } else if ((c0 & 0xE0) == 0xC0) {
            if (((text[1] & 0xC0) != 0x80)) {
                throw GCN_EXCEPTION("invalid utf8");
            }
            const unsigned char c1 = text[1] & 0x3F;

            return ((c0 & 0x1F) << 6) | c1;
        } else {
            if ((c0 & 0x80) != 0) {
                throw GCN_EXCEPTION("invalid utf8");
            }
            return (c0 & 0x7F);
        }
    }

    bool SDLInput::isKeyQueueEmpty()
    {
        return mKeyInputQueue.empty();
    }

    KeyInput SDLInput::dequeueKeyInput()
    {
        KeyInput keyInput;

        if (mKeyInputQueue.empty())
        {
            throw GCN_EXCEPTION("The queue is empty.");
        }

        keyInput = mKeyInputQueue.front();
        mKeyInputQueue.pop();

        return keyInput;
    }

    bool SDLInput::isMouseQueueEmpty()
    {
        return mMouseInputQueue.empty();
    }

    MouseInput SDLInput::dequeueMouseInput()
    {
        MouseInput mouseInput;

        if (mMouseInputQueue.empty())
        {
            throw GCN_EXCEPTION("The queue is empty.");
        }

        mouseInput = mMouseInputQueue.front();
        mMouseInputQueue.pop();

        return mouseInput;
    }

    void SDLInput::pushInput(SDL_Event event)
    {
        KeyInput keyInput;
        MouseInput mouseInput;

        switch (event.type)
        {
          case SDL_TEXTINPUT:
              keyInput.setKey(utf8ToUnicode(event.text.text));
              keyInput.setType(KeyInput::Pressed);
              keyInput.setShiftPressed(false);
              keyInput.setControlPressed(false);
              keyInput.setAltPressed(false);
              keyInput.setMetaPressed(false);
              keyInput.setNumericPad(false);

              mKeyInputQueue.push(keyInput);
              break;
          case SDL_KEYDOWN:
              keyInput.setKey(convertSDLEventToGuichanKeyValue(event));
              keyInput.setType(KeyInput::Pressed);
              keyInput.setShiftPressed(event.key.keysym.mod & KMOD_SHIFT);
              keyInput.setControlPressed(event.key.keysym.mod & KMOD_CTRL);
              keyInput.setAltPressed(event.key.keysym.mod & KMOD_ALT);
              keyInput.setMetaPressed(event.key.keysym.mod & KMOD_GUI);
              keyInput.setNumericPad(event.key.keysym.sym >= SDLK_KP_0
                                     && event.key.keysym.sym <= SDLK_KP_EQUALS);

              if (!keyInput.getKey().isPrintable() || keyInput.isAltPressed()
                  || keyInput.isControlPressed())
              {
                  mKeyInputQueue.push(keyInput);
              }
              break;

          case SDL_KEYUP:
              keyInput.setKey(convertSDLEventToGuichanKeyValue(event));
              keyInput.setType(KeyInput::Released);
              keyInput.setShiftPressed(event.key.keysym.mod & KMOD_SHIFT);
              keyInput.setControlPressed(event.key.keysym.mod & KMOD_CTRL);
              keyInput.setAltPressed(event.key.keysym.mod & KMOD_ALT);
              keyInput.setMetaPressed(event.key.keysym.mod & KMOD_GUI);
              keyInput.setNumericPad(event.key.keysym.sym >= SDLK_KP_0
                                     && event.key.keysym.sym <= SDLK_KP_EQUALS);

              mKeyInputQueue.push(keyInput);
              break;

          case SDL_MOUSEBUTTONDOWN:
              mMouseDown = true;
              mouseInput.setX(event.button.x);
              mouseInput.setY(event.button.y);
              mouseInput.setButton(convertMouseButton(event.button.button));
              mouseInput.setType(MouseInput::Pressed);
              mouseInput.setTimeStamp(SDL_GetTicks());
              mMouseInputQueue.push(mouseInput);
              break;

          case SDL_MOUSEBUTTONUP:
              mMouseDown = false;
              mouseInput.setX(event.button.x);
              mouseInput.setY(event.button.y);
              mouseInput.setButton(convertMouseButton(event.button.button));
              mouseInput.setType(MouseInput::Released);
              mouseInput.setTimeStamp(SDL_GetTicks());
              mMouseInputQueue.push(mouseInput);
              break;

          case SDL_MOUSEMOTION:
              mouseInput.setX(event.button.x);
              mouseInput.setY(event.button.y);
              mouseInput.setButton(MouseInput::Empty);
              mouseInput.setType(MouseInput::Moved);
              mouseInput.setTimeStamp(SDL_GetTicks());
              mMouseInputQueue.push(mouseInput);
              break;

          case SDL_MOUSEWHEEL:
              if (event.wheel.y > 0)
                  mouseInput.setType(MouseInput::WheelMovedUp);
              else
                  mouseInput.setType(MouseInput::WheelMovedDown);
              break;

          case SDL_WINDOWEVENT:
              /*
               * This occurs when the mouse leaves the window and the Gui-chan
               * application loses its mousefocus.
               */
              if (event.window.event & SDL_WINDOWEVENT_LEAVE)
              {
                  mMouseInWindow = false;

                  if (!mMouseDown)
                  {
                      mouseInput.setX(-1);
                      mouseInput.setY(-1);
                      mouseInput.setButton(MouseInput::Empty);
                      mouseInput.setType(MouseInput::Moved);
                      mMouseInputQueue.push(mouseInput);
                  }
              }

              if (event.window.event & SDL_WINDOWEVENT_ENTER)
              {
                  mMouseInWindow = true;
              }
              break;
        } // end switch
    }

    int SDLInput::convertMouseButton(int button)
    {
        switch (button)
        {
          case SDL_BUTTON_LEFT:
              return MouseInput::Left;
              break;
          case SDL_BUTTON_RIGHT:
              return MouseInput::Right;
              break;
          case SDL_BUTTON_MIDDLE:
              return MouseInput::Middle;
              break;
          default:
              // We have an unknown mouse type which is ignored.
              return button;
        }
    }

    Key SDLInput::convertSDLEventToGuichanKeyValue(SDL_Event event)
    {
        int value = -1;

        switch (event.key.keysym.sym)
        {
          case SDLK_TAB:
              value = Key::Tab;
              break;
          case SDLK_LALT:
              value = Key::LeftAlt;
              break;
          case SDLK_RALT:
              value = Key::RightAlt;
              break;
          case SDLK_LSHIFT:
              value = Key::LeftShift;
              break;
          case SDLK_RSHIFT:
              value = Key::RightShift;
              break;
          case SDLK_LCTRL:
              value = Key::LeftControl;
              break;
          case SDLK_RCTRL:
              value = Key::RightControl;
              break;
          case SDLK_BACKSPACE:
              value = Key::Backspace;
              break;
          case SDLK_PAUSE:
              value = Key::Pause;
              break;
          case SDLK_SPACE:
              value = Key::Space;
              break;
          case SDLK_ESCAPE:
              value = Key::Escape;
              break;
          case SDLK_DELETE:
              value = Key::Delete;
              break;
          case SDLK_INSERT:
              value = Key::Insert;
              break;
          case SDLK_HOME:
              value = Key::Home;
              break;
          case SDLK_END:
              value = Key::End;
              break;
          case SDLK_PAGEUP:
              value = Key::PageUp;
              break;
          case SDLK_PRINTSCREEN:
              value = Key::PrintScreen;
              break;
          case SDLK_PAGEDOWN:
              value = Key::PageDown;
              break;
          case SDLK_F1:
              value = Key::F1;
              break;
          case SDLK_F2:
              value = Key::F2;
              break;
          case SDLK_F3:
              value = Key::F3;
              break;
          case SDLK_F4:
              value = Key::F4;
              break;
          case SDLK_F5:
              value = Key::F5;
              break;
          case SDLK_F6:
              value = Key::F6;
              break;
          case SDLK_F7:
              value = Key::F7;
              break;
          case SDLK_F8:
              value = Key::F8;
              break;
          case SDLK_F9:
              value = Key::F9;
              break;
          case SDLK_F10:
              value = Key::F10;
              break;
          case SDLK_F11:
              value = Key::F11;
              break;
          case SDLK_F12:
              value = Key::F12;
              break;
          case SDLK_F13:
              value = Key::F13;
              break;
          case SDLK_F14:
              value = Key::F14;
              break;
          case SDLK_F15:
              value = Key::F15;
              break;
          case SDLK_NUMLOCKCLEAR:
              value = Key::NumLock;
              break;
          case SDLK_CAPSLOCK:
              value = Key::CapsLock;
              break;
          case SDLK_SCROLLLOCK:
              value = Key::ScrollLock;
              break;
          case SDLK_RGUI:
              value = Key::RightMeta;
              break;
          case SDLK_LGUI:
              value = Key::LeftMeta;
              break;
          case SDLK_MODE:
              value = Key::AltGr;
              break;
          case SDLK_UP:
              value = Key::Up;
              break;
          case SDLK_DOWN:
              value = Key::Down;
              break;
          case SDLK_LEFT:
              value = Key::Left;
              break;
          case SDLK_RIGHT:
              value = Key::Right;
              break;
          case SDLK_RETURN:
              value = Key::Enter;
              break;
          case SDLK_KP_ENTER:
              value = Key::Enter;
              break;

          default:
              value = event.key.keysym.sym;
              break;
        }

        if (!(event.key.keysym.mod & KMOD_NUM))
        {
            switch (event.key.keysym.sym)
            {
              case SDLK_KP_0:
                  value = Key::Insert;
                  break;
              case SDLK_KP_1:
                  value = Key::End;
                  break;
              case SDLK_KP_2:
                  value = Key::Down;
                  break;
              case SDLK_KP_3:
                  value = Key::PageDown;
                  break;
              case SDLK_KP_4:
                  value = Key::Left;
                  break;
              case SDLK_KP_5:
                  value = 0;
                  break;
              case SDLK_KP_6:
                  value = Key::Right;
                  break;
              case SDLK_KP_7:
                  value = Key::Home;
                  break;
              case SDLK_KP_8:
                  value = Key::Up;
                  break;
              case SDLK_KP_9:
                  value = Key::PageUp;
                  break;
              default:
                  break;
            }
        }
        else
        {
            switch (event.key.keysym.sym)
            {
              case SDLK_KP_0:
                  value = SDLK_0;
                  break;
              case SDLK_KP_1:
                  value = SDLK_1;
                  break;
              case SDLK_KP_2:
                  value = SDLK_2;
                  break;
              case SDLK_KP_3:
                  value = SDLK_3;
                  break;
              case SDLK_KP_4:
                  value = SDLK_4;
                  break;
              case SDLK_KP_5:
                  value = SDLK_5;
                  break;
              case SDLK_KP_6:
                  value = SDLK_6;
                  break;
              case SDLK_KP_7:
                  value = SDLK_7;
                  break;
              case SDLK_KP_8:
                  value = SDLK_8;
                  break;
              case SDLK_KP_9:
                  value = SDLK_9;
                  break;
              default:
                  break;
            }
        }

        return value;
    }
}
