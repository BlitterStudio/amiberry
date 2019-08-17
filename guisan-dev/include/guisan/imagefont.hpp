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

#ifndef GCN_IMAGEFONT_HPP
#define GCN_IMAGEFONT_HPP

#include <string>

#include "guisan/font.hpp"
#include "guisan/platform.hpp"
#include "guisan/rectangle.hpp"

namespace gcn
{
    class Color;
    class Graphics;
    class Image;

    /**
     * A font using an image containing the font data. It implements the font
     * class. You can use any filetype for the font data as long as it can be
     * loaded with your ImageLoader.
     *
     * This are two examples of an image containing a font.
     *  \image html imagefontexample.bmp
     *  \image html imagefontexample2.bmp
     *
     * The Image font format works like this: The first pixel, the pixal at
     * coordinate (0,0), tells which color to look for when seperating glyphs.
     * You create an image with your glyphs and simple separates them with
     * the seperation color. When you create your ImageFont you supply the
     * constructor with the glyphs present in your image. When creating an
     * ImageFont for the image data in the first example above, the following
     * constructor call would be used.
     * @code gcn::ImageFont imageFont("fixedfont_big.bmp"," abcdefghijklmno\
pqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"); @endcode
     * Noteworthy is that the first glyph actually gives the width of space.
     * Glyphs can, as seen in the second example above, be seperated with
     * horizontal lines making it possible to draw glyphs on more then one
     * line in the image. However, these vertical lines must be of one pixel
     * size!
     */
    class GCN_CORE_DECLSPEC ImageFont: public Font
    {
    public:

        /**
         * Constructor which takes an image file containing the font and
         * a string containing the glyphs. The glyphs in the string should
         * be in the same order as they appear in the font image.
         *
         * @param filename the filename of the image.
         * @param glyphs the glyphs found in the image.
         * @throws Exception when glyph list is incorrect or the font file is
         *                   corrupt or if no ImageLoader exists.
         */
        ImageFont(const std::string& filename, const std::string& glyphs);

        /**
         * Constructor which takes an image file containing the font and
         * two boundaries of ASCII values. The font image should include
         * all glyphs specified with the boundaries in increasing ASCII
         * order. The boundaries are inclusive.
         *
         * @param filename the filename of the image.
         * @param glyphsFrom the ASCII value of the first glyph found in the
         *                   image.
         * @param glyphsTo the ASCII value of the last glyph found in the
         *                 image.
         * @throws Exception when glyph bondaries are incorrect or the font
         *                   file is corrupt or if no ImageLoader exists.
         */
        ImageFont(const std::string& filename, unsigned char glyphsFrom=32,
                  unsigned char glyphsTo=126);

        /**
         * Destructor.
         */
        virtual ~ImageFont();

        /**
         * Draws a glyph.
         *
         * NOTE: You normally won't use this function to draw text since
         *       the Graphics class contains better functions for drawing
         *       text.
         *
         * @param graphics a graphics object to be used for drawing.
         * @param glyph a glyph to draw.
         * @param x the x coordinate where to draw the glyph.
         * @param y the y coordinate where to draw the glyph.
         * @return the width of the glyph in pixels.
         * @see Graphics
         */
        virtual int drawGlyph(Graphics* graphics, unsigned char glyph,
                              int x, int y);

        /**
         * Sets the spacing between rows in pixels.  Default is 0 pixels.
         * The spacing can be negative.
         *
         * @param spacing the spacing in pixels.
         */
        virtual void setRowSpacing(int spacing);

        /**
         * Gets the spacing between rows in pixels.
         *
         * @return the spacing.
         */
        virtual int getRowSpacing();

        /**
         * Sets the spacing between letters in pixels. Default is 0 pixels.
         * The spacing can be negative.
         *
         * @param spacing the spacing in pixels
         */
        virtual void setGlyphSpacing(int spacing);

        /**
         * Gets the spacing between letters in pixels.
         *
         * @return the spacing.
         */
        virtual int getGlyphSpacing();

        /**
         * Gets a width of a glyph.
         *
         * @param glyph the glyph which width will be returned
         * @return the width of a glyph
         */
        virtual int getWidth(unsigned char glyph) const;


        // Inherited from Font

        virtual int getWidth(const std::string& text) const;

        virtual void drawString(Graphics* graphics, const std::string& text,
                                int x, int y);

        virtual int getHeight() const;

        virtual int getStringIndexAt(const std::string& text, int x);

    protected:
        void addGlyph(unsigned char c, int &x, int &y, const Color& separator);

        Rectangle mGlyph[256];
        int mHeight;
        int mGlyphSpacing;
        int mRowSpacing;
        Image* mImage;
        std::string mFilename;
    };
}

#endif // end GCN_IMAGEFONT_HPP
