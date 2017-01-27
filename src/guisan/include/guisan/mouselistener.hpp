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

#ifndef GCN_MOUSELISTENER_HPP
#define GCN_MOUSELISTENER_HPP

#include "guisan/mouseevent.hpp"
#include "guisan/platform.hpp"

namespace gcn
{
    /**
     * Mouse listeners base class. Inorder to use this class you must inherit
     * from it and implements it's functions. MouseListeners listen for mouse
     * events on a Widgets. When a Widget recives a mouse event, the
     * corresponding function in all it's mouse listeners will be called.
     *
     * @see Widget::addMouseListener
     */
    class GCN_CORE_DECLSPEC MouseListener
    {
    public:

        /**
         * Destructor.
         */
        virtual ~MouseListener() { }

        /**
         * Called when the mouse has entered into the widget area.
         *
         * @param mouseEvent describes the event.
         * @since 0.6.0
         */
        virtual void mouseEntered(MouseEvent& mouseEvent)
        {

        }

        /**
         * Called when the mouse has exited the widget area.
         *
         * @param mouseEvent describes the event.
         */
        virtual void mouseExited(MouseEvent& mouseEvent)
        {

        }

        /**
         * Called when a mouse button has been pressed on the widget area.
         *
         * NOTE: A mouse press is NOT equal to a mouse click.
         *       Use mouseClickMessage to check for mouse clicks.
         *
         * @param mouseEvent describes the event.
         * @since 0.6.0
         */
        virtual void mousePressed(MouseEvent& mouseEvent)
        {

        }

        /**
         * Called when a mouse button has been released on the widget area.
         *
         * @param mouseEvent describes the event.
         */
        virtual void mouseReleased(MouseEvent& mouseEvent)
        {

        }

        /**
         * Called when a mouse button is pressed and released (clicked) on
         * the widget area.
         *
         * @param mouseEvent describes the event.
         * @since 0.6.0
         */
        virtual void mouseClicked(MouseEvent& mouseEvent)
        {

        }

        /**
         * Called when the mouse wheel has moved up on the widget area.
         *
         * @param mouseEvent describes the event.
         * @since 0.6.0
         */
        virtual void mouseWheelMovedUp(MouseEvent& mouseEvent)
        {

        }

        /**
         * Called when the mouse wheel has moved down on the widget area.
         *
         * @param mousEvent describes the event.
         * @since 0.6.0
         */
        virtual void mouseWheelMovedDown(MouseEvent& mouseEvent)
        {

        }

        /**
         * Called when the mouse has moved in the widget area and no mouse button
         * has been pressed (i.e no widget is being dragged).
         *
         * @param mouseEvent describes the event.
         * @since 0.6.0
         */
        virtual void mouseMoved(MouseEvent& mouseEvent)
        {

        }

        /**
         * Called when the mouse has moved and the mouse has previously been
         * pressed on the widget.
         *
         * @param mouseEvent describes the event.
         * @since 0.6.0
         */
        virtual void mouseDragged(MouseEvent& mouseEvent)
        {

        }

    protected:
        /**
         * Constructor.
         *
         * You should not be able to make an instance of MouseListener,
         * therefore its constructor is protected. To use MouseListener
         * you must inherit from this class and implement it's
         * functions.
         */
        MouseListener() { }
    };
}

#endif // end GCN_MOUSELISTENER_HPP
