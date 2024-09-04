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

#ifndef GCN_INPUTEVENT_HPP
#define GCN_INPUTEVENT_HPP

#include "guisan/event.hpp"
#include "guisan/platform.hpp"

namespace gcn
{
    /**
     * Base class for all events concerning input.
     *
     * @author Olof Naessén
     * @since 0.6.0
     */
    class GCN_CORE_DECLSPEC InputEvent: public Event
    {
    public:

        /**
         * Constructor.
         *
         * @param source The widget the event concerns.
         * @param distributor The distributor of the event.
         * @param isShiftPressed True if shift is pressed, false otherwise.
         * @param isControlPressed True if control is pressed, false otherwise.
         * @param isAltPressed True if alt is pressed, false otherwise.
         * @param isMetaPressed True if meta is pressed, false otherwise.
         */
        InputEvent(Widget* source,
                   Widget* distributor,
                   bool isShiftPressed,
                   bool isControlPressed,
                   bool isAltPressed,
                   bool isMetaPressed);

        /**
         * Checks if shift is pressed.
         *
         * @return True if shift was pressed at the same time as the key,
         *         false otherwise.
         */
        bool isShiftPressed() const;

        /**
         * Checks if control is pressed.
         *
         * @return True if control was pressed at the same time as the key,
         *         false otherwise.
         */
        bool isControlPressed() const;

        /**
         * Checks if alt is pressed.
         *
         * @return True if alt was pressed at the same time as the key,
         *         false otherwise.
         */
        bool isAltPressed() const;

        /**
         * Checks whether meta is pressed.
         *
         * @return True if meta was pressed at the same time as the key,
         *         false otherwise.
         */
        bool isMetaPressed() const;

        /**
         * Marks the event as consumed. Input event listeners may discard
         * consumed input or act on consumed input. An example of a widget
         * that discards consumed input is the ScrollArea widget that
         * discards consumed mouse wheel events so the ScrollArea will not
         * scroll if for instance a Slider's value inside the ScrollArea was
         * changed with the mouse wheel.
         *
         * @see isConsumed
         */
        void consume();

        /**
         * Checks if the input event is consumed.
         *
         * @return True if the input event is consumed, 
         *         false otherwise.
         * @see consume
         */
        bool isConsumed() const;

        /**
         * Gets the distributor of the event.
         * The function is used to tell which widget actually distributed the
         * event. As input events bubbles up, the source of the event
         * may not be the same as the distributor of the event.
         */
        Widget* getDistributor() const;

    protected:
        /**
         * True if shift is pressed, false otherwise.
         */
        bool mShiftPressed;

        /**
         * True if control is pressed, false otherwise.
         */
        bool mControlPressed;

        /**
         * True if alt is pressed, false otherwise.
         */
        bool mAltPressed;

        /**
         * True if meta is pressed, false otherwise.
         */
        bool mMetaPressed;

        /**
         * True if the input event is consumed,
         * false otherwise.
         */
        bool mIsConsumed;

        /**
         * Holds the distributor of the event.
         */
        Widget* mDistributor;

        /**
         * Gui is a friend of this class in order to be able to manipulate
         * the protected member variables of this class and at the same time
         * keep the MouseEvent class as const as possible. Gui needs to
         * update the distributer of this class whenever the distributer
         * changes as events bubble up.
         */
        friend class Gui;
    };
}

#endif // end GCN_INPUTEVENT_HPP
