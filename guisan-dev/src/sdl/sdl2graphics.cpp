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

/*
 * For comments regarding functions please see the header file.
 */

#include "guisan/sdl/sdl2graphics.hpp"

#include "guisan/exception.hpp"
#include "guisan/font.hpp"
#include "guisan/image.hpp"
#include "guisan/sdl/sdlimage.hpp"
#include "guisan/sdl/sdlpixel.hpp"

// For some reason an old version of MSVC did not like std::abs,
// so we added this macro.
#ifndef ABS
#define ABS(x) ((x)<0?-(x):(x))
#endif

namespace gcn
{

    SDL2Graphics::SDL2Graphics()
    {
        mAlpha = false;
    }
    
    SDL2Graphics::~SDL2Graphics()
    {
        if(mRenderTarget != NULL)
        {
            SDL_DestroyTexture(mTexture);
            SDL_FreeSurface(mTarget);
        }
    }

    void SDL2Graphics::_beginDraw()
    {
        Rectangle area;
        area.x = 0;
        area.y = 0;
        area.width = mTarget->w;
        area.height = mTarget->h;
        pushClipArea(area);
    }

    void SDL2Graphics::_endDraw()
    {
        popClipArea();
    }
    
    void SDL2Graphics::setTarget(SDL_Renderer* renderer, int width, int height)
    {
        mRenderTarget = renderer;
        // An internal surface is still required to be able to handle surfaces and colorkeys
        mTarget = SDL_CreateRGBSurface(0, width, height, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
        SDL_FillRect(mTarget, NULL, SDL_MapRGB(mTarget->format, 0xff, 0, 0xff));
        SDL_SetColorKey(mTarget, SDL_TRUE, SDL_MapRGB(mTarget->format, 0xff, 0, 0xff)); // magenta, Guisan default
        SDL_SetSurfaceBlendMode(mTarget, SDL_BLENDMODE_NONE); // needed to cleanup temp data properly
        mTexture = SDL_CreateTextureFromSurface(mRenderTarget, mTarget);
        SDL_SetTextureBlendMode(mTexture, SDL_BLENDMODE_BLEND);
    }

    bool SDL2Graphics::pushClipArea(Rectangle area)
    {
        SDL_Rect rect;
        bool result = Graphics::pushClipArea(area);

        const ClipRectangle& carea = mClipStack.top();
        rect.x = carea.x;
        rect.y = carea.y;
        rect.w = carea.width;
        rect.h = carea.height;

        SDL_RenderSetClipRect(mRenderTarget, &rect);

        return result;
    }

    void SDL2Graphics::popClipArea()
    {
        Graphics::popClipArea();

        if (mClipStack.empty())
        {
            return;
        }

        const ClipRectangle& carea = mClipStack.top();
        SDL_Rect rect;
        rect.x = carea.x;
        rect.y = carea.y;
        rect.w = carea.width;
        rect.h = carea.height;

        SDL_RenderSetClipRect(mRenderTarget, &rect);
    }
    
    SDL_Renderer* SDL2Graphics::getTarget() const
    {
        return mRenderTarget;
    }

    void SDL2Graphics::drawImage(const Image* image, int srcX,
                                int srcY, int dstX, int dstY,
                                int width, int height)
    {
        if (mClipStack.empty()) {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you"
                "called a draw function outside of _beginDraw() and _endDraw()?");
        }
    
        const ClipRectangle& top = mClipStack.top();
        SDL_Rect src;
        SDL_Rect dst;
        SDL_Rect temp;
        src.x = srcX;
        src.y = srcY;
        src.w = width;
        src.h = height;
        dst.x = dstX + top.xOffset;
        dst.y = dstY + top.yOffset;
        dst.w = width;
        dst.h = height;
        temp.x = 0;
        temp.y = 0;
        temp.w = width;
        temp.h = height;

        const SDLImage* srcImage = dynamic_cast<const SDLImage*>(image);

        if (srcImage == NULL)
        {
            throw GCN_EXCEPTION("Trying to draw an image of unknown format, must be an SDLImage.");
        }
        
        if(srcImage->getTexture() == NULL)
        {
            SDL_FillRect(mTarget, &temp, SDL_MapRGBA(mTarget->format, 0xff, 0, 0xff, 0));
            SDL_BlitSurface(srcImage->getSurface(), &src, mTarget, &temp);
            SDL_UpdateTexture(mTexture, &temp, mTarget->pixels, mTarget->pitch);
            SDL_RenderCopy(mRenderTarget, mTexture, &temp, &dst);
        } 
        else 
        {
            SDL_RenderCopy(mRenderTarget, srcImage->getTexture(), &src, &dst);
        }
        
    }

