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

#ifndef GCN_KEYINPUT_HPP
#define GCN_KEYINPUT_HPP

#include "guisan/key.hpp"
#include "guisan/platform.hpp"

namespace gcn
{
    /**
     * Internal class representing keyboard input. Generally you won't have to
     * bother using this class.
     */
    class GCN_CORE_DECLSPEC KeyInput
    {
    public:

        /**
         * Constructor.
         */
        KeyInput() { };

        /**
         * Constructor.
         *
         * @param key the Key the input concerns.
         * @param type the type of input.
         */
        KeyInput(const Key& key, int type);

        /**
         * Sets the input type.
         *
         * @param type the type of input.
         */
        void setType(int type);

        /**
         * Gets the input type.
         *
         * @return the input type.
         */
        int getType() const;

        /**
         * Sets the key the input concerns.
         *
         * @param key the Key the input concerns.
         */
        void setKey(const Key& key);

        /**
         * Gets the key the input concerns.
         *
         * @return the Key the input concerns.
         */
        const Key& getKey() const;

        /**
         * Checks whether shift is pressed.
         *
         * @return true if shift was pressed at the same time as the key.
         * @since 0.6.0
         */
        bool isShiftPressed() const;

        /**
         * Sets the shift pressed flag.
         *
         * @param pressed the shift flag value.
         * @since 0.6.0
         */
        void setShiftPressed(bool pressed);

        /**
         * Checks whether control is pressed.
         *
         * @return true if control was pressed at the same time as the key.
         * @since 0.6.0
         */
        bool isControlPressed() const;

        /**
         * Sets the control pressed flag.
         *
         * @param pressed the control flag value.
         * @since 0.6.0
         */
        void setControlPressed(bool pressed);

        /**
         * Checks whether alt is pressed.
         *
         * @return true if alt was pressed at the same time as the key.
         * @since 0.6.0
         */
        bool isAltPressed() const;

        /**
         * Sets the alt pressed flag.
         *
         * @param pressed the alt flag value.
         * @since 0.6.0
         */
        void setAltPressed(bool pressed);

        /**
         * Checks whether meta is pressed.
         *
         * @return true if meta was pressed at the same time as the key.
         * @since 0.6.0
         */
        bool isMetaPressed() const;

        /**
         * Sets the meta pressed flag.
         *
         * @param pressed the meta flag value.
         * @since 0.6.0
         */
        void setMetaPressed(bool pressed);

        /**
         * Checks whether the key was pressed at the numeric pad.
         *
         * @return true if key pressed at the numeric pad.
         * @since 0.6.0
         */
        bool isNumericPad() const;

        /**
         * Sets the numeric pad flag.
         *
         * @param numpad the numeric pad flag value.
         * @since 0.6.0
         */
        void setNumericPad(bool numpad);

        /**
         * Key input types. This enum corresponds to the enum with event
         * types on KeyEvent for easy mapping.
         */
        enum
        {
            PRESSED = 0,
            RELEASED
        };

    protected:
        Key mKey;
        int mType;
        int mButton;
        bool mShiftPressed;
        bool mControlPressed;
        bool mAltPressed;
        bool mMetaPressed;
        bool mNumericPad;
    };
}

#endif // end GCN_KEYINPUT_HPP
