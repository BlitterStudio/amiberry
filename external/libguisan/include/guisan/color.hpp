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

#ifndef GCN_COLOR_HPP
#define GCN_COLOR_HPP

#include "guisan/platform.hpp"

#include <iostream>

namespace gcn
{
    /**
     * Represents a color with red, green, blue and alpha components.
     */
    class GCN_CORE_DECLSPEC Color
    {
    public:

        /**
         * Constructor. Initializes the color to black.
         */
        Color();

        /**
         * Constructor. Constructs a color from the bytes in an integer. 
         * Call it with a hexadecimal constant for HTML-style color representation.
         * The alpha component is 255 by default.
         *
         * EXAMPLE: Color(0xff50a0) constructs a very nice pinkish color.
         *
         * NOTE: Because of this constructor, integers will be automatically
         *       casted to a color by your compiler.
         *
         * @param color The color to initialise the object with.
         */
        Color(int color);

        /**
         * Constructor. The default alpha value is 255.
         *
         * @param r Red color component (range 0-255).
         * @param g Green color component  (range 0-255).
         * @param b Blue color component (range 0-255).
         * @param a Alpha, used for transparency. A value of 0 means
         *          totaly transparent, 255 is totaly opaque.
         */
        Color(int r, int g, int b, int a = 255);

        /**
         * Adds the RGB values of two colors together. The values will be
         * clamped if they go out of range. 
         *
         * WARNING: This function will reset the alpha value of the
         *          returned color to 255.
         *
         * @param color A color to add to this color.
         * @return The added colors with an alpha value set to 255.
         */
        Color operator+(const Color& color) const;

        /**
         * Subtracts the RGB values of one color from another.
         * The values will be clamped if they go out of range.
         *
         * WARNING: This function will reset the alpha value of the
         *          returned color to 255.
         *
         * @param color A color to subtract from this color.
         * @return The subtracted colors with an alpha value set to 255.
         */
        Color operator-(const Color& color) const;

        /**
         * Multiplies the RGB values of a color with a float value.
         * The values will be clamped if they go out of range.
         *
         * @param value The value to multiply the color with.
         * @return The multiplied colors. The alpha value will, unlike
         *         the add and subtract operations, be multiplied as
         *         well.
         */
        Color operator*(float value) const;

        /**
         * Compares two colors.
         *
         * @return True if the two colors have the same RGBA components
         *         false otherwise.
         */
        bool operator==(const Color& color) const;

        /**
         * Compares two colors.
         *
         * @return True if the two colors have different RGBA components,
         *         false otherwise.
         */
        bool operator!=(const Color& color) const;

        /**
         * Output operator for output.
         *
         * @param out The stream to output to.
         * @param color The color to output.
         */
        friend std::ostream& operator<<(std::ostream& out, const Color& Color);

        /**
         * Holds the red color component (range 0-255).
         */
        int r;

        /**
         *  Holds the green color component (range 0-255).
         */
        int g;

        /**
         *  Holds the blue color component (range 0-255).
         */
        int b;

        /**
         * Holds the alpha color component. A value of 0 means totally
         * transparent while a value of 255 is considered opaque.
         */
        int a;
    };
}

#endif // end GCN_COLOR_HPP
