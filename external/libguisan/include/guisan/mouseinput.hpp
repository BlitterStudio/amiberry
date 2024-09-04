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

#ifndef GCN_MOUSEINPUT_HPP
#define GCN_MOUSEINPUT_HPP

#include "guisan/platform.hpp"

namespace gcn
{

    /**
     * Internal class that represents mouse input. Generally you won't have to
     * bother using this class unless you implement an Input class for
     * a back end.
     *
     * @author Olof Naessén
     * @author Per Larsson
     * @since 0.1.0
     */
    class GCN_CORE_DECLSPEC MouseInput
    {
    public:

        /**
         * Constructor.
         */
        MouseInput() { };

        /**
         * Constructor.
         *
         * @param button The button pressed.
         * @param type The type of mouse input.
         * @param x The mouse x coordinate.
         * @param y The mouse y coordinate.
         * @param timeStamp The timestamp of the mouse input. Used to
         *                  check for double clicks.
         */
        MouseInput(unsigned int button,
                   unsigned int type,
                   int x,
                   int y,
                   int timeStamp);

        /**
         * Sets the type of the mouse input.
         *
         * @param type The type of the mouse input. Should be a value from the
         *             mouse event type enum
         * @see getType
         * @since 0.1.0
         */
        void setType(unsigned int type);

        /**
         * Gets the type of the mouse input.
         *
         * @return The type of the mouse input. A value from the mouse event
         *         type enum.
         * @see setType
         * @since 0.1.0
         */
        unsigned int getType() const;

        /**
         * Sets the button pressed.
         *
         * @param button The button pressed. Should be one of the values
         *               in the mouse event button enum.
         * @see getButton.
         * @since 0.1.0
         */
        void setButton(unsigned int button);

        /**
         * Gets the button pressed.
         *
         * @return The button pressed. A value from the mouse event
         *         button enum.
         * @see setButton
         * @since 0.1.0
         */
        unsigned int getButton() const;

        /**
         * Sets the timestamp for the mouse input.
         * Used to check for double clicks.
         *
         * @param timeStamp The timestamp of the mouse input.
         * @see getTimeStamp
         * @since 0.1.0
         */
        void setTimeStamp(int timeStamp);

        /**
         * Gets the time stamp of the input.
         * Used to check for double clicks.
         *
         * @return The time stamp of the mouse input.
         * @see setTimeStamp
         * @since 0.1.0
         */
        int getTimeStamp() const;

        /**
         * Sets the x coordinate of the mouse input.
         *
         * @param x The x coordinate of the mouse input.
         * @see getX
         * @since 0.6.0
         */
        void setX(int x);

        /**
         * Gets the x coordinate of the mouse input.
         *
         * @return The x coordinate of the mouse input.
         * @see setX
         * @since 0.6.0
         */
        int getX() const;

        /**
         * Sets the y coordinate of the mouse input.
         *
         * @param y The y coordinate of the mouse input.
         * @see getY
         * @since 0.6.0
         */
        void setY(int y);

        /**
         * Gets the y coordinate of the mouse input.
         *
         * @return The y coordinate of the mouse input.
         * @see setY
         * @since 0.6.0
         */
        int getY() const;

        /**
         * Mouse input event types. This enum partially corresponds
         * to the enum with event types in MouseEvent for easy mapping.
         */
        enum
        {
            Moved = 0,
            Pressed,
            Released,
            WheelMovedDown,
            WheelMovedUp
        };

        /**
         * Mouse button types.
         */
        enum
        {
            Empty = 0,
            Left,
            Right,
            Middle
        };

    protected:
        /**
         * Holds the type of the mouse input.
         */
        unsigned int mType;

        /**
         * Holds the button of the mouse input.
         */
        unsigned int mButton;

        /** 
         * Holds the timestamp of the mouse input. Used to 
         * check for double clicks.
         */
        int mTimeStamp;

        /** 
         * Holds the x coordinate of the mouse input.
         */
        int mX;

        /** 
         * Holds the y coordinate of the mouse input.
         */
        int mY;
    };
}

#endif // end GCN_MOUSEINPUT_HPP
