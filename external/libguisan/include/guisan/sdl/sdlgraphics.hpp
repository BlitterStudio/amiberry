/*      _______   __   __   __   ______   __   __   _______   __   __
 *     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\
 *    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /
 *   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /
 *  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /
 * /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /
 * \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/
 *
 * Copyright (c) 2004, 2005, 2006, 2007 Olof Naess�n and Per Larsson
 *
 *                                                         Js_./
 * Per Larsson a.k.a finalman                          _RqZ{a<^_aa
 * Olof Naess�n a.k.a jansem/yakslem                _asww7!uY`>  )\a//
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

#ifndef GCN_SDLGRAPHICS_HPP
#define GCN_SDLGRAPHICS_HPP

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
	class GCN_EXTENSION_DECLSPEC SDLGraphics : public Graphics
	{
	public:

		// Needed so that drawImage(gcn::Image *, int, int) is visible.
		using Graphics::drawImage;

		/**
		 * Constructor.
		 */
		SDLGraphics();

		/**
		 * Destructor.
		 */
		~SDLGraphics();

		/**
		 * Sets the target SDL_Surface to draw to. The target can be any
		 * SDL_Surface. This function also pushes a clip areas corresponding to
		 * the dimension of the target.
		 *
		 * @param target the target to draw to.
		 */
		virtual void setTarget(SDL_Surface* target);

		/**
		 * Gets the target SDL_Surface.
		 *
		 * @return the target SDL_Surface.
		 */
		[[nodiscard]] virtual SDL_Surface* getTarget() const;

		/**
		 * Draws an SDL_Surface on the target surface. Normally you'll
		 * use drawImage, but if you want to write SDL specific code
		 * this function might come in handy.
		 *
		 * NOTE: The clip areas will be taken into account.
		 */
		virtual void drawSDLSurface(SDL_Surface* surface, SDL_Rect source,
		                            SDL_Rect destination);


		// Inherited from Graphics

		void _beginDraw() override;

		void _endDraw() override;

		bool pushClipArea(Rectangle area) override;

		void popClipArea() override;

		void drawImage(const Image* image, int srcX, int srcY,
		               int dstX, int dstY, int width,
		               int height) override;

		void drawPoint(int x, int y) override;

		void drawLine(int x1, int y1, int x2, int y2) override;

		void drawRectangle(const Rectangle& rectangle) override;

		void fillRectangle(const Rectangle& rectangle) override;

		void setColor(const Color& color) override;

		const Color& getColor() override;

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

		SDL_Surface* mTarget{};
		Color mColor;
		bool mAlpha;
	};
}

#endif // end GCN_SDLGRAPHICS_HPP
