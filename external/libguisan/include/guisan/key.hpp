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

#ifndef GCN_KEY_HPP
#define GCN_KEY_HPP

#include "guisan/platform.hpp"

namespace gcn
{
    /**
     * Represents a key or a character.
     */
    class GCN_CORE_DECLSPEC Key
    {
    public:

        /**
         * Constructor.
         *
         * @param value The ascii or enum value for the key.
         */
        Key(int value = 0);

        /**
         * Checks if a key is a character.
         *
         * @return True if the key is a letter, number or whitespace,
         *         false otherwise.
         */
        bool isCharacter() const;

        /**
         * Checks if a key is a number.
         *
         * @return True if the key is a number (0-9),
         *         false otherwise.
         */
        bool isNumber() const;

        /**
         * Checks if a key is a letter.
         *
         * @return True if the key is a letter (a-z,A-Z),
         *         false otherwise.
         */
        bool isLetter() const;

        /**
         * Checks if a key is printable.
         *
         * @return True if the key is a printable,
         *         false otherwise.
         */
        bool isPrintable() const;

        /**
         * Gets the value of the key. If an ascii value exists it
         * will be returned. Otherwise an enum value will be returned.
         *
         * @return The value of the key.
         */
        int getValue() const;

        /**
         *  Gets the char value of the key if available.
         *
         *  @return The char value of the key, the null character otherwise
         */
        char getChar() const;


        /**
         * Compares two keys.
         * 
         * @param key The key to compare this key with.
         * @return True if the keys are equal, false otherwise.
         */
        bool operator==(const Key& key) const;

        /**
         * Compares two keys.
         * 
         * @param key The key to compare this key with.
         * @return True if the keys are not equal, false otherwise.
         */
        bool operator!=(const Key& key) const;

        /**
         * An enum with key values.
         */
        enum
        {
            Space              = ' ',
            Tab                = '\t',
            Enter              = '\n',
            // Negative values, to avoid conflicts with higher character codes
            LeftAlt           = -1000,
            RightAlt,
            LeftShift,
            RightShift,
            LeftControl,
            RightControl,
            LeftMeta,
            RightMeta,
            LeftSuper,
            RightSuper,
            Insert,
            Home,
            PageUp,
            Delete,
            End,
            PageDown,
            Escape,
            CapsLock,
            Backspace,
            F1,
            F2,
            F3,
            F4,
            F5,
            F6,
            F7,
            F8,
            F9,
            F10,
            F11,
            F12,
            F13,
            F14,
            F15,
            PrintScreen,
            ScrollLock,
            Pause,
            NumLock,
            AltGr,
            Left,
            Right,
            Up,
            Down
        };

    protected:
        /**
         * Holds the value of the key. It may be an ascii value
         * or an enum value.
         */
        int mValue;
    };
}

#endif // end GCN_KEY_HPP
