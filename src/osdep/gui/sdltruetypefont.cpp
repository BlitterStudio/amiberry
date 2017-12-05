/*      _______   __   __   __   ______   __   __   _______   __   __
 *     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\
 *    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /
 *   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /
 *  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /
 * /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /
 * \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/
 *
 * Copyright (c) 2004 - 2008 Olof Naessén and Per Larsson
 *
 *
 * Per Larsson a.k.a finalman
 * Olof Naessén a.k.a jansem/yakslem
 *
 * Visit: http://guichan.sourceforge.net
 *
 * License: (BSD)
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Guichan nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * For comments regarding functions please see the header file. 
 */

#include "sdltruetypefont.hpp"

#include "guichan/exception.hpp"
#include "guichan/image.hpp"
#include "guichan/graphics.hpp"
#include "guichan/sdl/sdlgraphics.hpp"

namespace gcn
{
    namespace contrib
    {
        SDLTrueTypeFont::SDLTrueTypeFont (const std::string& filename, int size)
        {
            mRowSpacing = 0;
            mGlyphSpacing = 0;
            mAntiAlias = true;        
            mFilename = filename;
            mFont = NULL;
        
            mFont = TTF_OpenFont(filename.c_str(), size);
        
            if (mFont == NULL)
            {
                throw GCN_EXCEPTION("SDLTrueTypeFont::SDLTrueTypeFont. "+std::string(TTF_GetError()));
            }
        }
    
        SDLTrueTypeFont::~SDLTrueTypeFont()
        {
            TTF_CloseFont(mFont);
        }
  
        int SDLTrueTypeFont::getWidth(const std::string& text) const
        {
            int w, h;
            TTF_SizeText(mFont, text.c_str(), &w, &h);
        
            return w;
        }

        int SDLTrueTypeFont::getHeight() const
        {
            return TTF_FontHeight(mFont) + mRowSpacing;
        }
    
        void SDLTrueTypeFont::drawString(gcn::Graphics* graphics, const std::string& text, const int x, const int y)
        {
            if (text == "")
            {
                return;
            }
        
            gcn::SDLGraphics *sdlGraphics = dynamic_cast<gcn::SDLGraphics *>(graphics);

            if (sdlGraphics == NULL)
            {
                throw GCN_EXCEPTION("SDLTrueTypeFont::drawString. Graphics object not an SDL graphics object!");
                return;
            }
        
            // This is needed for drawing the Glyph in the middle if we have spacing
            int yoffset = getRowSpacing() / 2;
        
            Color col = sdlGraphics->getColor();

            SDL_Color sdlCol;
            sdlCol.b = col.b;
            sdlCol.r = col.r;
            sdlCol.g = col.g;

            SDL_Surface *textSurface;
            if (mAntiAlias)
            {
                textSurface = TTF_RenderText_Blended(mFont, text.c_str(), sdlCol);
            }
            else
            {
                textSurface = TTF_RenderText_Solid(mFont, text.c_str(), sdlCol);
            }
        
            if(textSurface != NULL) {
              SDL_Rect dst, src;
              dst.x = x;
              dst.y = y + yoffset;
              src.w = textSurface->w;
              src.h = textSurface->h;
              src.x = 0;
              src.y = 0;
          
              sdlGraphics->drawSDLSurface(textSurface, src, dst);
              SDL_FreeSurface(textSurface);        
              textSurface = NULL;
            }
        }
    
        void SDLTrueTypeFont::setRowSpacing(int spacing)
        {
            mRowSpacing = spacing;
        }

        int SDLTrueTypeFont::getRowSpacing()
        {
            return mRowSpacing;
        }
    
        void SDLTrueTypeFont::setGlyphSpacing(int spacing)
        {
            mGlyphSpacing = spacing;
        }
    
        int SDLTrueTypeFont::getGlyphSpacing()
        {
            return mGlyphSpacing;
        }

        void SDLTrueTypeFont::setAntiAlias(bool antiAlias)
        {
            mAntiAlias = antiAlias;
        }

        bool SDLTrueTypeFont::isAntiAlias()
        {
            return mAntiAlias;        
        }    
    }
}