    void SDL2Graphics::fillRectangle(const Rectangle& rectangle)
    {
    if (mClipStack.empty()) {
        throw GCN_EXCEPTION("Clip stack is empty, perhaps you"
            "called a draw function outside of _beginDraw() and _endDraw()?");
    }

        const ClipRectangle& top = mClipStack.top();

        Rectangle area = rectangle;
        area.x += top.xOffset;
        area.y += top.yOffset;

        if(!area.intersect(top))
        {
            return;
        }

        if (mAlpha)
        {
            int x1 = area.x > top.x ? area.x : top.x;
            int y1 = area.y > top.y ? area.y : top.y;
            int x2 = area.x + area.width < top.x + top.width ? area.x + area.width : top.x + top.width;
            int y2 = area.y + area.height < top.y + top.height ? area.y + area.height : top.y + top.height;
            int x, y;
            SDL_Rect rect;
            rect.x = x1;
            rect.y = y1;
            rect.w = x2 - x1;
            rect.h = y2 - y1;
            
            saveRenderColor();
            SDL_SetRenderDrawColor(mRenderTarget, mColor.r, mColor.g, mColor.b, mColor.a);
            SDL_RenderFillRect(mRenderTarget, &rect);
            restoreRenderColor();

        }
        else
        {
            SDL_Rect rect;
            rect.x = area.x;
            rect.y = area.y;
            rect.w = area.width;
            rect.h = area.height;

            Uint32 color = SDL_MapRGBA(mTarget->format, mColor.r, mColor.g, mColor.b, mColor.a);
            saveRenderColor();
            SDL_SetRenderDrawColor(mRenderTarget, mColor.r, mColor.g, mColor.b, mColor.a);
            SDL_RenderFillRect(mRenderTarget, &rect);
            restoreRenderColor();
            
        }
    }

    void SDL2Graphics::drawPoint(int x, int y)
    {
        if (mClipStack.empty()) {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you"
                "called a draw function outside of _beginDraw() and _endDraw()?");
        }

        const ClipRectangle& top = mClipStack.top();

        x += top.xOffset;
        y += top.yOffset;

        if(!top.isPointInRect(x,y))
            return;

