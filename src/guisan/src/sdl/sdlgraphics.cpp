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

/*
 * For comments regarding functions please see the header file.
 */

#include "guisan/sdl/sdlgraphics.hpp"

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

    SDLGraphics::SDLGraphics()
    {
        mAlpha = false;
    }

    void SDLGraphics::_beginDraw()
    {
        Rectangle area;
        area.x = 0;
        area.y = 0;
        area.width = mTarget->w;
        area.height = mTarget->h;
        pushClipArea(area);
    }

    void SDLGraphics::_endDraw()
    {
        popClipArea();
    }

    void SDLGraphics::setTarget(SDL_Surface* target)
    {
        mTarget = target;
    }

    bool SDLGraphics::pushClipArea(Rectangle area)
    {
        SDL_Rect rect;
        bool result = Graphics::pushClipArea(area);

        const ClipRectangle& carea = mClipStack.top();
        rect.x = carea.x;
        rect.y = carea.y;
        rect.w = carea.width;
        rect.h = carea.height;

        SDL_SetClipRect(mTarget, &rect);

        return result;
    }

    void SDLGraphics::popClipArea()
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

        SDL_SetClipRect(mTarget, &rect);
    }

    SDL_Surface* SDLGraphics::getTarget() const
    {
        return mTarget;
    }

    void SDLGraphics::drawImage(const Image* image, int srcX,
                                int srcY, int dstX, int dstY,
                                int width, int height)
    {
	if (mClipStack.empty()) {
		throw GCN_EXCEPTION("Clip stack is empty, perhaps you"
			"called a draw funtion outside of _beginDraw() and _endDraw()?");
	}

	const ClipRectangle& top = mClipStack.top();
        SDL_Rect src;
        SDL_Rect dst;
        src.x = srcX;
        src.y = srcY;
        src.w = width;
        src.h = height;
        dst.x = dstX + top.xOffset;
        dst.y = dstY + top.yOffset;

        const SDLImage* srcImage = dynamic_cast<const SDLImage*>(image);

        if (srcImage == NULL)
        {
            throw GCN_EXCEPTION("Trying to draw an image of unknown format, must be an SDLImage.");
        }

        SDL_BlitSurface(srcImage->getSurface(), &src, mTarget, &dst);
    }

    void SDLGraphics::fillRectangle(const Rectangle& rectangle)
    {
	if (mClipStack.empty()) {
		throw GCN_EXCEPTION("Clip stack is empty, perhaps you"
			"called a draw funtion outside of _beginDraw() and _endDraw()?");
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

            SDL_LockSurface(mTarget);
            for (y = y1; y < y2; y++)
            {
                for (x = x1; x < x2; x++)
                {
                    SDLputPixelAlpha(mTarget, x, y, mColor);
                }
            }
            SDL_UnlockSurface(mTarget);

        }
        else
        {
            SDL_Rect rect;
            rect.x = area.x;
            rect.y = area.y;
            rect.w = area.width;
            rect.h = area.height;

            Uint32 color = SDL_MapRGBA(mTarget->format, mColor.r, mColor.g, mColor.b, mColor.a);
            SDL_FillRect(mTarget, &rect, color);
        }
    }

    void SDLGraphics::drawPoint(int x, int y)
    {
	if (mClipStack.empty()) {
		throw GCN_EXCEPTION("Clip stack is empty, perhaps you"
			"called a draw funtion outside of _beginDraw() and _endDraw()?");
	}

        const ClipRectangle& top = mClipStack.top();

        x += top.xOffset;
        y += top.yOffset;

        if(!top.isPointInRect(x,y))
            return;

        if (mAlpha)
        {
            SDLputPixelAlpha(mTarget, x, y, mColor);
        }
        else
        {
            SDLputPixel(mTarget, x, y, mColor);
        }
    }

    void SDLGraphics::drawHLine(int x1, int y, int x2)
    {
	if (mClipStack.empty()) {
		throw GCN_EXCEPTION("Clip stack is empty, perhaps you"
			"called a draw funtion outside of _beginDraw() and _endDraw()?");
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

        int bpp = mTarget->format->BytesPerPixel;

        SDL_LockSurface(mTarget);

        Uint8 *p = (Uint8 *)mTarget->pixels + y * mTarget->pitch + x1 * bpp;

        Uint32 pixel = SDL_MapRGB(mTarget->format, mColor.r, mColor.g, mColor.b);

        switch(bpp) {
          case 1:
          {
              for (;x1 <= x2; ++x1)
              {
                  *(p++) = pixel;
              }
          } break;

          case 2:
          {
              Uint16* q = (Uint16*)p;
              for (;x1 <= x2; ++x1)
              {
                  *(q++) = pixel;
              }
          } break;

          case 3:
          {
              if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                  for (;x1 <= x2; ++x1)
                  {
                      p[0] = (pixel >> 16) & 0xff;
                      p[1] = (pixel >> 8) & 0xff;
                      p[2] = pixel & 0xff;
                      p += 3;
                  }
              }
              else
              {
                  for (;x1 <= x2; ++x1)
                  {
                      p[0] = pixel & 0xff;
                      p[1] = (pixel >> 8) & 0xff;
                      p[2] = (pixel >> 16) & 0xff;
                      p += 3;
                  }
              }
          } break;

          case 4:
          {
              Uint32* q = (Uint32*)p;
              for (;x1 <= x2; ++x1)
              {
                  if (mAlpha)
                  {
                      *q = SDLAlpha32(pixel,*q,mColor.a);
                      q++;
                  }
                  else
                  {
                      *(q++) = pixel;
                  }
              }
          } break;

        } // end switch

        SDL_UnlockSurface(mTarget);
    }

    void SDLGraphics::drawVLine(int x, int y1, int y2)
    {
	if (mClipStack.empty()) {
		throw GCN_EXCEPTION("Clip stack is empty, perhaps you"
			"called a draw funtion outside of _beginDraw() and _endDraw()?");
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

        int bpp = mTarget->format->BytesPerPixel;

        SDL_LockSurface(mTarget);

        Uint8 *p = (Uint8 *)mTarget->pixels + y1 * mTarget->pitch + x * bpp;

        Uint32 pixel = SDL_MapRGB(mTarget->format, mColor.r, mColor.g, mColor.b);

        switch(bpp) {
          case 1:
          {
              for (;y1 <= y2; ++y1)
              {
                  *p = pixel;
                  p += mTarget->pitch;
              }
          } break;

          case 2:
          {
              for (;y1 <= y2; ++y1)
              {
                  *(Uint16*)p = pixel;
                  p += mTarget->pitch;
              }
          } break;

          case 3:
          {
              if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                  for (;y1 <= y2; ++y1)
                  {
                      p[0] = (pixel >> 16) & 0xff;
                      p[1] = (pixel >> 8) & 0xff;
                      p[2] = pixel & 0xff;
                      p += mTarget->pitch;
                  }
              }
              else
              {
                  for (;y1 <= y2; ++y1)
                  {
                      p[0] = pixel & 0xff;
                      p[1] = (pixel >> 8) & 0xff;
                      p[2] = (pixel >> 16) & 0xff;
                      p += mTarget->pitch;
                  }
              }
          } break;

          case 4:
          {
              for (;y1 <= y2; ++y1)
              {
                  if (mAlpha)
                  {
                      *(Uint32*)p = SDLAlpha32(pixel,*(Uint32*)p,mColor.a);
                  }
                  else
                  {
                      *(Uint32*)p = pixel;
                  }
                  p += mTarget->pitch;
              }
          } break;
        } // end switch

        SDL_UnlockSurface(mTarget);
    }

    void SDLGraphics::drawRectangle(const Rectangle& rectangle)
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

    void SDLGraphics::drawLine(int x1, int y1, int x2, int y2)
    {
        if (x1 == x2)
        {
            drawVLine(x1, y1, y2);
            return;
        }
        if (y1 == y2)
        {
            drawHLine(x1, y1, x2);
            return;
        }

	if (mClipStack.empty()) {
		throw GCN_EXCEPTION("Clip stack is empty, perhaps you"
			"called a draw funtion outside of _beginDraw() and _endDraw()?");
	}
        const ClipRectangle& top = mClipStack.top();

        x1 += top.xOffset;
        y1 += top.yOffset;
        x2 += top.xOffset;
        y2 += top.yOffset;

        // Draw a line with Bresenham

        int dx = ABS(x2 - x1);
        int dy = ABS(y2 - y1);

        if (dx > dy)
        {
            if (x1 > x2)
            {
                // swap x1, x2
                x1 ^= x2;
                x2 ^= x1;
                x1 ^= x2;

                // swap y1, y2
                y1 ^= y2;
                y2 ^= y1;
                y1 ^= y2;
            }

            if (y1 < y2)
            {
                int y = y1;
                int p = 0;

                for (int x = x1; x <= x2; x++)
                {
                    if (top.isPointInRect(x, y))
                    {
                        if (mAlpha)
                        {
                            SDLputPixelAlpha(mTarget, x, y, mColor);
                        }
                        else
                        {
                            SDLputPixel(mTarget, x, y, mColor);
                        }
                    }

                    p += dy;

                    if (p * 2 >= dx)
                    {
                        y++;
                        p -= dx;
                    }
                }
            }
            else
            {
                int y = y1;
                int p = 0;

                for (int x = x1; x <= x2; x++)
                {
                    if (top.isPointInRect(x, y))
                    {
                        if (mAlpha)
                        {
                            SDLputPixelAlpha(mTarget, x, y, mColor);
                        }
                        else
                        {
                            SDLputPixel(mTarget, x, y, mColor);
                        }
                    }

                    p += dy;

                    if (p * 2 >= dx)
                    {
                        y--;
                        p -= dx;
                    }
                }
            }
        }
        else
        {
            if (y1 > y2)
            {
                // swap y1, y2
                y1 ^= y2;
                y2 ^= y1;
                y1 ^= y2;

                // swap x1, x2
                x1 ^= x2;
                x2 ^= x1;
                x1 ^= x2;
            }

            if (x1 < x2)
            {
                int x = x1;
                int p = 0;

                for (int y = y1; y <= y2; y++)
                {
                    if (top.isPointInRect(x, y))
                    {
                        if (mAlpha)
                        {
                            SDLputPixelAlpha(mTarget, x, y, mColor);
                        }
                        else
                        {
                            SDLputPixel(mTarget, x, y, mColor);
                        }
                    }

                    p += dx;

                    if (p * 2 >= dy)
                    {
                        x++;
                        p -= dy;
                    }
                }
            }
            else
            {
                int x = x1;
                int p = 0;

                for (int y = y1; y <= y2; y++)
                {
                    if (top.isPointInRect(x, y))
                    {
                        if (mAlpha)
                        {
                            SDLputPixelAlpha(mTarget, x, y, mColor);
                        }
                        else
                        {
                            SDLputPixel(mTarget, x, y, mColor);
                        }
                    }

                    p += dx;

                    if (p * 2 >= dy)
                    {
                        x--;
                        p -= dy;
                    }
                }
            }
        }
    }

    void SDLGraphics::setColor(const Color& color)
    {
        mColor = color;

        mAlpha = color.a != 255;
    }

    const Color& SDLGraphics::getColor()
    {
        return mColor;
    }

    void SDLGraphics::drawSDLSurface(SDL_Surface* surface, SDL_Rect source,
                                     SDL_Rect destination)
    {
	if (mClipStack.empty()) {
		throw GCN_EXCEPTION("Clip stack is empty, perhaps you"
			"called a draw funtion outside of _beginDraw() and _endDraw()?");
	}
        const ClipRectangle& top = mClipStack.top();

        destination.x += top.xOffset;
        destination.y += top.yOffset;

        SDL_BlitSurface(surface, &source, mTarget, &destination);
    }
}
