/*      _______   __   __   __   ______   __   __   _______   __   __
 *     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\
 *    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /
 *   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /
 *  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /
 * /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /
 * \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/
 *
 * Copyright (c) 2004, 2005, 2006, 2007 Olof Naessén and Per Larsson
 * Copyright (c) 2016, 2018, 2019 Gwilherm Baudic
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

#ifndef GCN_SDL2GRAPHICS_HPP
#define GCN_SDL2GRAPHICS_HPP

#include "SDL.h"

#include "guisan/color.hpp"
#include "guisan/graphics.hpp"
#include "guisan/platform.hpp"

namespace gcn
{
    class Image;
    class Rectangle;

    /**
     * SDL implementation of the Graphics.
     */
    class GCN_EXTENSION_DECLSPEC SDL2Graphics : public Graphics
    {
    public:

        // Needed so that drawImage(gcn::Image *, int, int) is visible.
        using Graphics::drawImage;

        /**
         * Constructor.
         */
        SDL2Graphics();
        
        /**
         * Destructor.
         */
        ~SDL2Graphics();
        
        /**
         *  Sets the target SDL_Renderer to use for drawing. Preferably done only once. 
         *  
         *  @param renderer the SDL_Renderer to use for drawing.
         *  @param width screen width
         *  @param height screen height
         */
        virtual void setTarget(SDL_Renderer* renderer, int width, int height);
        
        /**
         * Gets the target SDL_Renderer.
         *
         * @return the target SDL_Renderer.
         */
        virtual SDL_Renderer* getTarget() const;

        /**
         * Draws an SDL_Surface on the target surface. Normally you'll
         * use drawImage, but if you want to write SDL specific code
         * this function might come in handy.
         *
         * NOTE: The clip areas will be taken into account.
         */
        virtual void drawSDLSurface(SDL_Surface* surface, SDL_Rect source,
                                    SDL_Rect destination);

        /**
         * Draws an SDL_Texture on the target surface. Normally you'll
         * use drawImage, but if you want to write SDL specific code
         * this function might come in handy.
         *
         * NOTE: The clip areas will be taken into account.
         */
        virtual void drawSDLTexture(SDL_Texture* texture, SDL_Rect source,
                                    SDL_Rect destination);

        // Inherited from Graphics

        virtual void _beginDraw();

        virtual void _endDraw();

        virtual bool pushClipArea(Rectangle area);

        virtual void popClipArea();

        virtual void drawImage(const Image* image, int srcX, int srcY,
                               int dstX, int dstY, int width,
                               int height);

        virtual void drawPoint(int x, int y);

        virtual void drawLine(int x1, int y1, int x2, int y2);

        virtual void drawRectangle(const Rectangle& rectangle);

        virtual void fillRectangle(const Rectangle& rectangle);

        virtual void setColor(const Color& color);

        virtual const Color& getColor();

    protected:
        /**
         * Draws a horizontal line.
         *
         * @param x1 the start coordinate of the line.
         * @param y the y coordinate of the line.
         * @param x2 the end coordinate of the line.
         */
        virtual void drawHLine(int x1, int y, int x2);

        /**
         * Draws a vertical line.
         *
         * @param x the x coordinate of the line.
         * @param y1 the start coordinate of the line.
         * @param y2 the end coordinate of the line.
         */
        virtual void drawVLine(int x, int y1, int y2);
        
        /**
         *  Save the current rendering color before drawing.
         *  Does not affect the mColor attribute. 
         */
        virtual void saveRenderColor();
        
        /**
         *  Restore the rendering color after drawing
         */
        virtual void restoreRenderColor();

        SDL_Surface* mTarget;
        SDL_Renderer* mRenderTarget;
        SDL_Texture* mTexture;
        Color mColor;
        Uint8 r, g, b, a; //! to store previous color from renderer
        bool mAlpha;
    };
}

#endif // end GCN_SDL2GRAPHICS_HPP
