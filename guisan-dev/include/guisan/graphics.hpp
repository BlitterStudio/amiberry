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

#ifndef GCN_GRAPHICS_HPP
#define GCN_GRAPHICS_HPP

#include <iosfwd>
#include <stack>

#include "guisan/cliprectangle.hpp"
#include "guisan/platform.hpp"

namespace gcn
{
    class Color;
    class Font;
    class Image;

    /**
     * Used for drawing graphics. It contains all vital functions for drawing.
     * We include implemented Graphics classes for some common platforms like
     * the Allegro library, the OpenGL library and the SDL library. To make
     * Guichan usable under another platform, a Graphics class must be
     * implemented.
     *
     * In Graphics you can set clip areas to limit drawing to certain
     * areas of the screen. Clip areas are put on a stack, which means that you
     * can push smaller and smaller clip areas onto the stack. All coordinates
     * will be relative to the topmost clip area. In most cases you won't have
     * to worry about the clip areas, unless you want to implement some really
     * complex widget. Pushing and poping of clip areas are handled
     * automatically by container widgets when their child widgets are drawn.
     *
     * IMPORTANT: Remember to pop each clip area that you pushed on the stack
     * after you are done with it.
     *
     * If you feel that Graphics is to restrictive for your needs, there is
     * no one stopping you from using your own code for drawing in Widgets.
     * You could for instance use pure SDL in the drawing of Widgets bypassing
     * Graphics. This might however hurt portability of your application.
     *
     * If you implement a Graphics class not present in Guichan we would be very
     * happy to add it to Guichan.
     *
     * @see AllegroGraphics, OpenGLGraphics, SDLGraphics, Image
     */
    class GCN_CORE_DECLSPEC Graphics
    {
    public:
        Graphics();

        virtual ~Graphics() { }

        /**
         * Initializes drawing. Called by the Gui when Gui::draw() is called.
         * It is needed by some implementations of Graphics to perform
         * preparations before drawing. An example of such an implementation
         * would be OpenGLGraphics.
         *
         * NOTE: You will never need to call this function yourself.
         *       Gui will do it for you.
         *
         * @see _endDraw, Gui::draw
         */
        virtual void _beginDraw() { }

        /**
         * Deinitializes drawing. Called by the Gui when a Gui::draw() is done.
         * done. It should reset any state changes made by _beginDraw().
         *
         * NOTE: You will never need to call this function yourself.
         *       Gui will do it for you.
         *
         * @see _beginDraw, Gui::draw
         */
        virtual void _endDraw() { }

        /**
         * Pushes a clip area onto the stack. The x and y coordinates in the
         * Rectangle will be relative to the last pushed clip area.
         * If the new area falls outside the current clip area, it will be
         * clipped as necessary.
         *
         * @param area the clip area to be pushed onto the stack.
         * @return false if the the new area lays totally outside the
         *         current clip area. Note that an empty clip area
         *         will be pused in this case.
         */
        virtual bool pushClipArea(Rectangle area);

        /**
         * Removes the topmost clip area from the stack.
         *
         * @throws Exception if the stack is empty.
         */
        virtual void popClipArea();

        /**
         * Gets the current clip area. Usefull if you want to do drawing
         * bypassing Graphics.
         *
         * @return the current clip area.
         */
        virtual const ClipRectangle& getCurrentClipArea();

        /**
         * Draws a part of an Image.
         *
         * NOTE: Width and height arguments will not scale the Image but
         *       specifies the size of the part to be drawn. If you want
         *       to draw the whole Image there is a simplified version of
         *       this function.
         *
         * EXAMPLE: @code drawImage(myImage, 10, 10, 20, 20, 40, 40); @endcode
         *          Will draw a rectangular piece of myImage starting at
         *          coordinate (10, 10) in myImage, with width and height 40.
         *          The piece will be drawn with it's top left corner at
         *          coordinate (20, 20).
         *
         * @param image the Image to draw.
         * @param srcX source Image x coordinate.
         * @param srcY source Image y coordinate.
         * @param dstX destination x coordinate.
         * @param dstY destination y coordinate.
         * @param width the width of the piece.
         * @param height the height of the piece.
         */
        virtual void drawImage(const Image* image, int srcX, int srcY,
                               int dstX, int dstY, int width,
                               int height) = 0;
        /**
         * Draws an image. A simplified version of the other drawImage.
         * It will draw a whole image at the coordinate you specify.
         * It is equivalent to calling:
         * @code drawImage(myImage, 0, 0, dstX, dstY, image->getWidth(), \
         image->getHeight()); @endcode
         */
        virtual void drawImage(const Image* image, int dstX, int dstY);

        /**
         * Draws a single point/pixel.
         *
         * @param x the x coordinate.
         * @param y the y coordinate.
         */
        virtual void drawPoint(int x, int y) = 0;

        /**
         * Ddraws a line.
         *
         * @param x1 the first x coordinate.
         * @param y1 the first y coordinate.
         * @param x2 the second x coordinate.
         * @param y2 the second y coordinate.
         */
        virtual void drawLine(int x1, int y1, int x2, int y2) = 0;

        /**
         * Draws a simple, non-filled, Rectangle with one pixel width.
         *
         * @param rectangle the Rectangle to draw.
         */
        virtual void drawRectangle(const Rectangle& rectangle) = 0;

        /**
         * Draws a filled Rectangle.
         *
         * @param rectangle the filled Rectangle to draw.
         */
        virtual void fillRectangle(const Rectangle& rectangle) = 0;

        /**
         * Sets the Color to use when drawing.
         *
         * @param color a Color.
         */
        virtual void setColor(const Color& color) = 0;

        /**
         * Gets the Color to use when drawing.
         *
         * @return the Color used when drawing.
         */
        virtual const Color& getColor() = 0;

        /**
         * Sets the font to use when drawing text.
         *
         * @param font the Font to use when drawing.
         */
        virtual void setFont(Font* font);

        /**
         * Draws text.
         *
         * @param text the text to draw.
         * @param x the x coordinate where to draw the text.
         * @param y the y coordinate where to draw the text.
         * @param alignment Graphics::LEFT, Graphics::CENTER or Graphics::RIGHT.
         * @throws Exception when no Font is set.
         */
        virtual void drawText(const std::string& text, int x, int y,
                              unsigned int alignment = LEFT);
        /**
         * Alignments for text drawing.
         */
        enum
        {
            LEFT = 0,
            CENTER,
            RIGHT
        };

    protected:
        std::stack<ClipRectangle> mClipStack;
        Font* mFont;
    };
}

#endif // end GCN_GRAPHICS_HPP

/*
 * yakslem - "little cake on cake, but that's the fall"
 * finalman - "skall jag skriva det?"
 * yakslem - "ja, varfor inte?"
 */