        saveRenderColor();
        SDL_SetRenderDrawColor(mRenderTarget, mColor.r, mColor.g, mColor.b, mColor.a);
        /*if (mAlpha)
        {
            SDLputPixelAlpha(mTarget, x, y, mColor);
            
        }
        else
        {
            SDLputPixel(mTarget, x, y, mColor);
        }*/
        SDL_RenderDrawPoint(mRenderTarget, x, y);
        restoreRenderColor();
    }

    void SDL2Graphics::drawHLine(int x1, int y, int x2)
    {
        if (mClipStack.empty()) {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you"
                "called a draw function outside of _beginDraw() and _endDraw()?");
        }
        const ClipRectangle& top = mClipStack.top();

        x1 += top.xOffset;
        y += top.yOffset;
        x2 += top.xOffset;

        if (y < top.y || y >= top.y + top.height)
            return;

        if (x1 > x2)
        {
            x1 ^= x2;
            x2 ^= x1;
            x1 ^= x2;
        }

        if (top.x > x1)
        {
            if (top.x > x2)
            {
                return;
            }
            x1 = top.x;
        }

        if (top.x + top.width <= x2)
        {
            if (top.x + top.width <= x1)
            {
                return;
            }
            x2 = top.x + top.width -1;
        }

        saveRenderColor();
        SDL_SetRenderDrawColor(mRenderTarget, mColor.r, mColor.g, mColor.b, mColor.a);
        SDL_RenderDrawLine(mRenderTarget, x1, y, x2, y); 
        restoreRenderColor();
    }

    void SDL2Graphics::drawVLine(int x, int y1, int y2)
    {
        if (mClipStack.empty()) {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you"
                "called a draw function outside of _beginDraw() and _endDraw()?");
        }
        const ClipRectangle& top = mClipStack.top();

        x += top.xOffset;
        y1 += top.yOffset;
        y2 += top.yOffset;

        if (x < top.x || x >= top.x + top.width)
            return;

        if (y1 > y2)
        {
            y1 ^= y2;
            y2 ^= y1;
            y1 ^= y2;
        }

        if (top.y > y1)
        {
            if (top.y > y2)
            {
                return;
            }
            y1 = top.y;
        }

        if (top.y + top.height <= y2)
        {
            if (top.y + top.height <= y1)
            {
                return;
            }
            y2 = top.y + top.height - 1;
        }

        saveRenderColor();
        SDL_SetRenderDrawColor(mRenderTarget, mColor.r, mColor.g, mColor.b, mColor.a);
        SDL_RenderDrawLine(mRenderTarget, x, y1, x, y2);
        restoreRenderColor();
    }
        
    void SDL2Graphics::drawRectangle(const Rectangle& rectangle)
    {
        int x1 = rectangle.x;
        int x2 = rectangle.x + rectangle.width - 1;
        int y1 = rectangle.y;
        int y2 = rectangle.y + rectangle.height - 1;

        drawHLine(x1, y1, x2);
        drawHLine(x1, y2, x2);

        drawVLine(x1, y1, y2);
        drawVLine(x2, y1, y2);
    }

    void SDL2Graphics::drawLine(int x1, int y1, int x2, int y2)
    {
        
        if (mClipStack.empty()) {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you"
                "called a draw function outside of _beginDraw() and _endDraw()?");
        }
        const ClipRectangle& top = mClipStack.top();

        x1 += top.xOffset;
        y1 += top.yOffset;
        x2 += top.xOffset;
        y2 += top.yOffset;

        saveRenderColor();
        SDL_SetRenderDrawColor(mRenderTarget, mColor.r, mColor.g, mColor.b, mColor.a);
        SDL_RenderDrawLine(mRenderTarget, x1, y1, x2, y2); 
        restoreRenderColor();
    }

    void SDL2Graphics::setColor(const Color& color)
    {
        mColor = color;

        mAlpha = color.a != 255;
    }

    const Color& SDL2Graphics::getColor()
    {
        return mColor;
    }

    void SDL2Graphics::drawSDLSurface(SDL_Surface* surface, SDL_Rect source,
                                     SDL_Rect destination)
    {
        if (mClipStack.empty()) {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you"
                "called a draw function outside of _beginDraw() and _endDraw()?");
        }
        const ClipRectangle& top = mClipStack.top();

        destination.x += top.xOffset;
        destination.y += top.yOffset;
        destination.w = source.w;
        destination.h = source.h;
        SDL_Rect temp;
        temp.x = 0;
        temp.y = 0;
        temp.w = source.w;
        temp.h = source.h;

        SDL_FillRect(mTarget, &temp, SDL_MapRGBA(mTarget->format, 0xff, 0, 0xff, 0));
        SDL_BlitSurface(surface, &source, mTarget, &temp);
        SDL_UpdateTexture(mTexture, &temp, mTarget->pixels, mTarget->pitch);
        SDL_RenderCopy(mRenderTarget, mTexture, &temp, &destination);
    }

    void SDL2Graphics::drawSDLTexture(SDL_Texture * texture, SDL_Rect source, 
                                      SDL_Rect destination)
    {
        if (mClipStack.empty()) {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you"
                "called a draw function outside of _beginDraw() and _endDraw()?");
        }
        const ClipRectangle& top = mClipStack.top();

        destination.x += top.xOffset;
        destination.y += top.yOffset;
        destination.w = source.w;
        destination.h = source.h;

        SDL_RenderCopy(mRenderTarget, texture, &source, &destination);
    }
    
    void SDL2Graphics::saveRenderColor()
    {
        SDL_GetRenderDrawColor(mRenderTarget, &r, &g, &b, &a);
    }
    
    void SDL2Graphics::restoreRenderColor()
    {
        SDL_SetRenderDrawColor(mRenderTarget, r, g, b, a);
    }
}
