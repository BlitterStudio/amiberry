/*      _______   __   __   __   ______   __   __   _______   __   __
 *     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\
 *    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /
 *   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /
 *  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /
 * /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /
 * \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/
 *
 * Copyright (c) 2004, 2005, 2006, 2007 Olof Naessén and Per Larsson
 *
 *                                                         Js_./
 * Per Larsson a.k.a finalman                          _RqZ{a<^_aa
 * Olof Naessén a.k.a jansem/yakslem                _asww7!uY`>  )\a//
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

#ifndef GCN_GUISAN_HPP
#define GCN_GUISAN_HPP

#include <guisan/actionevent.hpp>
#include <guisan/actionlistener.hpp>
#include <guisan/cliprectangle.hpp>
#include <guisan/color.hpp>
#include <guisan/deathlistener.hpp>
#include <guisan/event.hpp>
#include <guisan/exception.hpp>
#include <guisan/focushandler.hpp>
#include <guisan/focuslistener.hpp>
#include <guisan/font.hpp>
#include <guisan/genericinput.hpp>
#include <guisan/graphics.hpp>
#include <guisan/gui.hpp>
#include <guisan/image.hpp>
#include <guisan/imagefont.hpp>
#include <guisan/imageloader.hpp>
#include <guisan/input.hpp>
#include <guisan/inputevent.hpp>
#include <guisan/key.hpp>
#include <guisan/keyevent.hpp>
#include <guisan/keyinput.hpp>
#include <guisan/keylistener.hpp>
#include <guisan/listmodel.hpp>
#include <guisan/mouseevent.hpp>
#include <guisan/mouseinput.hpp>
#include <guisan/mouselistener.hpp>
#include <guisan/rectangle.hpp>
#include <guisan/selectionevent.hpp>
#include <guisan/selectionlistener.hpp>
#include <guisan/widget.hpp>
#include <guisan/widgetlistener.hpp>

#include <guisan/widgets/button.hpp>
#include <guisan/widgets/checkbox.hpp>
#include <guisan/widgets/container.hpp>
#include <guisan/widgets/dropdown.hpp>
#include <guisan/widgets/icon.hpp>
#include <guisan/widgets/imagebutton.hpp>
#include <guisan/widgets/imagetextbutton.hpp>
#include <guisan/widgets/inputbox.hpp>
#include <guisan/widgets/label.hpp>
#include <guisan/widgets/listbox.hpp>
#include <guisan/widgets/messagebox.hpp>
#include <guisan/widgets/progressbar.hpp>
#include <guisan/widgets/scrollarea.hpp>
#include <guisan/widgets/slider.hpp>
#include <guisan/widgets/radiobutton.hpp>
#include <guisan/widgets/tab.hpp>
#include <guisan/widgets/tabbedarea.hpp>
#include <guisan/widgets/textbox.hpp>
#include <guisan/widgets/textfield.hpp>
#include <guisan/widgets/togglebutton.hpp>
#include <guisan/widgets/window.hpp>

#include "guisan/platform.hpp"

class Widget;

extern "C" {
/**
 * Gets the the version of Guisan. As it is a C function
 * it can be used to check for Guichan with autotools.
 *
 * @return the version of Guisan.
 */
GCN_CORE_DECLSPEC extern const char* gcnGuisanVersion();
}

#endif // end GCN_GUISAN_HPP
